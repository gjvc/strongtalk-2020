
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/CodeGenerator.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/code/Locations.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


// Computes the byte offset from the beginning of an Oop
static inline std::int32_t byteOffset( std::int32_t offset ) {
    st_assert( offset >= 0, "bad offset" );
    return offset * sizeof( Oop ) - MEMOOP_TAG;
}


//
// Sometimes a little stub has to be generated if a merge between two execution
// paths requires that the PseudoRegisterMapping of one path is made conformant with the
// mapping of the other path.
//
// If this code cannot be emitted in place (because it would destroy the mapping in use) a conditional jump to the stub instructions
// is generated instead (note that in case of an absolute jump, merge code can
// always be emitted in place).
//
// A Stub holds the necessary information to generate the merge instructions.
//
// Stub routines should eventually be canonicalized if possible -> saves Space. FIX THIS

class Stub : public ResourceObject {

private:
    PseudoRegisterMapping *_mapping;
    Node                  *_dst;
    Label                 _stub_code;


    Stub( PseudoRegisterMapping *mapping, Node *dst ) {
        st_assert( dst->hasMapping() and not mapping->isConformant( dst->mapping() ), "no stub required" );
        _mapping = mapping;
        _dst     = dst;
    }


public:
    static Stub *new_jcc_stub( PseudoRegisterMapping *mapping, Node *dst, Assembler::Condition cc ) {
        Stub *s = new Stub( mapping, dst );
        // generate conditional jump to stub code
        mapping->assembler()->jcc( cc, s->_stub_code );
        return s;
    }


    static Stub *new_NonLocalReturn_stub( PseudoRegisterMapping *mapping, Node *dst, std::int32_t flags ) {
        Stub *s = new Stub( mapping, dst );
        // generate inline cache with NonLocalReturn jumping to stub code
        mapping->assembler()->ic_info( s->_stub_code, flags );
        return s;
    }


    void generateMergeStub() {
        _mapping->assembler()->bind( _stub_code );
        _mapping->makeConformant( _dst->mapping() );
        _mapping->assembler()->jmp( _dst->_label );
    }
};


class DebugInfoWriter : public PseudoRegisterClosure {
private:
    GrowableArray<PseudoRegister *> *_pseudoRegisters;            // maps index -> pseudoRegister
    GrowableArray<std::int32_t>     *_locations;        // previous pseudoRegister location or Location::ILLEGAL_LOCATION
    GrowableArray<bool>             *_present;          // true if pseudoRegister is currently present

    Location location_at( std::int32_t i ) {
        return Location( _locations->at( i ) );
    }


    void location_at_put( std::int32_t i, Location loc ) {
        _locations->at_put( i, loc._loc );
    }


public:
    DebugInfoWriter( std::int32_t number_of_pseudoRegisters ) {
        _pseudoRegisters = new GrowableArray<PseudoRegister *>( number_of_pseudoRegisters, number_of_pseudoRegisters, nullptr );
        _locations       = new GrowableArray<std::int32_t>( number_of_pseudoRegisters, number_of_pseudoRegisters, Location::ILLEGAL_LOCATION._loc );
        _present         = new GrowableArray<bool>( number_of_pseudoRegisters, number_of_pseudoRegisters, false );
    }


    void pseudoRegister_do( PseudoRegister *pseudoRegister ) {
        if ( pseudoRegister->logicalAddress() not_eq nullptr and not pseudoRegister->_location.isContextLocation() ) {
            // record only debug-visible PseudoRegisters & ignore context PseudoRegisters
            // Note: ContextPseudoRegisters appear in the mapping only because
            //       their values might also be cached in a register.
            std::int32_t i = pseudoRegister->id();
            _pseudoRegisters->at_put( i, pseudoRegister );        // make sure pseudoRegister is available
            _present->at_put( i, true );        // mark it as present
        }
    }


    void write_debug_info( PseudoRegisterMapping *mapping, std::int32_t pc_offset ) {
        // record current pseudoRegisters in mapping
        mapping->iterate( this );
        // determine changes & notify ScopeDescriptorRecorder if necessary
        ScopeDescriptorRecorder *rec = theCompiler->scopeDescRecorder();

        for ( std::int32_t i = _locations->length(); i-- > 0; ) {

            PseudoRegister *pseudoRegister = _pseudoRegisters->at( i );
            bool           present         = _present->at( i );
            Location       old_loc         = location_at( i );
            Location       new_loc         = present ? mapping->locationFor( pseudoRegister ) : Location::ILLEGAL_LOCATION;

            if ( ( not present and old_loc not_eq Location::ILLEGAL_LOCATION ) or
                 // pseudoRegister not present anymore but has been there before
                 ( present and old_loc == Location::ILLEGAL_LOCATION ) or    // pseudoRegister present but has not been there before
                 ( present and old_loc not_eq new_loc ) ) {        // pseudoRegister present but has changed location
                // pseudoRegister location has changed => notify ScopeDescriptorRecorder
                NameNode *nameNode;
                if ( new_loc == Location::ILLEGAL_LOCATION ) {
                    nameNode = new IllegalName();
                } else {
                    nameNode = new LocationName( new_loc );
                }
                // debugging
                if ( PrintDebugInfoGeneration ) {
                    spdlog::info( "{:5d}: {:20s} @ {}", pc_offset, pseudoRegister->name(), new_loc.name() );
                }
                rec->changeLogicalAddress( pseudoRegister->logicalAddress(), nameNode, pc_offset );
            }
            location_at_put( i, new_loc );  // record current location
            _present->at_put( i, false );   // mark as not present for next round
        }
    }


    void print() {
        spdlog::info( "a DebugInfoWriter" );
    }
};


// Implementation of CodeGenerator

CodeGenerator::CodeGenerator( MacroAssembler *masm, PseudoRegisterMapping *mapping ) :
    _mergeStubs( 16 ) {
    st_assert( masm == mapping->assembler(), "should be the same" );
    PseudoRegisterLocker::initialize();
    _masm            = masm;
    _currentMapping  = mapping;
    _debugInfoWriter = new DebugInfoWriter( bbIterator->pseudoRegisterTable->length() );
    _maxNofStackTmps = 0;
    _previousNode    = nullptr;
    _nilReg          = noreg;
    _pushCode        = nullptr;
}


void CodeGenerator::setMapping( PseudoRegisterMapping *mapping ) {
    maxNofStackTmps(); // enforce adjustment of _maxNofStackTmps
    _currentMapping = mapping;
}


std::int32_t CodeGenerator::maxNofStackTmps() {
    if ( _currentMapping not_eq nullptr ) {
        _maxNofStackTmps = max( _maxNofStackTmps, _currentMapping->maxNofStackTmps() );
    }
    return _maxNofStackTmps;
}


Register CodeGenerator::def( PseudoRegister *pseudoRegister ) const {
    st_assert( not pseudoRegister->isConstPseudoRegister(), "cannot assign to ConstPseudoRegister" );
    st_assert( not pseudoRegister->_location.isContextLocation(), "cannot assign into context yet" );
    return _currentMapping->def( pseudoRegister );
}


bool CodeGenerator::isLiveRangeBoundary( Node *a, Node *b ) const {
    return a->scope() not_eq b->scope() or a->byteCodeIndex() not_eq b->byteCodeIndex();
}


void CodeGenerator::jmp( Node *from, Node *to, bool to_maybe_nontrivial ) {
    // keep only PseudoRegisters that are still alive at dst
    if ( from not_eq nullptr and isLiveRangeBoundary( from, to ) )
        _currentMapping->killDeadsAt( to );

    // make mappings conformant if necessary
    if ( to_maybe_nontrivial or ( to->isMergeNode() and not to->isTrivial() ) ) {
        // dst has more than one predecessor
        if ( not to->hasMapping() ) {
            // first jump to dst, use current mapping, must be injective
            _currentMapping->makeInjective();
            to->setMapping( _currentMapping );
        } else {
            // not the first mapping => make mapping conformant
            _currentMapping->makeConformant( to->mapping() );
        }
    } else {
        // dst has exactly one predecessor => use current mapping
        st_assert( not to->hasMapping(), "more than one predecessor?" );
        to->setMapping( _currentMapping );
    }
    _masm->jmp( to->_label );
    setMapping( nullptr );
}


void CodeGenerator::jcc( Assembler::Condition cc, Node *from, Node *to, bool to_maybe_nontrivial ) {
    // make mappings conformant if necessary
    if ( to_maybe_nontrivial or ( to->isMergeNode() and not to->isTrivial() ) ) {
        // dst has more than one predecessor
        if ( not to->hasMapping() ) {
            // first jump to dst, use current mapping, must be injective
            _currentMapping->makeInjective(); // may generate code => must be applied to current mapping
            PseudoRegisterMapping *copy = new PseudoRegisterMapping( _currentMapping );
            // eliminate PseudoRegisters that are not alive anymore at dst
            if ( isLiveRangeBoundary( from, to ) )
                copy->killDeadsAt( to );
            to->setMapping( copy );
            _masm->jcc( cc, to->_label );
        } else {
            // not the first mapping
            PseudoRegisterMapping *copy = new PseudoRegisterMapping( _currentMapping );
            if ( isLiveRangeBoundary( from, to ) )
                copy->killDeadsAt( to );
            if ( copy->isConformant( to->mapping() ) ) {
                // everythink ok, simply jump (must use a copy with dead PseudoRegisters removed for comparison)
                _masm->jcc( cc, to->_label );
            } else {
                // must make mappings conformant, use stub routine
                _mergeStubs.push( Stub::new_jcc_stub( copy, to, cc ) );
            }
        }
    } else {
        // dst has exactly one predecessor
        st_assert( not to->hasMapping(), "more than one predecessor?" );
        PseudoRegisterMapping *copy = new PseudoRegisterMapping( _currentMapping );
        if ( isLiveRangeBoundary( from, to ) )
            copy->killDeadsAt( to );
        to->setMapping( copy );
        _masm->jcc( cc, to->_label );
    }
}


void CodeGenerator::bindLabel( Node *node ) {
    if ( _currentMapping == nullptr ) {
        // continue with mapping at node, live ranges already adjusted
        st_assert( node->hasMapping(), "must have a mapping" );
        setMapping( node->mapping() );
    } else {
        // current mapping exists
        if ( not node->hasMapping() ) {
            // node is approached the first time
            if ( node->isMergeNode() and not node->isTrivial() ) {
                // more than one predecessor => store injective version of current mapping at node
                // (if only one predecessor => simply continue to use the current mapping)
                if ( _previousNode not_eq nullptr and isLiveRangeBoundary( _previousNode, node ) )
                    _currentMapping->killDeadsAt( node );
                _currentMapping->makeInjective();
                node->setMapping( _currentMapping );
            }
        } else {
            // merge current mapping with node mapping
            if ( _previousNode not_eq nullptr and isLiveRangeBoundary( _previousNode, node ) )
                _currentMapping->killDeadsAt( node );
            _currentMapping->makeConformant( node->mapping() );
            setMapping( node->mapping() );
        }
    }
    st_assert( _currentMapping not_eq nullptr, "must have a mapping" );
    _masm->bind( node->_label );
}


void CodeGenerator::inlineCache( Node *call, MergeNode *nlrTestPoint, std::int32_t flags ) {
    st_assert( _currentMapping not_eq nullptr, "mapping must exist" );
    st_assert( call->scope() == nlrTestPoint->scope(), "should be in the same scope" );
    // make mappings conformant if necessary
    if ( nlrTestPoint->isMergeNode() and not nlrTestPoint->isTrivial() ) {
        // dst has more than one predecessor
        if ( not nlrTestPoint->hasMapping() ) {
            // first jump to dst, use current mapping, must be injective
            PseudoRegisterMapping *copy = new PseudoRegisterMapping( _currentMapping );
            if ( isLiveRangeBoundary( call, nlrTestPoint ) )
                copy->killDeadsAt( nlrTestPoint );
            st_assert( _currentMapping->isInjective(), "must be injective" );
            copy->acquireNonLocalReturnRegisters();
            nlrTestPoint->setMapping( copy );
            _masm->ic_info( nlrTestPoint->_label, flags );
        } else {
            // not the first mapping
            PseudoRegisterMapping *copy = new PseudoRegisterMapping( _currentMapping );
            if ( isLiveRangeBoundary( call, nlrTestPoint ) )
                copy->killDeadsAt( nlrTestPoint );
            copy->acquireNonLocalReturnRegisters();
            if ( copy->isConformant( nlrTestPoint->mapping() ) ) {
                // everything ok, simply jump (must use a copy with dead PseudoRegisters removed for comparison)
                _masm->ic_info( nlrTestPoint->_label, flags );
            } else {
                // must make mappings conformant, use stub routine
                _mergeStubs.push( Stub::new_NonLocalReturn_stub( copy, nlrTestPoint, flags ) );
            }
        }
    } else {
        // dst has exactly one predecessor
        st_assert( not nlrTestPoint->hasMapping(), "more than one predecessor?" );
        PseudoRegisterMapping *copy = new PseudoRegisterMapping( _currentMapping );
        if ( isLiveRangeBoundary( call, nlrTestPoint ) )
            copy->killDeadsAt( nlrTestPoint );
        copy->acquireNonLocalReturnRegisters();
        nlrTestPoint->setMapping( copy );
        _masm->ic_info( nlrTestPoint->_label, flags );
    }
}


void CodeGenerator::generateMergeStubs() {
    const char *start_pc = _masm->pc();

    while ( _mergeStubs.nonEmpty() )
        _mergeStubs.pop()->generateMergeStub();

    if ( PrintCodeGeneration and _masm->pc() > start_pc ) {
        spdlog::info( "---" );
        spdlog::info( "fixup merge stubs" );
        _masm->code()->decode();
    }
}


// Code generation for statistical information on nativeMethods

const char *CodeGenerator::nativeMethodAddress() const {
    // hack to compute hypothetical NativeMethod address
    // should be fixed at some point
    return (const char *) ( ( (NativeMethod *) ( _masm->code()->code_begin() ) ) - 1 );
}


void CodeGenerator::incrementInvocationCounter() {
    // Generates code to increment the NativeMethod execution counter
    const char *addr = nativeMethodAddress() + NativeMethod::invocationCountOffset();
    _masm->incl( Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ) );
}


// Initialization / Finalization

void CodeGenerator::initialize( InlinedScope *scope ) {
    // This routine is called at the very beginning of code generation
    // for one NativeMethod (after creation of the CodeGenerator of course).
    // It sets up the initial mapping and thus makes sure that the correct
    // debugging information is written out before the code generation for
    // the PrologueNode.

    // setup arguments
//    std::int32_t       i;
    for ( std::int32_t i = 0; i < scope->nofArguments(); i++ ) {
        _currentMapping->mapToArgument( scope->argument( i )->pseudoRegister(), i );
    }

    // setup temporaries (finalize() generates initialization code)
    for ( std::int32_t i = 0; i < scope->nofTemporaries(); i++ ) {
        _currentMapping->mapToTemporary( scope->temporary( i )->pseudoRegister(), i );
    }

    // setup receiver
    _currentMapping->mapToRegister( scope->self()->pseudoRegister(), self_reg );
}


void CodeGenerator::finalize( InlinedScope *scope ) {
    static_cast<void>(scope); // unused

    // first generate stubs if there are any
    generateMergeStubs();

    // patch 'initialize locals' code
    std::int32_t n          = maxNofStackTmps();
    std::int32_t frame_size = 2 + n;    // return address, old ebp + stack temps
    // make sure frame is big enough for deoptimization
    if ( frame_size < minimum_size_for_deoptimized_frame ) {
        // add the difference to
        n += minimum_size_for_deoptimized_frame - frame_size;
    }

    Assembler masm( _pushCode );
    if ( _pushCode->code_begin() + n <= _pushCode->code_limit() ) {
        while ( n-- > 0 )
            masm.pushl( _nilReg );
    } else {
        masm.jmp( _masm->pc(), RelocationInformation::RelocationType::none );
        while ( n-- > 0 )
            _masm->pushl( _nilReg );
        _masm->jmp( _pushCode->code_limit(), RelocationInformation::RelocationType::none );
    }

    // store nofCompilations at end of code for easier debugging
    if ( CompilerDebug )
        _masm->movl( eax, compilationCount );

    if ( PrintCodeGeneration ) {
        spdlog::info( "---" );
        spdlog::info( "merge stubs" );
        _masm->code()->decode();
        spdlog::info( "---" );
    }
}


void CodeGenerator::finalize2( InlinedScope *scope ) {

    // This routine is called at the very end of code generation for one NativeMethod; it provides the entry points & sets up the stack frame
    // (i.e., this is the first code executed when entering a NativeMethod). Note: _currentMapping is not used since this is one fixed code pattern.

    // first generate stubs if there are any
    generateMergeStubs();

    // set unverified entry point
    _masm->align( OOP_SIZE );
    theCompiler->set_entry_point_offset( _masm->offset() );

    // verify receiver
    if ( scope->isMethodScope() ) {
        // check class
        KlassOop klass = scope->selfKlass();
        if ( klass == smiKlassObject ) {
            // receiver must be a smi_t, check smi_t tag only
            _masm->testl( self_reg, MEMOOP_TAG );            // testl instead of test => no alignment nop's needed later
            _masm->jcc( Assembler::Condition::notZero, CompiledInlineCache::normalLookupRoutine() );
        } else {
            st_assert( self_reg not_eq temp1, "choose another register" );
            _masm->testl( self_reg, MEMOOP_TAG );            // testl instead of test => no alignment nop's needed later
            _masm->jcc( Assembler::Condition::zero, CompiledInlineCache::normalLookupRoutine() );
            _masm->cmpl( Address( self_reg, MemOopDescriptor::klass_byte_offset() ), klass );
            _masm->jcc( Assembler::Condition::notEqual, CompiledInlineCache::normalLookupRoutine() );
        }
    } else {
        // If this is a block method and we expect a context then the incoming context chain must be checked.
        // The context chain may contain a deoptimized contextOop. (see StubRoutines::verify_context_chain for details)
        if ( scope->method()->block_info() == MethodOopDescriptor::expects_context ) {
//            const bool use_fast_check = false;
            _masm->call( StubRoutines::verify_context_chain(), RelocationInformation::RelocationType::runtime_call_type );
        }
    }

    // set verified entry point (for callers who know the receiver is correct)
    _masm->align( OOP_SIZE );
    theCompiler->set_verified_entry_point_offset( _masm->offset() );

    // build stack frame & initialize locals
    _masm->enter();
    std::int32_t n          = maxNofStackTmps();
    std::int32_t frame_size = 2 + n;    // return address, old ebp + stack temps
    // make sure frame is big enough for deoptimization
    if ( frame_size < minimum_size_for_deoptimized_frame ) {
        // add the difference to
        n += minimum_size_for_deoptimized_frame - frame_size;
    }
    if ( n == 1 ) {
        _masm->pushl( nilObject );
    } else if ( n > 1 ) {
        _masm->movl( temp1, nilObject );
        while ( n-- > 0 ) _masm->pushl( temp1 );
    }

    // increment invocation counter & check for overflow (trigger recompilation)
    Label recompile_stub_call;
    if ( RecompilationPolicy::needRecompileCounter( theCompiler ) ) {
        const char *addr = nativeMethodAddress() + NativeMethod::invocationCountOffset();
        _masm->movl( temp1, Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ) );
        _masm->incl( temp1 );
        _masm->cmpl( temp1, theCompiler->get_invocation_counter_limit() );
        _masm->movl( Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ), temp1 );
        _masm->jcc( Assembler::Condition::greaterEqual, recompile_stub_call );
        //
        // need to fix this:
        // 1. put call to recompiler at end (otherwise we cannot provide debugging info easily)
        // 2. check if everything is still ok (e.g. does the recompiler call ever return? if not, no jump needed)
        // 3. check recompiler call stub routine (should not setup stack frame because registers cannot be seen!) - seems to be fixed
    }

    // jump to start of code

    // call to recompiler - if the NativeMethod turns zombie, this will be overwritten by a call to the zombie handler
    // (see also comment in NativeMethod)
    _masm->bind( recompile_stub_call );
    // write debug info
    theCompiler->set_special_handler_call_offset( _masm->offset() );
    _masm->call( StubRoutines::recompile_stub_entry(), RelocationInformation::RelocationType::runtime_call_type );


    // store nofCompilations at end of code for easier debugging
    if ( CompilerDebug ) _masm->movl( eax, _nofCompilations );

    if ( PrintCodeGeneration ) {
        spdlog::info( "---" );
        spdlog::info( "entry point" );
        _masm->code()->decode();
        spdlog::info( "---" );
    }
}


void CodeGenerator::zapContext( PseudoRegister *context ) {
    _masm->movl( Address( use( context ), ContextOopDescriptor::parent_byte_offset() ), 0 );
}


void CodeGenerator::storeCheck( Register obj ) {
    // Does a store check for the Oop in register obj.
    //
    // Note: Could be optimized by hardwiring the byte map base address in the code - however relocation would be necessary whenever the base changes.
    // Advantage: only one instead of two instructions.
    Temporary base( _currentMapping );
    Temporary indx( _currentMapping );
    Label     no_store;
    _masm->cmpl( obj, (std::int32_t) Universe::new_gen.boundary() );                  // assumes boundary between new_gen and old_gen is unchanging
    _masm->jcc( Assembler::Condition::less, no_store );                                // avoid marking dirty if target is a new object
    _masm->movl( base.reg(), Address( std::int32_t( &byte_map_base ), RelocationInformation::RelocationType::external_word_type ) );
    _masm->movl( indx.reg(), obj );                        // do not destroy obj (a pseudoRegister may be mapped to it)
    _masm->shrl( indx.reg(), card_shift );                    // divide obj by card_size
    _masm->movb( Address( base.reg(), indx.reg(), Address::ScaleFactor::times_1 ), 0 );    // clear entry
    _masm->bind( no_store );
}


void CodeGenerator::assign( PseudoRegister *dst, PseudoRegister *src, bool needsStoreCheck ) {
    PseudoRegisterLocker lock( src );        // make sure src stays in register if it's in a register
    enum {
        IS_CONST,       //
        IS_LOADED,      //
        IS_MAPPED,      //
        IS_UNDEFINED    //
    }                    state = IS_UNDEFINED;

    Oop            value{};               // valid if state == is_const
    Register       reg;                 // valid if state == is_loaded
    PseudoRegister *pseudoRegister{ nullptr };    // valid if state == is_mapped

    {
        Temporary t1( _currentMapping, NonLocalReturn_result_reg );
        if ( t1.reg() not_eq NonLocalReturn_result_reg ) {
            reg = t1.reg();
        } else {
            Temporary t2( _currentMapping );
            reg = t2.reg();
        }
    }

    Temporary t( _currentMapping, reg );
    st_assert( reg not_eq NonLocalReturn_result_reg, "fix this" );
    st_assert( t.reg() == reg, "should be the same" );

    // get/load source
    if ( src->isConstPseudoRegister() ) {
        value = ( (ConstPseudoRegister *) src )->constant;
        state = IS_CONST;
    } else if ( src->_location == Location::RESULT_OF_NON_LOCAL_RETURN ) {
        _currentMapping->mapToRegister( src, NonLocalReturn_result_reg );
        pseudoRegister = src;
        state          = IS_MAPPED;
    } else if ( src->_location.isContextLocation() ) {
        PseudoRegister *context = theCompiler->contextList->at( src->_location.contextNo() )->context();
        Address        addr     = Address( use( context ), Mapping::contextOffset( src->_location.tempNo() ) );
        _masm->movl( reg, addr );
        state = IS_LOADED;
    } else {
        st_assert( not src->_location.isSpecialLocation(), "what's this?" );
        pseudoRegister = src;
        state          = IS_MAPPED;
    }

    // map/store to dest
    if ( dst->_location == Location::TOP_OF_STACK ) {
        switch ( state ) {
            case IS_CONST:
                _masm->pushl( value );
                break;
            case IS_LOADED:
                _masm->pushl( reg );
                break;
            case IS_MAPPED:
                _masm->pushl( use( pseudoRegister ) );
                break;
            default       : ShouldNotReachHere();
        }
    } else if ( dst->_location.isContextLocation() ) {
        PseudoRegister       *context = theCompiler->contextList->at( dst->_location.contextNo() )->context();
        PseudoRegisterLocker lock( context );
        Address              addr     = Address( use( context ), Mapping::contextOffset( dst->_location.tempNo() ) );
        switch ( state ) {
            case IS_CONST:
                _masm->movl( addr, value );
                break;
            case IS_LOADED:
                _masm->movl( addr, reg );
                break;
            case IS_MAPPED:
                _masm->movl( addr, use( pseudoRegister ) );
                break;
            default       : ShouldNotReachHere();
        }
        if ( needsStoreCheck )
            storeCheck( use( context ) );
    } else {
        st_assert( not dst->_location.isSpecialLocation(), "what's this?" );
        switch ( state ) {
            case IS_CONST:
                _masm->movl( def( dst ), value );
                break;
            case IS_LOADED:
                _masm->movl( def( dst ), reg );
                break;
            case IS_MAPPED:
                _currentMapping->move( dst, pseudoRegister );
                break;
            default       : ShouldNotReachHere();
        }
    }
}


// Debugging

static std::int32_t _callDepth{ 0 };
static std::int32_t _numberOfCalls{ 0 };
static std::int32_t _numberOfReturns{ 0 };
static std::int32_t _numberOfNonLocalReturns{ 0 };


void CodeGenerator::indent() {
    const std::int32_t maxIndent{ 40 };
    if ( _callDepth <= maxIndent ) {
        _console->print( "%*s", _callDepth, "" );
    } else {
        _console->print( "%*d: ", maxIndent - 2, _callDepth );
    }
}


const char *CodeGenerator::nativeMethodName() {
    DeltaVirtualFrame *f = DeltaProcess::active()->last_delta_vframe();
    return f->method()->selector()->as_string();
}


void CodeGenerator::verifyObject( Oop obj ) {

    if ( not obj->is_smi() and not obj->is_mem() ) {
        st_fatal( "should be an ordinary Oop" );
    }

    KlassOop klass = obj->klass();
    if ( klass == nullptr or not klass->is_mem() ) {
        st_fatal( "should be an ordinary MemOop" );
    }

    if ( obj->is_block() ) {
        BlockClosureOop( obj )->verify();
    }

}


void CodeGenerator::verifyContext( Oop obj ) {
    if ( obj->is_mark() ) {
        error( "context should never be mark" );
    }

    if ( not Universe::is_heap( (Oop *) obj ) ) {
        error( "context outside of heap" );
    }

    if ( not obj->is_context() ) {
        error( "should be a context" );
    }

    Oop c = (Oop) ( ContextOop( obj )->parent() );
    if ( c->is_mem() ) {
        verifyContext( c );
    }

}


void CodeGenerator::verifyArguments( Oop recv, Oop *ebp, std::int32_t nofArgs ) {
    bool         print_args_long = true;
    ResourceMark resourceMark;
    _numberOfCalls++;
    _callDepth++;
    if ( TraceCalls ) {
        ResourceMark resourceMark;
        indent();
        _console->print( "( %s %s ", recv->print_value_string(), nativeMethodName() );
    }

    verifyObject( recv );
    std::int32_t i    = nofArgs;
    Oop          *arg = ebp + ( nofArgs + 2 );
    while ( i-- > 0 ) {
        arg--;
        verifyObject( *arg );
        if ( TraceCalls ) {
            ResourceMark resourceMark;
            if ( print_args_long or ( *arg )->is_smi() ) {
                _console->print( "%s ", ( *arg )->print_value_string() );
            } else {
                _console->print( "0x{0:x} ", *arg );
            }
        }
    }

    if ( TraceCalls ) {
        _console->cr();
    }

    if ( VerifyDebugInfo ) {
        DeltaVirtualFrame *f = DeltaProcess::active()->last_delta_vframe();
        while ( f not_eq nullptr ) {
            f->verify_debug_info();
            f = f->sender_delta_frame();
        }
    }
}


void CodeGenerator::verifyReturn( Oop result ) {
    _numberOfReturns++;
    result->verify();
    if ( TraceCalls ) {
        ResourceMark resourceMark;
        indent();
        spdlog::info( ") {} -> {}", nativeMethodName(), result->print_value_string() );
    }
    _callDepth--;
}


void CodeGenerator::verifyNonLocalReturn( const char *fp, const char *nlrFrame, std::int32_t nlrScopeID, Oop result ) {
    static_cast<void>(nlrScopeID); // unused

    _numberOfNonLocalReturns++;
    spdlog::info( "verifyNonLocalReturn(0x{0:x}, 0x{0:x}, %d, 0x{0:x})", static_cast<const void *>( fp ), static_cast<const void *>( nlrFrame ), static_cast<const void *>( result ) );
    if ( nlrFrame <= fp )
        spdlog::error( "NonLocalReturn went too far: 0x{0:x} <= 0x{0:x}", static_cast<const void *>( nlrFrame ), static_cast<const void *>( fp ) );

    // treat >99 scopes as likely error -- might actually be ok
//  if (nlrScopeID < 0 or nlrScopeID > 99) error("illegal NonLocalReturn scope ID 0x{0:x}", nlrScopeID);
    if ( result->is_mark() ) {
        spdlog::error( "NonLocalReturn result is a markOop" );
    }
    result->verify();

    if ( TraceCalls ) {
        ResourceMark resourceMark;
        indent();
        spdlog::info( ") {} ^ {}", nativeMethodName(), result->print_value_string() );
    }
    _callDepth--;
}


void CodeGenerator::callVerifyObject( Register obj ) {
    // generates transparent check code which verifies that obj is
    // a legal Oop and halts if not - for debugging purposes only
    if ( not VerifyCode ) {
        spdlog::warn( ": verifyObject should not be called" );
    }
    _masm->pushad();
    _masm->call_C( (const char *) CodeGenerator::verifyObject, obj );
    _masm->popad();
}


void CodeGenerator::callVerifyContext( Register context ) {
    // generates transparent check code which verifies that context is
    // a legal context and halts if not - for debugging purposes only
    if ( not VerifyCode ) {
        spdlog::warn( ": verifyContext should not be called" );
    }
    _masm->pushad();
    _masm->call_C( (const char *) CodeGenerator::verifyContext, context );
    _masm->popad();
}


void CodeGenerator::callVerifyArguments( Register recv, std::int32_t nofArgs ) {
    // generates transparent check code which verifies that all arguments
    // are legal oops and halts if not - for debugging purposes only
    if ( not VerifyCode and not TraceCalls and not TraceResults ) {
        spdlog::warn( ": performance bug: verifyArguments should not be called" );
    }
    st_assert( recv not_eq temp1, "use another temporary register" );
    _masm->pushad();
    _masm->movl( temp1, nofArgs );
    _masm->call_C( (const char *) CodeGenerator::verifyArguments, recv, ebp, temp1 );
    _masm->popad();
}


void CodeGenerator::callVerifyReturn() {
    // generates transparent check code which verifies that result contains
    // a legal Oop and halts if not - for debugging purposes only
    if ( not VerifyCode and not TraceCalls and not TraceResults ) {
        spdlog::warn( ": verifyReturn should not be called" );
    }
    _masm->pushad();
    _masm->call_C( (const char *) CodeGenerator::verifyReturn, result_reg );
    _masm->popad();
}


void CodeGenerator::callVerifyNonLocalReturn() {
    // generates transparent check code which verifies NonLocalReturn check & continuation
    if ( not VerifyCode and not TraceCalls and not TraceResults ) {
        spdlog::warn( ": verifyNonLocalReturn should not be called" );
    }
    _masm->pushad();
    _masm->call_C( (const char *) CodeGenerator::verifyNonLocalReturn, ebp, NonLocalReturn_home_reg, NonLocalReturn_homeId_reg, NonLocalReturn_result_reg );
    _masm->popad();
}


// Basic blocks

static bool bb_needs_jump;

// true if basic block needs a jump at the end to its successor, false otherwise
// Note: most gen() nodes with more than one successor are implemented such that
//       next() is the fall-through case. If that's not the case, an extra jump
//       has to be generated (via endOfBasicBlock()). However, some of the nodes
//       do explicit jumps to all successors to accomodate for arbitrary node
//       reordering, in which case they may set the flag to false (it is auto-
//       matically set to true for each node).
//
// This flag should go away at soon as all node with more than one exit are
// implemented correctly (i.e., do all the jumping themselves).


void CodeGenerator::beginOfBasicBlock( Node *node ) {
    if ( PrintCodeGeneration and WizardMode ) {
        spdlog::info( "--- begin of basic block (N{}) ---", node->id() );
    }
    bindLabel( node );
}


void CodeGenerator::endOfBasicBlock( Node *node ) {
    if ( bb_needs_jump and node->next() not_eq nullptr ) {
        Node *from = node;
        Node *to   = node->next();
        if ( PrintCodeGeneration ) {
            spdlog::info( "branch from N{} to N{}", from->id(), to->id() );
            if ( PrintPseudoRegisterMapping )
                _currentMapping->print();
        }
        jmp( from, to );
        _previousNode = nullptr;
        if ( PrintCodeGeneration )
            _masm->code()->decode();
    }

    if ( PrintCodeGeneration and WizardMode ) {
        spdlog::info( "--- end of basic block (N{}) ---", node->id() );
    }
}


void CodeGenerator::updateDebuggingInfo( Node *node ) {
    ScopeDescriptorRecorder *rec      = theCompiler->scopeDescRecorder();
    std::int32_t            pc_offset = assembler()->offset();
    rec->addProgramCounterDescriptor( pc_offset, node->scope()->getScopeInfo(), node->byteCodeIndex() );
    _debugInfoWriter->write_debug_info( _currentMapping, pc_offset );
}


// For all nodes
void CodeGenerator::beginOfNode( Node *node ) {

    st_assert( _currentMapping not_eq nullptr, "must have a valid mapping" );

    // adjust mapping to liveness of PseudoRegisters
    if ( _previousNode not_eq nullptr and isLiveRangeBoundary( _previousNode, node ) ) {
        _currentMapping->killDeadsAt( node );
    }
    _currentMapping->cleanupContextReferences();

    // adjust debugging information if desired (e.g., when using disassembler with full symbolic support)
    if ( GenerateFullDebugInfo ) {
        updateDebuggingInfo( node );
    }

    // debugging
    if ( PrintCodeGeneration ) {
        spdlog::info( "begin node [{}]", node->id() );
        node->print();
        spdlog::info( "end node [{}], byteCodeIndex [{}]", node->id(), node->byteCodeIndex() );
        if ( PrintPseudoRegisterMapping ) {
            _currentMapping->print();
        }
    }

    //
    bb_needs_jump = true;
}


void CodeGenerator::endOfNode( Node *node ) {
    if ( PrintCodeGeneration ) {
        _masm->code()->decode();
    }

    // if _currentMapping == nullptr there's no previous node & the next node will be
    // reached via a jump and it's mapping is already set up the right way
    // (i.e., no PseudoRegister killing required => set _previousNode to nullptr)
    _previousNode = _currentMapping == nullptr ? nullptr : node;
}


// Individual nodes
extern "C" const char *active_stack_limit();
extern "C" void check_stack_overflow();


void CodeGenerator::aPrologueNode( PrologueNode *node ) {
    // set unverified entry point
    _masm->align( OOP_SIZE );
    theCompiler->set_entry_point_offset( _masm->offset() );

    // verify receiver
    InlinedScope         *scope = node->scope();
    PseudoRegister       *recv  = scope->self()->pseudoRegister();
    PseudoRegisterLocker lock( recv );
    if ( scope->isMethodScope() ) {
        // check class
        KlassOop klass = scope->selfKlass();
        if ( klass == smiKlassObject ) {
            // receiver must be a smi_t, check smi_t tag only
            _masm->test( use( recv ), MEMOOP_TAG );
            _masm->jcc( Assembler::Condition::notZero, CompiledInlineCache::normalLookupRoutine() );
        } else {
            _masm->test( use( recv ), MEMOOP_TAG );
            _masm->jcc( Assembler::Condition::zero, CompiledInlineCache::normalLookupRoutine() );
            _masm->cmpl( Address( use( recv ), MemOopDescriptor::klass_byte_offset() ), klass );
            _masm->jcc( Assembler::Condition::notEqual, CompiledInlineCache::normalLookupRoutine() );
        }
    } else {
        // If this is a block method and we expect a context then the incoming context chain must be checked.
        // The context chain may contain a deoptimized contextOop. (see StubRoutines::verify_context_chain for details)
        if ( scope->method()->block_info() == MethodOopDescriptor::expects_context ) {
            const bool use_fast_check = false;
            if ( use_fast_check ) {
                // look in old backend for this code
                Unimplemented();
            } else {
                _masm->call( StubRoutines::verify_context_chain(), RelocationInformation::RelocationType::runtime_call_type );
            }
        }
    }

    // set verified entry point (for callers who know the receiver is correct)
    _masm->align( OOP_SIZE );
    theCompiler->set_verified_entry_point_offset( _masm->offset() );

    // build stack frame & initialize locals
    _masm->enter();
    {
        Temporary t( _currentMapping );
        _masm->movl( t.reg(), Universe::nilObject() );
        _nilReg = t.reg();
        const char   *beg = _masm->pc();
        std::int32_t i    = 10;
        while ( i-- > 0 )
            _masm->nop();
        const char *end = _masm->pc();
        _pushCode = new CodeBuffer( beg, end - beg );
    }

    if ( scope->isBlockScope() ) {
        // initialize context for blocks; recv is block closure => get context out of it
        // and store it in self & temp0 (which holds the context in the interpreter model).
        // Note: temp0 has been mapped to the stack when setting up temporaries.
        st_assert( scope->context() == scope->temporary( 0 )->pseudoRegister(), "should be the same" );
        Register reg = use( recv );
        _masm->movl( def( recv ), Address( reg, BlockClosureOopDescriptor::context_byte_offset() ) );
        assign( scope->context(), recv );
    }
    // debugging
    if ( VerifyCode or VerifyDebugInfo or TraceCalls )
        callVerifyArguments( use( recv ), scope->method()->number_of_arguments() );

    // increment invocation counter & check for overflow (trigger recompilation)
    Label recompile_stub_call;
    if ( RecompilationPolicy::needRecompileCounter( theCompiler ) ) {
        const char *addr = nativeMethodAddress() + NativeMethod::invocationCountOffset();
        _masm->movl( temp1, Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ) );
        _masm->incl( temp1 );
        _masm->cmpl( temp1, theCompiler->get_invocation_counter_limit() );
        _masm->movl( Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ), temp1 );
        _masm->jcc( Assembler::Condition::greaterEqual, recompile_stub_call );

        // need to fix this:
        // 1. put call to recompiler at end (otherwise we cannot provide debugging info easily)
        // 2. check if everything is still ok (e.g. does the recompiler call ever return? if not, no jump needed)
        // 3. check recompiler call stub routine (should not setup stack frame because registers cannot be seen!) - seems to be fixed
    }
    Label start;
    _masm->jmp( start );

    Label handle_stack_overflow, continue_after_stack_overflow;
    _masm->bind( handle_stack_overflow );
    _masm->call_C( (const char *) &check_stack_overflow, RelocationInformation::RelocationType::runtime_call_type );
    _masm->jmp( continue_after_stack_overflow );

    // call to recompiler - if the NativeMethod turns zombie, this will be overwritten
    // by a call to the zombie handler (see also comment in NativeMethod)
    _masm->bind( recompile_stub_call );
    updateDebuggingInfo( node );
    theCompiler->set_special_handler_call_offset( theMacroAssembler->offset() );
    _masm->call( StubRoutines::recompile_stub_entry(), RelocationInformation::RelocationType::runtime_call_type );

    _masm->bind( start );
    _masm->cmpl( esp, Address( std::int32_t( active_stack_limit() ), RelocationInformation::RelocationType::external_word_type ) );
    _masm->jcc( Assembler::Condition::less, handle_stack_overflow );
    _masm->bind( continue_after_stack_overflow );
}


void CodeGenerator::aLoadIntNode( LoadIntNode *node ) {
    _masm->movl( def( node->dst() ), node->value() );
}


void CodeGenerator::aLoadOffsetNode( LoadOffsetNode *node ) {
    PseudoRegisterLocker lock( node->base(), node->dst() );
    _masm->movl( def( node->dst() ), Address( use( node->base() ), byteOffset( node->_offset ) ) );
}


std::int32_t CodeGenerator::byteOffset( std::int32_t offset ) {
    // Computes the byte offset from the beginning of an Oop
    st_assert( offset >= 0, "wrong offset" );
    return offset * OOP_SIZE - MEMOOP_TAG;
}


void CodeGenerator::uplevelBase( PseudoRegister *startContext, std::int32_t nofLevels, Register base ) {
    // Compute uplevel base into register base; nofLevels is number of indirections (0 = in this context).
    _masm->movl( base, use( startContext ) );
    if ( VerifyCode )
        callVerifyContext( base );
    while ( nofLevels-- > 0 )
        _masm->movl( base, Address( base, ContextOopDescriptor::parent_byte_offset() ) );
}


void CodeGenerator::aLoadUplevelNode( LoadUplevelNode *node ) {
    PseudoRegisterLocker lock( node->context0() );
    Temporary            base( _currentMapping );
    uplevelBase( node->context0(), node->nofLevels(), base.reg() );
    Register dst = def( node->dst() );
    _masm->movl( dst, Address( base.reg(), byteOffset( node->offset() ) ) );
    if ( VerifyCode )
        callVerifyObject( dst );
}


void CodeGenerator::anAssignNode( AssignNode *node ) {
    assign( node->dst(), node->src() );
}


void CodeGenerator::aStoreOffsetNode( StoreOffsetNode *node ) {
    PseudoRegisterLocker lock( node->base(), node->src() );
    Register             base = use( node->base() );
    _masm->movl( Address( base, byteOffset( node->offset() ) ), use( node->src() ) );
    if ( node->needsStoreCheck() )
        storeCheck( base );
}


void CodeGenerator::aStoreUplevelNode( StoreUplevelNode *node ) {
    PseudoRegisterLocker lock( node->context0(), node->src() );
    Temporary            base( _currentMapping );
    uplevelBase( node->context0(), node->nofLevels(), base.reg() );
    _masm->movl( Address( base.reg(), byteOffset( node->offset() ) ), use( node->src() ) );
    if ( node->needsStoreCheck() )
        storeCheck( base.reg() );
}


void CodeGenerator::moveConstant( ArithOpCode op, PseudoRegister *&x, PseudoRegister *&y, bool &x_attr, bool &y_attr ) {
    if ( x->isConstPseudoRegister() and ArithOpIsCommutative[ static_cast<std::int32_t>( op ) ] ) {
        PseudoRegister *t1 = x;
        x = y;
        y = t1;
        bool t2 = x_attr;
        x_attr = y_attr;
        y_attr = t2;
    }
}


void CodeGenerator::arithRROp( ArithOpCode op, Register x, Register y ) { // x := x op y
    st_assert( INTEGER_TAG == 0, "check this code" );
    switch ( op ) {
        case ArithOpCode::TestArithOp:
            _masm->testl( x, y );
            break;
        case ArithOpCode::tAddArithOp: // fall through
        case ArithOpCode::AddArithOp:
            _masm->addl( x, y );
            break;
        case ArithOpCode::tSubArithOp: // fall through
        case ArithOpCode::SubArithOp:
            _masm->subl( x, y );
            break;
        case ArithOpCode::tMulArithOp:
            _masm->sarl( x, TAG_SIZE );
        case ArithOpCode::MulArithOp:
            _masm->imull( x, y );
            break;
        case ArithOpCode::tDivArithOp: // fall through
        case ArithOpCode::DivArithOp: Unimplemented();
            break;
        case ArithOpCode::tModArithOp: // fall through
        case ArithOpCode::ModArithOp: Unimplemented();
            break;
        case ArithOpCode::tAndArithOp: // fall through
        case ArithOpCode::AndArithOp:
            _masm->andl( x, y );
            break;
        case ArithOpCode::tOrArithOp: // fall through
        case ArithOpCode::OrArithOp:
            _masm->orl( x, y );
            break;
        case ArithOpCode::tXOrArithOp: // fall through
        case ArithOpCode::XOrArithOp:
            _masm->xorl( x, y );
            break;
        case ArithOpCode::tShiftArithOp: Unimplemented();
        case ArithOpCode::ShiftArithOp: Unimplemented();
        case ArithOpCode::tCmpArithOp: // fall through
        case ArithOpCode::CmpArithOp:
            _masm->cmpl( x, y );
            break;
        default: ShouldNotReachHere();
    }
}


void CodeGenerator::arithRCOp( ArithOpCode op, Register x, std::int32_t y ) { // x := x op y

    st_assert( INTEGER_TAG == 0, "check this code" );
    switch ( op ) {
        case ArithOpCode::TestArithOp:
            _masm->testl( x, y );
            break;

        case ArithOpCode::tAddArithOp: // fall through
        case ArithOpCode::AddArithOp:
            if ( y == 0 ) {
                spdlog::warn( "code generated to add 0 (no load required)" );
            } else {
                _masm->addl( x, y );
            }
            break;

        case ArithOpCode::tSubArithOp: // fall through
        case ArithOpCode::SubArithOp:
            if ( y == 0 ) {
                spdlog::warn( "code generated to subtract 0 (no load required)" );
            } else {
                _masm->subl( x, y );
            }
            break;

        case ArithOpCode::tMulArithOp:
            y = arithmetic_shift_right( y, TAG_SIZE );

        case ArithOpCode::MulArithOp:
            // catch a few trivial cases (since certain optimizations happen
            // after inlining of primitives, these cases cannot be handled in
            // the primitive inliner alone => phase ordering problem).
            // Note that overflow check must still remain possible (i.e.,
            // cannot easily substitute *4 with 2 adds without saving CC).
            switch ( y ) {
                case -1:
                    _masm->negl( x );
                    break;
                case 0:
                    spdlog::warn( "code generated to multiply with 0 (no load required)" );
                    _masm->xorl( x, x );
                    break;
                case 1:
                    spdlog::warn( "code generated to multiply with 1 (no load required)" );
                    // do nothing
                    break;
                case 2:
                    _masm->addl( x, x );
                    break;
                default:
                    _masm->imull( x, x, y );
                    break;
            }
            break;

        case ArithOpCode::tDivArithOp  : // fall through
        case ArithOpCode::DivArithOp  : Unimplemented();
            break;

        case ArithOpCode::tModArithOp  : // fall through
        case ArithOpCode::ModArithOp  : Unimplemented();
            break;

        case ArithOpCode::tAndArithOp  : // fall through
        case ArithOpCode::AndArithOp:
            _masm->andl( x, y );
            break;

        case ArithOpCode::tOrArithOp   : // fall through
        case ArithOpCode::OrArithOp:
            _masm->orl( x, y );
            break;

        case ArithOpCode::tXOrArithOp  : // fall through
        case ArithOpCode::XOrArithOp:
            _masm->xorl( x, y );
            break;

        case ArithOpCode::tShiftArithOp:
            if ( y < 0 ) {
                // shift right
                std::int32_t shift_count = ( ( -y ) >> TAG_SIZE ) % 32;
                _masm->sarl( x, shift_count );
                _masm->andl( x, -1 << TAG_SIZE );            // clear Tag bits
            } else if ( y > 0 ) {
                // shift left
                std::int32_t shift_count = ( ( +y ) >> TAG_SIZE ) % 32;
                _masm->shll( x, shift_count );
            }
            break;

        case ArithOpCode::ShiftArithOp: Unimplemented();
        case ArithOpCode::tCmpArithOp: // fall through
        case ArithOpCode::CmpArithOp:
            _masm->cmpl( x, y );
            break;

        default: ShouldNotReachHere();
    }
}


void CodeGenerator::arithROOp( ArithOpCode op, Register x, Oop y ) { // x := x op y
    st_assert( not y->is_smi(), "check this code" );
    switch ( op ) {
        case ArithOpCode::CmpArithOp:
            _masm->cmpl( x, y );
            break;
        default: ShouldNotReachHere();
    }
}


void CodeGenerator::arithRXOp( ArithOpCode op, Register x, Oop y ) { // x := x op y
    if ( y->is_smi() ) {
        arithRCOp( op, x, std::int32_t( y ) );                // y is SMIOop -> needs no relocation info
    } else {
        arithROOp( op, x, y );
    }
}


bool CodeGenerator::producesResult( ArithOpCode op ) {
    return ( op not_eq ArithOpCode::TestArithOp ) and ( op not_eq ArithOpCode::CmpArithOp ) and ( op not_eq ArithOpCode::tCmpArithOp );
}


Register CodeGenerator::targetRegister( ArithOpCode op, PseudoRegister *z, PseudoRegister *x ) {
    st_assert( PseudoRegisterLocker::locks( z ) and PseudoRegisterLocker::locks( x ), "should be locked" );
    Register reg = noreg;
    if ( producesResult( op ) ) {
        // result produced -> use a copy of x as register for z
        Register x_reg = use( x );
        // x is guaranteed to be in a register
        if ( _currentMapping->onStack( x ) ) {
            // x is also on stack -> release register location from mapping and use it as copy
            _currentMapping->killRegister( x );
            reg = _currentMapping->def( z, x_reg );
        } else {
            // pseudoRegister is only in register -> need to allocate a new register & copy it explicitly
            reg = def( z );
            _masm->movl( reg, x_reg );
        }
    } else {
        // no result produced -> can use x directly as register for z
        reg = use( x );
    }
    return reg;
}


void CodeGenerator::anArithRRNode( ArithRRNode *node ) {

    ArithOpCode op = node->op();

    PseudoRegister *z        = node->dst();
    PseudoRegister *x        = node->src();
    PseudoRegister *y        = node->operand();

    bool dummy;
    moveConstant( op, x, y, dummy, dummy );

    PseudoRegisterLocker lock( z, x, y );
    Register             reg = targetRegister( op, z, x );

    if ( y->isConstPseudoRegister() ) {
        arithRXOp( op, reg, ( (ConstPseudoRegister *) y )->constant );
    } else {
        arithRROp( op, reg, use( y ) );
    }
}


void CodeGenerator::anArithRCNode( ArithRCNode *node ) {
    ArithOpCode          op  = node->op();
    PseudoRegister       *z  = node->dst();
    PseudoRegister       *x  = node->src();
    std::int32_t         y   = node->operand();
    PseudoRegisterLocker lock( z, x );
    Register             reg = targetRegister( op, z, x );
    arithRCOp( op, reg, y );
}


void CodeGenerator::aTArithRRNode( TArithRRNode *node ) {
    ArithOpCode    op         = node->op();
    PseudoRegister *z         = node->dst();
    PseudoRegister *x         = node->src();
    PseudoRegister *y         = node->operand();
    bool           x_is_int   = node->arg1IsInt();
    bool           y_is_int   = node->arg2IsInt();
    moveConstant( op, x, y, x_is_int, y_is_int );
    PseudoRegisterLocker lock( z, x, y );
    Register             tags = noreg;
    if ( x_is_int ) {
        if ( y_is_int ) {
            // both x & y are smis => no tag check necessary
        } else {
            // x is smi_t => check y
            tags = use( y );
        }
    } else {
        if ( y_is_int ) {
            // y is smi_t => check x
            tags = use( x );
        } else {
            // check both x & y
            Temporary t( _currentMapping );
            tags = t.reg();
            _masm->movl( tags, use( x ) );
            _masm->orl( tags, use( y ) );
        }
    }
    if ( tags not_eq noreg ) {
        // check tags
        _masm->test( tags, MEMOOP_TAG );
        jcc( Assembler::Condition::notZero, node, node->next( 1 ) );
    }
    Register reg = targetRegister( op, z, x );
    if ( y->isConstPseudoRegister() ) {
        arithRXOp( op, reg, ( (ConstPseudoRegister *) y )->constant );
    } else {
        arithRROp( op, reg, use( y ) );
    }
}


void CodeGenerator::aFloatArithRRNode( FloatArithRRNode *node ) {
    static_cast<void>(node); // unused
    Unimplemented();
}


void CodeGenerator::aFloatUnaryArithNode( FloatUnaryArithNode *node ) {
    static_cast<void>(node); // unused
    Unimplemented();
}


void CodeGenerator::aContextCreateNode( ContextCreateNode *node ) {
    // node->dst() has been pre-allocated (temp0) in the prologue node -> remove it from
    // mapping. Note that in cases where there's an incoming context (which serves as parent (node->src())),
    // node->src() and node->dst() differ because the NodeBuilder allocates a new SinglyAssignedPseudoRegister in this case.
    st_assert( node->src() not_eq node->dst(), "should not be the same" );
    st_assert( node->dst() == node->scope()->context(), "should assign to scope context" );
    _currentMapping->kill( node->dst() );        // kill it so that aPrimNode(node) can map the result to it
    switch ( node->sizeOfContext() ) {
        case 0 : // fall through for now - fix this
        case 1 : // fall through for now - fix this
        case 2 : // fall through for now - fix this
        default:
            _masm->pushl( std::int32_t( smiOopFromValue( node->nofTemps() ) ) );
            aPrimitiveNode( node );
            _masm->addl( esp, OOP_SIZE );    // pop argument, this is not a Pascal call - change this as some point?
            break;
    }

    PseudoRegisterLocker lock( node->dst() );        // once loaded, make sure context stays in register
    Register             context_reg = use( node->dst() );
    if ( node->src() == nullptr ) {
        st_assert( node->scope()->isMethodScope() or node->scope()->method()->block_info() == MethodOopDescriptor::expects_nil, "inconsistency" );
        _masm->movl( Address( context_reg, ContextOopDescriptor::parent_byte_offset() ), nullptr );
        // nullptr for now; the interpreter uses nil. However, some of the
        // context verification code called from compiled code checks for
        // parents that are either a frame pointer, nullptr or a context.
        // This should be unified at some point. (gri 5/9/96)
    } else if ( _currentMapping->isDefined( node->src() ) ) {
        // node->src() holds incoming context as parent and has been defined before
        _masm->movl( Address( context_reg, ContextOopDescriptor::parent_byte_offset() ), use( node->src() ) );
    } else {
        // node->src()->loc is pointing to the current stack frame (method context) and has not been explicitly defined
        st_assert( node->src()->_location == frameLoc, "parent should point to current stack frame" );
        _masm->movl( Address( context_reg, ContextOopDescriptor::parent_byte_offset() ), frame_reg );
    }
    storeCheck( context_reg );
}


void CodeGenerator::aContextInitNode( ContextInitNode *node ) {
    // initialize context temporaries (parent has been initialized in the ContextCreateNode)
    for ( std::int32_t i = node->nofTemps(); i-- > 0; ) {
        PseudoRegister *src = node->initialValue( i )->pseudoRegister();
        PseudoRegister *dst;
        if ( src->isBlockPseudoRegister() ) {
            // Blocks aren't actually assigned (at the PseudoRegister level) so that the inlining info isn't lost.
            if ( node->wasEliminated() ) {
                continue;                // there's no assignment (context was eliminated)
            } else {
                dst = node->contextTemp( i )->pseudoRegister();    // fake destination created by compiler
            }
        } else {
            dst = node->contextTemp( i )->pseudoRegister();
        }
        assign( dst, src, false );
    }
    // NB: no store check necessary (done in ContextCreateNode)
    // init node must follow create immediately (since fields are uninitialized)
    st_assert( node->firstPrev()->isContextCreateNode(), "should be immediatly after a ContextCreateNode" );
}


void CodeGenerator::aContextZapNode( ContextZapNode *node ) {
    if ( not node->isActive() )
        return;
    st_assert( node->scope()->needsContextZapping() and node->src() == node->scope()->context(), "no zapping needed or wrong context" );
    // Make sure these registers are not used within zapContext
    // because they are used for return/non-local return
    // -> allocate them as temporaries for now
    Temporary t1( _currentMapping, NonLocalReturn_result_reg );
    Temporary t2( _currentMapping, NonLocalReturn_home_reg );
    Temporary t3( _currentMapping, NonLocalReturn_homeId_reg );
    // zap context
    _masm->movl( Address( use( node->src() ), ContextOopDescriptor::parent_byte_offset() ), 0 );
    // A better solution should be found here: There should be a mechanism
    // to exclude certain register from beeing taken.
}


void CodeGenerator::copyIntoContexts( BlockCreateNode *node ) {

    //
    // Copy newly created block into all contexts that have a copy.
    // The BlockPseudoRegister has a list of all contexts containing the block.
    // It should be stored into those that are allocated (weren't eliminated) and are in a sender scope.
    //
    // Why not copy into contexts in a sibling scope?  There are two cases:
    //   (1) The sibling scope never created the block(s) that uplevel-access this
    //       block.  The context location still contains 0 but that doesn't matter
    //       because that context location is now inaccessible.
    //   (2) The sibling scope did create these block(s).  In this case, the receiver
    //       must already exist since it was materialized when the first uplevel-
    //       accessing block was created.
    // Urs 4/96
    //

    BlockPseudoRegister       *blk    = node->block();
    GrowableArray<Location *> *copies = blk->contextCopies();
    if ( copies not_eq nullptr ) {
        for ( std::int32_t i = copies->length(); i-- > 0; ) {

            Location       *l                = copies->at( i );
            InlinedScope   *scopeWithContext = theCompiler->scopes->at( l->scopeID() );
            PseudoRegister *r                = scopeWithContext->contextTemporaries()->at( l->tempNo() )->pseudoRegister();

            if ( r->_location == Location::UNALLOCATED_LOCATION )
                continue;      // not uplevel-accessed (eliminated)

            if ( r->isBlockPseudoRegister() )
                continue;          // ditto

            if ( not r->_location.isContextLocation() ) st_fatal( "expected context location" );
            if ( scopeWithContext->isSenderOrSame( node->scope() ) ) {
                assign( r, node->block() );
            }
        }
    }
}


void CodeGenerator::materializeBlock( BlockCreateNode *node ) {
    CompileTimeClosure *closure = node->block()->closure();
    // allocate closure
    _currentMapping->kill( node->dst() );    // kill it so that aPrimNode(node) can map the result to it

    std::int32_t nofArgs             = closure->nofArgs();

    switch ( nofArgs ) {
        case 0 : // fall through for now - fix this
        case 1 : // fall through for now - fix this
        case 2 : // fall through for now - fix this
        default:
            _masm->pushl( std::int32_t( smiOopFromValue( nofArgs ) ) );
            aPrimitiveNode( node );            // Note: primitive calls are called via call_C - also necessary for primitiveValue calls?
            _masm->addl( esp, OOP_SIZE );    // pop argument, this is not a Pascal call - change this at some point?
            break;
    }

    // copy into all contexts that have a copy
    if ( node->block()->isMemoized() )
        copyIntoContexts( node );

    // initialize closure fields
    PseudoRegisterLocker lock( node->block() );    // once loaded, make sure closure stays in register
    Register             closure_reg = use( node->block() );

    // assert(theCompiler->JumpTableID == closure->parent_id(), "NativeMethod id must be the same");
    // fix this: RELOCATION INFORMATION IS NEEDED WHEN MOVING THE JUMPTABLE (Snapshot reading etc.)
    _masm->movl( Address( closure_reg, BlockClosureOopDescriptor::context_byte_offset() ), use( closure->context() ) );
    _masm->movl( Address( closure_reg, BlockClosureOopDescriptor::method_or_entry_byte_offset() ), (std::int32_t) closure->jump_table_entry() );
    storeCheck( closure_reg );
}


void CodeGenerator::aBlockCreateNode( BlockCreateNode *node ) {
    if ( node->block()->closure()->method()->is_clean_block() ) {
        // create the block now (doesn't need to be copied at run-time
        CompileTimeClosure *closure = node->block()->closure();
        BlockClosureOop    blk      = BlockClosureOopDescriptor::create_clean_block( closure->nofArgs(), closure->jump_table_entry() );
        _masm->movl( def( node->dst() ), blk );
    } else if ( node->block()->isMemoized() ) {
        // initialize block variable
        _masm->movl( def( node->dst() ), MemoizedBlockNameDescriptor::uncreatedBlockValue() );
    } else {
        // actually create block
        materializeBlock( node );
    }
}


void CodeGenerator::aBlockMaterializeNode( BlockMaterializeNode *node ) {
    st_assert( node->next() == node->likelySuccessor(), "code pattern is not optimal" );
    if ( node->block()->closure()->method()->is_clean_block() ) {
        // no need to create the block (already exists)
    } else if ( node->block()->isMemoized() ) {
        // materialize block if it is not already materialized
        // (nothing to do in case of non-memoized blocks)
        Register closure_reg = use( node->block() );
        st_assert( MemoizedBlockNameDescriptor::uncreatedBlockValue() == Oop( 0 ), "change the code generation here" );
        _masm->testl( closure_reg, closure_reg );
        jcc( Assembler::Condition::notZero, node, node->next(), true );
        materializeBlock( node );
        jmp( node, node->next(), true ); // will be eliminated since next() is the likely successor
        bb_needs_jump = false;
    }
}


void CodeGenerator::aSendNode( SendNode *node ) {
    // Question concerning saveRegisters() below: is it really needed to also save the
    // recv (it is a parameter passed in)? If it happens to be also a visible value of
    // the caller and if it has not been stored before we would get an "intermediate"
    // frame with a unsaved register value => we should save the recv as well. However
    // this is only true, if the recv value is not explicitly assigned (and the assignment
    // has not been eliminated). Otherwise this is an unneccessary save instr.
    // For now: be conservative & save it always.
    if ( node->isCounting() )
        incrementInvocationCounter();
    const char     *entry = node->isSuperSend() ? CompiledInlineCache::superLookupRoutine() : CompiledInlineCache::normalLookupRoutine();
    PseudoRegister *recv  = node->recv();
    _currentMapping->killDeadsAt( node->next(), recv ); // free mapping of unused pseudoRegisters
    _currentMapping->makeInjective();                   // make injective because NonLocalReturn cannot deal with non-injective mappings yet
    _currentMapping->saveRegisters();                   // make sure none of the remaining pseudoRegister values are lost
    _currentMapping->killRegisters( recv );             // so PseudoRegisterMapping::use can safely allocate receiverLoc if necessary
    _currentMapping->use( recv, receiver_reg );         // make sure recv is in the right register
    updateDebuggingInfo( node );
    _masm->call( entry, RelocationInformation::RelocationType::ic_type );
    _currentMapping->killRegisters();

    // compute flag settings
    std::int32_t flags = 0;
    if ( node->isSuperSend() )
        setNthBit( flags, super_send_bit_no );
    if ( node->isUninlinable() )
        setNthBit( flags, uninlinable_bit_no );
    if ( node->staticReceiver() )
        setNthBit( flags, receiver_static_bit_no );
    // inline cache
    inlineCache( node, node->scope()->nlrTestPoint(), flags );
    _currentMapping->mapToRegister( node->dst(), result_reg );    // NonLocalReturn mapping of node->dst is handled in NonLocalReturnTestNode
}


void CodeGenerator::aPrimitiveNode( PrimitiveNode *node ) {
    MergeNode *nlr = node->pdesc()->can_perform_NonLocalReturn() ? node->scope()->nlrTestPoint() : nullptr;
    _currentMapping->killDeadsAt( node->next() );        // free mapping of unused pseudoRegisters
    _currentMapping->makeInjective();            // make injective because NonLocalReturn cannot deal with non-injective mappings yet
    _currentMapping->saveRegisters();            // make sure none of the remaining pseudoRegister values are lost
    _currentMapping->killRegisters();
    updateDebuggingInfo( node );
    // Note: cannot use call_C because inline cache code has to come immediately after call instruction!
    _masm->set_last_Delta_frame_before_call();
    _masm->call( (const char *) ( node->pdesc()->fn() ), RelocationInformation::RelocationType::primitive_type );
    _currentMapping->killRegisters();
    if ( nlr not_eq nullptr )
        inlineCache( node, nlr );
    _masm->reset_last_Delta_frame();
    _currentMapping->mapToRegister( node->dst(), result_reg );    // NonLocalReturn mapping of node->dst is handled in NonLocalReturnTestNode
}


void CodeGenerator::aDLLNode( DLLNode *node ) {
    // determine entry point depending on whether a run-time lookup is needed or not.
    // Note: do not do a DLL lookup at compile time since this may cause a call back.
    const char *entry = ( node->function() == nullptr ) ? StubRoutines::lookup_DLL_entry( node->async() ) : StubRoutines::call_DLL_entry( node->async() );
    // pass arguments for DLL lookup/parameter conversion routine in registers
    // (change this code if the corresponding routines change (StubRoutines))
    // ebx: no. of arguments
    // ecx: address of last argument
    // edx: dll function entry point (backpatched, belongs to Compiled_DLLCache)
    _currentMapping->saveRegisters();
    _currentMapping->killRegisters();
    updateDebuggingInfo( node );
    _masm->movl( ebx, node->nofArguments() );
    _masm->movl( ecx, esp );

    // Compiled_DLLCache
    // This code pattern must correspond to the Compiled_DLLCache layout
    // (make sure assembler is not optimizing mov reg, 0 into xor reg, reg!)
    _masm->movl( edx, std::int32_t( node->function() ) );        // part of Compiled_DLLCache
    _masm->inline_oop( node->dll_name() );            // part of Compiled_DLLCache
    _masm->inline_oop( node->function_name() );        // part of Compiled_DLLCache
    _masm->call( entry, RelocationInformation::RelocationType::runtime_call_type );    // call lookup/parameter conversion routine
    _currentMapping->killRegisters();

    // For now: ordinary inline cache even though NonLocalReturns through DLLs are not allowed yet
    // (make sure somebody is popping arguments if NonLocalReturns are used).
    inlineCache( node, node->scope()->nlrTestPoint() );
    _currentMapping->mapToRegister( node->dst(), result_reg );
    _masm->addl( esp, node->nofArguments() * OOP_SIZE );    // get rid of arguments
}


/*
static void testForSingleKlass(Register obj, klassOop klass, Register klassReg, Label& success, Label& failure) {
  if (klass == Universe::smiKlassObject()) {
    // check tag
    theMacroAssm->test(obj, MEMOOP_TAG);
  } else if (klass == Universe::trueObject()->klass()) {
    // only one instance: compare with trueObject
    theMacroAssm->cmpl(obj, Universe::trueObject());
  } else if (klass == Universe::falseObject()->klass()) {
    // only one instance: compare with falseObject
    theMacroAssm->cmpl(obj, Universe::falseObject());
  } else if (klass == Universe::nilObject()->klass()) {
    // only one instance: compare with nilObject
    theMacroAssm->cmpl(obj, Universe::nilObject());
  } else {
    // compare against obj's klass - must check if smi_t first
    theMacroAssm->test(obj, MEMOOP_TAG);
    theMacroAssm->jcc(Assembler::Condition::zero, failure);
    theMacroAssm->movl(klassReg, Address(obj, memOopDescriptor::klass_byte_offset()));
    theMacroAssm->cmpl(klassReg, klass);
  }
  theMacroAssm->jcc(Assembler::Condition::notEqual, failure);
  theMacroAssm->jmp(success);	// this jump will be eliminated since this is the likely successor
}
*/


void CodeGenerator::testForSingleKlass( Register obj, KlassOop klass, Register klassReg, Label &success, Label &failure ) {

    if ( klass == Universe::smiKlassObject() ) {
        // check tag
        _masm->test( obj, MEMOOP_TAG );
    } else if ( klass == Universe::trueObject()->klass() ) {
        // only one instance: compare with trueObject
        _masm->cmpl( obj, Universe::trueObject() );
    } else if ( klass == Universe::falseObject()->klass() ) {
        // only one instance: compare with falseObject
        _masm->cmpl( obj, Universe::falseObject() );
    } else if ( klass == Universe::nilObject()->klass() ) {
        // only one instance: compare with nilObject
        _masm->cmpl( obj, Universe::nilObject() );
    } else {
        // compare against obj's klass - must check if smi_t first
        _masm->test( obj, MEMOOP_TAG );
        _masm->jcc( Assembler::Condition::zero, failure );
        _masm->movl( klassReg, Address( obj, MemOopDescriptor::klass_byte_offset() ) );
        _masm->cmpl( klassReg, klass );
    }
    _masm->jcc( Assembler::Condition::notEqual, failure );
    _masm->jmp( success );    // this jump will be eliminated since this is the likely successor
}


void CodeGenerator::generateTypeTests( LoopHeaderNode *node, Label &failure ) {
    static_cast<void>(node); // unused
    static_cast<void>(failure); // unused

    Unimplemented();

    std::int32_t       last = 0;
    for ( std::int32_t i    = 0; i <= last; i++ ) {
        HoistedTypeTest *t = node->tests()->at( i );
        if ( t->_testedPR->_location == Location::UNALLOCATED_LOCATION )
            continue;    // optimized away, or ConstPseudoRegister
        if ( t->_testedPR->isConstPseudoRegister() ) {
            guarantee( t->_testedPR->_location == Location::UNALLOCATED_LOCATION, "code assumes ConstPseudoRegisters are unallocated" );
            //handleConstantTypeTest((ConstPseudoRegister*)t->testedPR, t->klasses);
        } else {


        }
    }
}


/*
void LoopHeaderNode::generateTypeTests(Label& cont, Label& failure) {
  // test all values against expected classes
  Label* ok;
  const Register klassReg = temp2;
  const std::int32_t len = _tests->length() - 1;
  std::int32_t last;						// last case that generates a test
  for (last = len; last >= 0 and _tests->at(last)->testedPR->loc ==Location::UNALLOCATED_LOCATION; last--) ;
  if (last < 0) return;					// no tests at all
  for (std::int32_t i = 0; i <= last; i++) {
    HoistedTypeTest* t = _tests->at(i);
    if (t->testedPR->loc ==Location::UNALLOCATED_LOCATION) continue;	// optimized away, or ConstPseudoRegister
    if (t->testedPR->isConstPseudoRegister()) {
      guarantee(t->testedPR->loc ==Location::UNALLOCATED_LOCATION, "code assumes ConstPseudoRegisters are unallocated");
      handleConstantTypeTest((ConstPseudoRegister*)t->testedPR, t->klasses);
    } else {
      const Register obj = movePseudoRegisterToReg(t->testedPR, temp1);
      Label okLabel;
      ok = (i == last) ? &cont : &okLabel;
      if (t->klasses->length() == 1) {
	testForSingleKlass(obj, t->klasses->at(0), klassReg, *ok, failure);
      } else if (t->klasses->length() == 2 and
		 testForBoolKlasses(obj, t->klasses->at(0), t->klasses->at(1), klassReg, true,
		 *ok, *ok, failure)) {
	// ok, was a bool test
      } else {
	const std::int32_t len = t->klasses->length();
	GrowableArray<Label*> labels(len + 1);
	labels.append(&failure);
	for (std::int32_t i = 0; i < len; i++) labels.append(ok);
	generalTypeTest(obj, klassReg, true, t->klasses, &labels);
      }
      if (i not_eq last) theMacroAssm->bind(*ok);
    }
  }
}
*/


/*
void CodeGenerator::handleConstantTypeTest(ConstPseudoRegister* r, GrowableArray<klassOop>* klasses) {
  // constant r is tested against klasses (efficiency hack: klasses == nullptr means {smi_t})
  if ((klasses == nullptr and r->constant->is_smi()) or (klasses and klasses->contains(r->constant->klass()))) {
    // always ok, no need to test
  } else {
    compiler_warning("loop header type test will always fail!");
    // don't jump to failure because that would make subsequent LoopHeader code unreachable (--> breaks back end)
    theMacroAssm->call(StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type);
  }
}
*/


void CodeGenerator::generateIntegerLoopTest( PseudoRegister *pseudoRegister, LoopHeaderNode *node, Label &failure ) {
    static_cast<void>(node); // unused
    static_cast<void>(failure); // unused

    if ( pseudoRegister not_eq nullptr ) {
        if ( pseudoRegister->isConstPseudoRegister() ) {
            // no run-time test necessary
            //handleConstantTypeTest((ConstPseudoRegister*)pseudoRegister, nullptr);
        } else if ( pseudoRegister->_location == Location::UNALLOCATED_LOCATION ) {
            // pseudoRegister is never used in loop => no test needed
            guarantee( pseudoRegister->cpseudoRegister() == pseudoRegister, "should use cpseudoRegister()" );
        } else {
            // generate run-time test
            /*
      if (prev.is_unbound()) theMacroAssm->bind(prev);
      Label ok;
      const Register obj = movePseudoRegisterToReg(p, temp1);
      testForSingleKlass(obj, Universe::smiKlassObject(), klassReg, ok, failure);
      theMacroAssm->bind(ok);
      */
        }
    }
}


/*
void LoopHeaderNode::generateIntegerLoopTest(PseudoRegister* p, Label& prev, Label& failure) {
  const Register klassReg = temp2;
  if (p not_eq nullptr) {
    if (p->isConstPseudoRegister()) {
      // no run-time test necessary
      handleConstantTypeTest((ConstPseudoRegister*)p, nullptr);
    } else if (p->loc ==Location::UNALLOCATED_LOCATION) {
      // p is never used in loop, so no test needed
      guarantee(p->cpseudoRegister() == p, "should use cpseudoRegister()");
    } else {
      // generate run-time test
      if (prev.is_unbound()) theMacroAssm->bind(prev);
      Label ok;
      const Register obj = movePseudoRegisterToReg(p, temp1);
      testForSingleKlass(obj, Universe::smiKlassObject(), klassReg, ok, failure);
      theMacroAssm->bind(ok);
    }
  }
}
*/


void CodeGenerator::generateIntegerLoopTests( LoopHeaderNode *node, Label &failure ) {
    st_assert( node->isIntegerLoop(), "must be integer loop" );
    generateIntegerLoopTest( node->lowerBound(), node, failure );
    generateIntegerLoopTest( node->upperBound(), node, failure );
    generateIntegerLoopTest( node->loopVar(), node, failure );
}


//void LoopHeaderNode::generateIntegerLoopTests( Label &prev, Label &failure ) {
//    if ( not _integerLoop ) return;
//    generateIntegerLoopTest( _lowerBound, prev, failure );
//    generateIntegerLoopTest( _upperBound, prev, failure );
//    generateIntegerLoopTest( _loopVar, prev, failure );
//}


void CodeGenerator::generateArrayLoopTests( LoopHeaderNode *node, Label &failure ) {
    static_cast<void>(node); // unused
    static_cast<void>(failure); // unused

    st_assert( node->isIntegerLoop(), "must be integer loop" );
    if ( node->upperLoad() == nullptr ) return;

    // The loop variable iterates from lowerBound...array size; if any of the array accesses
    // use the loop variable without an index range check, we need to check it here.
    PseudoRegister      *loopArray = node->upperLoad()->src();
    AbstractArrayAtNode *atNode;

    std::int32_t i = node->arrayAccesses()->length();
    while ( i-- > 0 ) {
        atNode = node->arrayAccesses()->at( i );
        if ( atNode->src() == loopArray and not atNode->needsBoundsCheck() ) {
            break;
        }
    }

    if ( i < 0 ) {
        return;
    }

    // loopVar is used to index into array; make sure lower & upper bound is within array range
    PseudoRegister *lo = node->lowerBound();
//    PseudoRegister *hi = node->upperBound();

    if ( ( lo not_eq nullptr ) and
         ( lo->isConstPseudoRegister() ) and
         ( (ConstPseudoRegister *) lo )->constant->is_smi() and
         ( (ConstPseudoRegister *) lo )->constant >= smiOopFromValue( 1 ) ) {
        // nothing

    } else {
        // test lower bound
        //
        if ( lo->_location == Location::UNALLOCATED_LOCATION ) {
            // nothing
        } else {
            // nothing
        }
    }
    // test upper bound

}


/*
void LoopHeaderNode::generateArrayLoopTests(Label& prev, Label& failure) {
  if (not _integerLoop) return;
  Register boundReg = temp1;
  const Register tempseudoRegister  = temp2;
  if (_upperLoad not_eq nullptr) {
    // The loop variable iterates from lowerBound...array size; if any of the array accesses use the loop variable
    // without an index range check, we need to check it here.
    PseudoRegister* loopArray = _upperLoad->src();
    AbstractArrayAtNode* atNode;
    for (std::int32_t i = _arrayAccesses->length() - 1; i >= 0; i--) {
      atNode = _arrayAccesses->at(i);
      if (atNode->src() == loopArray and not atNode->needsBoundsCheck()) break;
    }
    if (i >= 0) {
      // loopVar is used to index into array; make sure lower & upper bound is within array range
      if (_lowerBound not_eq nullptr and _lowerBound->isConstPseudoRegister() and ((ConstPseudoRegister*)_lowerBound)->constant->is_smi() and ((ConstPseudoRegister*)_lowerBound)->constant >= smiOopFromValue(1)) {
	// loopVar iterates from smi_const to array size, so no test necessary
      } else {
	// test lower bound
       if (prev.is_unbound()) theMacroAssm->bind(prev);
       if (_lowerBound->loc ==Location::UNALLOCATED_LOCATION) {
	 guarantee(_lowerBound->cpseudoRegister() == _lowerBound, "should use cpseudoRegister()");
       } else {
	 const Register t = movePseudoRegisterToReg(_lowerBound ? _lowerBound : _loopVar, tempseudoRegister);
	 theMacroAssm->cmpl(boundReg, smiOopFromValue(1));
	 theMacroAssm->jcc(Assembler::Condition::less, failure);
       }
      }

      // test upper bound
      boundReg = movePseudoRegisterToReg(_upperBound, boundReg);
      const Register t = movePseudoRegisterToReg(atNode->src(), tempseudoRegister);
      theMacroAssm->movl(t, Address(t, byteOffset(atNode->sizeOffset())));
      theMacroAssm->cmpl(boundReg, t);
      theMacroAssm->jcc(Assembler::Condition::above, failure);
    }
  }
}
*/


void CodeGenerator::aLoopHeaderNode( LoopHeaderNode *node ) {

    if ( node->isActivated() ) {
        spdlog::warn( "loop header node not yet implemented" );
        return;

        // the loop header node performs all checks hoisted out of the loop:
        // for general loops:
        //   - do all type tests in the list, uncommon branch if they fail
        //     (common case: true/false tests, single-klass tests)
        // additionally for integer loops:
        //   - test lowerBound (may be nullptr), upperBound, loopVar for smi_t-ness (the first two may be ConstPseudoRegisters)
        //   - if upperBound is nullptr, upperLoad is load of the array size
        //   - if loopArray is non-nullptr, check lowerBound (if non-nullptr) or initial value of loopVar against 1
        Label failure;
        generateTypeTests( node, failure );
        if ( node->isIntegerLoop() ) {
            generateIntegerLoopTests( node, failure );
            generateArrayLoopTests( node, failure );
        }
        _masm->bind( failure );
        updateDebuggingInfo( node );
        _masm->call( StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type );
        bb_needs_jump = false;
        setMapping( nullptr );
    }


}


/*

void LoopHeaderNode::gen() {
  if (not _activated) return;    // loop wasn't optimized
  // the loop header node performs all checks hoisted out of the loop:
  // for general loops:
  //   - do all type tests in the list, uncommon branch if they fail
  //     (common case: true/false tests, single-klass tests)
  // additionally for integer loops:
  //   - test lowerBound (may be nullptr), upperBound, loopVar for smi_t-ness (the first two may be ConstPseudoRegisters)
  //   - if upperBound is nullptr, upperLoad is load of the array size
  //   - if loopArray is non-nullptr, check lowerBound (if non-nullptr) or initial value of loopVar against 1

  TrivialNode::gen();
  Label ok;
  Label failure;
  generateTypeTests(ok, failure);
  generateIntegerLoopTests(ok, failure);
  generateArrayLoopTests(ok, failure);
  if (ok.is_unbound()) theMacroAssm->bind(ok);
  theMacroAssm->jmp(next()->label);
  // above 2 lines could be eliminated with: if (ok.is_unbound()) ok.redirectTo(next()->label)
  bb_needs_jump = false;  // we generate all jumps explicitly
  theMacroAssm->bind(failure);
  theMacroAssm->call(StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type);

}

*/


void CodeGenerator::aReturnNode( ReturnNode *node ) {
    InlinedScope *scope = node->scope();
    if ( scope->needsContextZapping() )
        zapContext( scope->context() );    // <<< still needed? What about ContextZapNode?
    // make sure result is in result_reg, no other pseudoRegisters are used anymore
    PseudoRegister *result = scope->resultPR;
    _currentMapping->killRegisters( result );
    _currentMapping->use( result, result_reg );
    // remove stack frame & return
    if ( VerifyCode or TraceCalls or TraceResults )
        callVerifyReturn();
    std::int32_t no_of_args_to_pop = scope->nofArguments();
    if ( scope->method()->is_blockMethod() ) {
        // blocks are called via primitiveValue => need to pop first argument
        // of primitiveValue (= block closure) as well since return happens
        // directly (and not through primitiveValue).
        no_of_args_to_pop++;
    }
    _masm->leave();
    _masm->ret( no_of_args_to_pop * OOP_SIZE );
    // no pseudoRegisters accessible anymore
    setMapping( nullptr );
}


void CodeGenerator::aNonLocalReturnSetupNode( NonLocalReturnSetupNode *node ) {
    InlinedScope *scope = node->scope();
    // compute home into a temporary register (NonLocalReturn_home_reg might still be in use - but try to use it if possible)
    // and check if context has been  zapped
    //
    // QUESTION: Who is popping the arguments (for an ordinary return it happens automatically in the return).
    // Couldn't that be a problem in loops? How's this done in the old backend? In the interpreter?
    Label        NonLocalReturn_error;                            // for NonLocalReturns to non-existing frames
    Temporary    home( _currentMapping, NonLocalReturn_home_reg );            // try to allocate temporary home into right register
    uplevelBase( scope->context(), scope->homeContext() + 1, home.reg() );    // compute home
    _masm->testl( home.reg(), home.reg() );                    // check if zapped
    _masm->jcc( Assembler::Condition::zero, NonLocalReturn_error );                // zero -> home has been zapped
    // load result into temporary register (NonLocalReturn_result_reg might still be in use - but try to use it if possible)
    PseudoRegister *resultPseudoRegister = scope->resultPR;
    _currentMapping->killRegisters( resultPseudoRegister );                // no PseudoRegisters are used anymore except result
    Register result;                            // temporary result register
    {
        Temporary t( _currentMapping, NonLocalReturn_result_reg );
        result = t.reg();
    }    // try to allocate temporary result into right register
    _currentMapping->use( resultPseudoRegister, result );                // load result into temporary result register
    // finally assign result and home to the right registers, make sure that they
    // don't overwrite each other (home could be in the result register & vice versa).
    // For now push them and pop them back into the right registers.
    if ( result not_eq NonLocalReturn_result_reg or home.reg() not_eq NonLocalReturn_home_reg ) {
        _masm->pushl( result );
        _masm->pushl( home.reg() );
        _masm->popl( NonLocalReturn_home_reg );
        _masm->popl( NonLocalReturn_result_reg );
    }
    // assign home id
    _masm->movl( NonLocalReturn_homeId_reg, scope->home()->scopeID() );
    // issue NonLocalReturn
    if ( VerifyCode or TraceCalls or TraceResults )
        callVerifyNonLocalReturn();
    _masm->jmp( StubRoutines::continue_NonLocalReturn_entry(), RelocationInformation::RelocationType::runtime_call_type );
    // call run-time routine in failure case
    // what about the debugging information? FIX THIS
    _masm->bind( NonLocalReturn_error );
    _masm->call_C( (const char *) suspend_on_NonLocalReturn_error, RelocationInformation::RelocationType::runtime_call_type );
    // no pseudoRegisters accessible anymore
    setMapping( nullptr );
}


void CodeGenerator::anInlinedReturnNode( InlinedReturnNode *node ) {
    static_cast<void>(node); // unused
    // Not generated anymore for new backend
    ShouldNotReachHere();
}


void CodeGenerator::aNonLocalReturnContinuationNode( NonLocalReturnContinuationNode *node ) {
    guarantee( _currentMapping->NonLocalReturninProgress(), "NonLocalReturn must be in progress" );
    InlinedScope *scope = node->scope();
    if ( scope->needsContextZapping() )
        zapContext( scope->context() );
    // continue with NonLocalReturn
    if ( VerifyCode or TraceCalls or TraceResults )
        callVerifyNonLocalReturn();
    _masm->jmp( StubRoutines::continue_NonLocalReturn_entry(), RelocationInformation::RelocationType::runtime_call_type );
    // no pseudoRegisters accessible anymore
    setMapping( nullptr );
}


Assembler::Condition CodeGenerator::mapToCC( BranchOpCode op ) {
    switch ( op ) {
        case BranchOpCode::EQBranchOp:
            return Assembler::Condition::equal;
        case BranchOpCode::NEBranchOp:
            return Assembler::Condition::notEqual;
        case BranchOpCode::LTBranchOp:
            return Assembler::Condition::less;
        case BranchOpCode::LEBranchOp:
            return Assembler::Condition::lessEqual;
        case BranchOpCode::GTBranchOp:
            return Assembler::Condition::greater;
        case BranchOpCode::GEBranchOp:
            return Assembler::Condition::greaterEqual;
        case BranchOpCode::LTUBranchOp:
            return Assembler::Condition::below;
        case BranchOpCode::LEUBranchOp:
            return Assembler::Condition::belowEqual;
        case BranchOpCode::GTUBranchOp:
            return Assembler::Condition::above;
        case BranchOpCode::GEUBranchOp:
            return Assembler::Condition::aboveEqual;
        case BranchOpCode::VSBranchOp:
            return Assembler::Condition::overflow;
        case BranchOpCode::VCBranchOp:
            return Assembler::Condition::noOverflow;
    }
    ShouldNotReachHere();
    return Assembler::Condition::zero;
}


void CodeGenerator::aBranchNode( BranchNode *node ) {
    jcc( mapToCC( node->op() ), node, node->next( 1 ) );
}


void CodeGenerator::aTypeTestNode( TypeTestNode *node ) {

    //
    // Note 1: This code pattern requires *no* particular order of the the classes of the TypeTestNode.
    //
    // Note 2: In case of a TypeTestNode without unknown case, the last case would not have to be conditional.
    //         However, for debugging purposes right now all cases are always explicitly checked.
    //

    const std::int32_t len                  = node->classes()->length();

    if ( ReorderBBs ) {
        PseudoRegisterLocker lock( node->src() );
        Register             obj = use( node->src() );

        if ( len == 1 ) {
            // handle all cases where only one klass is involved
            st_assert( node->hasUnknown(), "should be eliminated if there's no unknown case" );
            st_assert( node->likelySuccessor() == node->next( 1 ), "code pattern is not optimal" );
            KlassOop klass = node->classes()->at( 0 );
            if ( klass == Universe::smiKlassObject() ) {
                // check tag
                _masm->test( obj, MEMOOP_TAG );
            } else if ( klass == Universe::trueObject()->klass() ) {
                // only one instance: compare with trueObject
                _masm->cmpl( obj, Universe::trueObject() );
            } else if ( klass == Universe::falseObject()->klass() ) {
                // only one instance: compare with falseObject
                _masm->cmpl( obj, Universe::falseObject() );
            } else if ( klass == Universe::nilObject()->klass() ) {
                // only one instance: compare with nilObject
                _masm->cmpl( obj, Universe::nilObject() );
            } else {
                // compare against obj's klass - must check if smi_t first
                Temporary objKlass( _currentMapping );
                _masm->test( obj, MEMOOP_TAG );
                _masm->jcc( Assembler::Condition::zero, node->next()->_label );
                _masm->movl( objKlass.reg(), Address( obj, MemOopDescriptor::klass_byte_offset() ) );
                _masm->cmpl( objKlass.reg(), klass );
            }
            jcc( Assembler::Condition::notEqual, node, node->next() );
            jmp( node, node->next( 1 ) );            // this jump will be eliminated since this is the likely successor
            bb_needs_jump = false;            // no jump necessary at end of basic block
            return;
        }

        if ( len == 2 ) {
            // handle pure boolean cases (ifTrue:/ifFalse:)
            KlassOop klass1 = node->classes()->at( 0 );
            KlassOop klass2 = node->classes()->at( 1 );
            Oop      bool1  = Universe::trueObject();
            Oop      bool2  = Universe::falseObject();

            if ( klass1 == bool2->klass() and klass2 == bool1->klass() ) {
                Oop t = bool1;
                bool1 = bool2;
                bool2 = t;
            }

            if ( klass1 == bool1->klass() and klass2 == bool2->klass() ) {
                const bool ignoreNoUnknownForNow = true;

                // Note: Uncommon case: A TypeTestNode with no uncommon case has a successor
                //       at next(0) anyhow (because there are no "holes" (= NULLs) in the
                //       successor list of a node). That is, for now we have to jump to that
                //       point somehow (even though it can never happen), because otherwise
                //       the PseudoRegisterMapping is not set for that node. (Maybe one should detect
                //       this case and then set a "dummy" PseudoRegisterMapping, since it is not used
                //       anyhow but needs to be there only for assertion checking).

                if ( ignoreNoUnknownForNow or node->hasUnknown() ) {
                    st_assert( ignoreNoUnknownForNow or node->likelySuccessor() == node->next( 2 ), "code pattern is not optimal" );
                    _masm->cmpl( obj, bool1 );
                    jcc( Assembler::Condition::equal, node, node->next( 1 ) );
                    _masm->cmpl( obj, bool2 );
                    jcc( Assembler::Condition::notEqual, node, node->next() );
                    jmp( node, node->next( 2 ) );        // this jump will be eliminated since this is the likely successor

                } else {
                    st_assert( node->likelySuccessor() == node->next( 1 ), "code pattern is not optimal" );
                    _masm->cmpl( obj, bool2 );
                    jcc( Assembler::Condition::equal, node, node->next( 2 ) );
                    jmp( node, node->next( 1 ) );        // this jump will be eliminated since this is the likely successor
                }

                bb_needs_jump = false;            // no jump necessary at end of basic block
                return;
            }
        }
    }

    // general case
    Label                unknownCase;
    Temporary            objKlass( _currentMapping );
    bool                 klassHasBeenLoaded = false;
    bool                 smiHasBeenChecked  = false;
    PseudoRegisterLocker lock( node->src() );
    Register             obj                = use( node->src() );
    for ( std::int32_t   i                  = 0; i < len; i++ ) {
        KlassOop klass = node->classes()->at( i );
        if ( klass == trueObject->klass() ) {
            // only one instance: compare with trueObject
            _masm->cmpl( obj, trueObject );
            jcc( Assembler::Condition::equal, node, node->next( i + 1 ) );
        } else if ( klass == falseObject->klass() ) {
            // only one instance: compare with falseObject
            _masm->cmpl( obj, falseObject );
            jcc( Assembler::Condition::equal, node, node->next( i + 1 ) );
        } else if ( klass == nilObject->klass() ) {
            // only one instance: compare with nilObject
            _masm->cmpl( obj, nilObject );
            jcc( Assembler::Condition::equal, node, node->next( i + 1 ) );
        } else if ( klass == smiKlassObject ) {
            // check smi_t tag only if not checked already, otherwise ignore
            if ( not smiHasBeenChecked ) {
                _masm->test( obj, MEMOOP_TAG );
                jcc( Assembler::Condition::zero, node, node->next( i + 1 ) );
                smiHasBeenChecked = true;
            }
        } else {
            // compare with klass
            if ( not klassHasBeenLoaded ) {
                if ( not smiHasBeenChecked ) {
                    Node *smiCase = node->smiCase();
                    if ( smiCase not_eq nullptr or node->hasUnknown() ) {
                        // smi_t can actually appear => check for it
                        _masm->test( obj, MEMOOP_TAG );
                        if ( smiCase not_eq nullptr ) {
                            // jump to smiCase if there's one
                            jcc( Assembler::Condition::zero, node, smiCase );
                        } else {
                            // node hasUnknown & smiCase cannot happen => jump to unknown case (end of typetest)
                            _masm->jcc( Assembler::Condition::zero, unknownCase );
                        }
                    }
                    smiHasBeenChecked = true;
                }
                _masm->movl( objKlass.reg(), Address( obj, MemOopDescriptor::klass_byte_offset() ) );
                klassHasBeenLoaded = true;
            }
            _masm->cmpl( objKlass.reg(), klass );
            jcc( Assembler::Condition::equal, node, node->next( i + 1 ) );
        }
    }
    // bind label in any case to avoid unbound label assertion bug
    _masm->bind( unknownCase );

    // Note: Possible problem: if the smi_t case is checked before the class
    //       is loaded, there's possibly a jump to the end of the TypeTestNode
    //       from the smi_t case. However, then the klass register isn't defined.
    //       if later there's the uncommon case, the klass register is defined.
    //       What if one refers to that register? Or is it not possible because
    //       it's not a regular PseudoRegister but a temporary? Think about this!
    //
    // >>>>> IS A PROBLEM! The temporary is likely to throw out another PseudoRegister from a register! FIX THIS!
}


// Note: Maybe should reorganize the way NonLocalReturns are treated in the intermediate representation;
// may be able to avoid some jumps. For example, continuing the NonLocalReturn is done via a stub routine,
// maybe one can jump to that routine conditionally and thereby save a jump around a jump (that
// stub routine could also do the zapping if necessary (could come in two versions)).

void CodeGenerator::aNonLocalReturnTestNode( NonLocalReturnTestNode *node ) {
    st_assert( _currentMapping->NonLocalReturninProgress(), "NonLocalReturn must be in progress" );
    InlinedScope *scope = node->scope();
    // check if arrived at the right frame
    Label        L;
    _masm->cmpl( NonLocalReturn_home_reg, frame_reg );
    _masm->jcc( Assembler::Condition::notEqual, L );
    // check if arrived at the right scope within the frame
    std::int32_t id = scope->scopeID();
    if ( id == 0 ) {
        // use test instruction to compare against 0 (smaller code than with cmp)
        _masm->testl( NonLocalReturn_homeId_reg, NonLocalReturn_homeId_reg );
    } else {
        _masm->cmpl( NonLocalReturn_homeId_reg, id );
    }
    _currentMapping->releaseNonLocalReturnRegisters();
    jcc( Assembler::Condition::equal, node, node->next( 1 ) );
    _currentMapping->acquireNonLocalReturnRegisters();
    // otherwise continue NonLocalReturn
    _masm->bind( L );
}


void CodeGenerator::aMergeNode( MergeNode *node ) {
    st_assert( node->isTrivial() or _currentMapping->isInjective(), "must be injective if more than one predecessor" );
}


void CodeGenerator::jcc_error( Assembler::Condition cc, AbstractBranchNode *node, Label &label ) {
    st_assert( node->canFail(), "should not be called if node cannot fail" );
    Node *failure_start = node->next( 1 );
    if ( failure_start->isUncommonNode() ) {
        jcc( cc, node, failure_start, true );
    } else {
        _masm->jcc( cc, label );
    }
}


void CodeGenerator::anArrayAtNode( ArrayAtNode *node ) {
    PseudoRegister       *array    = node->array();
    PseudoRegister       *index    = node->index();
    PseudoRegister       *result   = node->dst();
    PseudoRegister       *error    = node->error();
    PseudoRegisterLocker lock( array, index );
    Register             array_reg = use( array );
    // use temporary register for index - will be modified
    Temporary            offset( _currentMapping, index );
    // first element is at index 1 => subtract smi_t(1) (doesn't change smi_t/Oop property)
    theMacroAssembler->subl( offset.reg(), std::int32_t( smiOop_one ) );
    // do index smi_t check if necessary (still possible, even after subtracting smi_t(1))
    Label indexNotSmi;
    if ( not node->index_is_smi() ) {
        _masm->test( offset.reg(), MEMOOP_TAG );
        jcc_error( Assembler::Condition::notZero, node, indexNotSmi );
    }
    // do bounds check if necessary
    Label indexOutOfBounds;
    if ( node->index_needs_bounds_check() ) {
        const std::int32_t size_offset = byteOffset( node->size_word_offset() );
        _masm->cmpl( offset.reg(), Address( array_reg, size_offset ) );
        jcc_error( Assembler::Condition::aboveEqual, node, indexOutOfBounds );
    }
    // load element
    st_assert( TAG_SIZE == 2, "check this code" );
    const std::int32_t data_offset = byteOffset( node->data_word_offset() );
    switch ( node->access_type() ) {
        case ArrayAtNode::byte_at: {
            Register result_reg = def( result );
            _masm->sarl( offset.reg(), TAG_SIZE );    // adjust index
            if ( result_reg.hasByteRegister() ) {
                // result_reg has byte register -> can use byte load instruction
                _masm->xorl( result_reg, result_reg );    // clear destination register
                _masm->movb( result_reg, Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
            } else {
                // result_reg has no byte register -> cannot use byte load instruction
                // instead of doing better register selection use word load & mask for now
                _masm->movl( result_reg, Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
                _masm->andl( result_reg, 0x000000FF );    // clear uppper 3 bytes
            }
            _masm->shll( result_reg, TAG_SIZE );    // make result a smi_t
        }
            break;
        case ArrayAtNode::double_byte_at: {
            Register result_reg = def( result );
            _masm->sarl( offset.reg(), TAG_SIZE - 1 );// adjust index
            _masm->movl( result_reg, Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
            _masm->andl( result_reg, 0x0000FFFF );    // clear upper 2 bytes
            _masm->shll( result_reg, TAG_SIZE );    // make result a smi_t
        }
            break;
        case ArrayAtNode::character_at: {
            Register result_reg = def( result );
            _masm->sarl( offset.reg(), TAG_SIZE - 1 );// adjust index
            _masm->movl( result_reg, Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
            _masm->andl( result_reg, 0x0000FFFF );    // clear upper 2 bytes
            // use result_reg as index into asciiCharacters()
            // check index first, must be 0 <= result_reg < asciiCharacters()->length()
            ObjectArrayOop chars = Universe::asciiCharacters();
            _masm->cmpl( result_reg, chars->length() );
            jcc_error( Assembler::Condition::aboveEqual, node, indexOutOfBounds );
            // get character out of chars array
            _masm->movl( offset.reg(), chars );
            _masm->movl( result_reg, Address( offset.reg(), result_reg, Address::ScaleFactor::times_4, byteOffset( chars->klass()->klass_part()->non_indexable_size() + 1 ) ) );
        }
            break;
        case ArrayAtNode::object_at:
            // offset already shifted => no scaling necessary
            _masm->movl( def( result ), Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
            break;
        default: ShouldNotReachHere();
            break;
    }
    // handle error cases if not uncommon
    if ( node->canFail() and not node->next( 1 )->isUncommonNode() ) {
        Label exit;
        _masm->jmp( exit );
        // error messages
        if ( not node->index_is_smi() ) {
            _masm->bind( indexNotSmi );
            _masm->hlt();
        }
        if ( node->index_needs_bounds_check() ) {
            _masm->bind( indexOutOfBounds );
            _masm->hlt();
        }
        // hack for now - jcc so mapping stays alive
        // must do all the mapping in the program path taken - otherwise
        // mappings are inconsistent
        _masm->bind( exit );
        Register r = def( error );
        _masm->test( r, 0 );
        jcc( Assembler::Condition::notZero, node, node->next( 1 ) );
    }
}


void CodeGenerator::anArrayAtPutNode( ArrayAtPutNode *node ) {

    PseudoRegister *array          = node->array();
    PseudoRegister *index          = node->index();
    PseudoRegister *element        = node->element();
    PseudoRegister *error          = node->error();

    PseudoRegisterLocker lock( array, index, element );
    Register             array_reg = use( array );
    // use temporary register for index - will be modified
    Temporary            offset( _currentMapping, index );
    // first element is at index 1 => subtract smi_t(1) (doesn't change smi_t/Oop property)
    theMacroAssembler->subl( offset.reg(), std::int32_t( smiOop_one ) );
    // do index smi_t check if necessary (still possible, even after subtracting smi_t(1))
    Label indexNotSmi;
    if ( not node->index_is_smi() ) {
        _masm->test( offset.reg(), MEMOOP_TAG );
        jcc_error( Assembler::Condition::notZero, node, indexNotSmi );
    }

    // do bounds check if necessary
    Label indexOutOfBounds;
    if ( node->index_needs_bounds_check() ) {
        const std::int32_t size_offset = byteOffset( node->size_word_offset() );
        _masm->cmpl( offset.reg(), Address( array_reg, size_offset ) );
        jcc_error( Assembler::Condition::aboveEqual, node, indexOutOfBounds );
    }

    // store element
    st_assert( TAG_SIZE == 2, "check this code" );
    const std::int32_t data_offset = byteOffset( node->data_word_offset() );
    Label              elementNotSmi, elementOutOfRange;
    switch ( node->access_type() ) {
        case ArrayAtPutNode::byte_at_put: { // use temporary register for element - will be modified
            Temporary elt( _currentMapping, element );
            _masm->sarl( offset.reg(), TAG_SIZE );    // adjust index
            // do element smi_t check if necessary
            if ( not node->element_is_smi() ) {
                _masm->test( elt.reg(), MEMOOP_TAG );
                jcc_error( Assembler::Condition::notZero, node, elementNotSmi );
            }
            _masm->sarl( elt.reg(), TAG_SIZE );    // convert element into (std::int32_t) byte
            // do element range check if necessary
            if ( node->element_needs_range_check() ) {
                _masm->cmpl( elt.reg(), 0x100 );
                jcc_error( Assembler::Condition::aboveEqual, node, elementOutOfRange );
            }
            // store the element
            if ( elt.reg().hasByteRegister() ) {
                // elt.reg() has byte register -> can use byte store instruction
                _masm->movb( Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ), elt.reg() );
            } else {
                // elt.reg() has no byte register -> cannot use byte store instruction
                // instead of doing a better register selection use word load/store & mask for now
                Temporary field( _currentMapping );
                _masm->movl( field.reg(), Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
                _masm->andl( field.reg(), 0xFFFFFF00 );    // mask out lower byte
                _masm->orl( field.reg(), elt.reg() );    // move elt (elt < 0x100 => no masking of elt needed)
                _masm->movl( Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ), field.reg() );
            }
            st_assert( not node->needs_store_check(), "just checking" );
        }
            break;
        case ArrayAtPutNode::double_byte_at_put: { // use temporary register for element - will be modified
            Temporary elt( _currentMapping, element );
            _masm->sarl( offset.reg(), TAG_SIZE - 1 );// adjust index
            // do element smi_t check if necessary
            if ( not node->element_is_smi() ) {
                _masm->test( elt.reg(), MEMOOP_TAG );
                jcc_error( Assembler::Condition::notZero, node, elementNotSmi );
            }
            _masm->sarl( elt.reg(), TAG_SIZE );    // convert element into (std::int32_t) double byte
            // do element range check if necessary
            if ( node->element_needs_range_check() ) {
                _masm->cmpl( elt.reg(), 0x10000 );
                jcc_error( Assembler::Condition::aboveEqual, node, elementOutOfRange );
            }
            // store the element
            if ( elt.reg().hasByteRegister() ) {
                // elt.reg() has byte register -> can use byte store instructions
                _masm->movb( Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset + 0 ), elt.reg() );
                _masm->shrl( elt.reg(), 8 );        // shift 2nd byte into low-byte position
                _masm->movb( Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset + 1 ), elt.reg() );
            } else {
                // elt.reg() has no byte register -> cannot use byte store instructions
                // instead of doing a better register selection use word load/store & mask for now
                Temporary field( _currentMapping );
                _masm->movl( field.reg(), Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
                _masm->andl( field.reg(), 0xFFFF0000 );    // mask out lower two bytes
                _masm->orl( field.reg(), elt.reg() );    // move elt (elt < 0x10000 => no masking of elt needed)
                _masm->movl( Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ), field.reg() );
            }
            st_assert( not node->needs_store_check(), "just checking" );
        }
            break;
        case ArrayAtPutNode::object_at_put:
            // offset already shifted => no scaling necessary
            if ( node->needs_store_check() ) {
                _masm->leal( offset.reg(), Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ) );
                _masm->movl( Address( offset.reg() ), use( element ) );
                storeCheck( offset.reg() );
            } else {
                _masm->movl( Address( array_reg, offset.reg(), Address::ScaleFactor::times_1, data_offset ), use( element ) );
            }
            break;
        default: ShouldNotReachHere();
            break;
    }
    // handle error cases if not uncommon
    if ( node->canFail() and not node->next( 1 )->isUncommonNode() ) {
        Label exit;
        _masm->jmp( exit );
        // error messages
        if ( not node->index_is_smi() ) {
            _masm->bind( indexNotSmi );
            _masm->hlt();
        }
        if ( node->index_needs_bounds_check() ) {
            _masm->bind( indexOutOfBounds );
            _masm->hlt();
        }
        if ( not node->element_is_smi() ) {
            _masm->bind( elementNotSmi );
            _masm->hlt();
        }
        if ( node->element_needs_range_check() ) {
            _masm->bind( elementOutOfRange );
            _masm->hlt();
        }
        // hack for now - jcc so mapping stays alive
        // must do all the mapping in the program path taken - otherwise
        // mappings are inconsistent
        _masm->bind( exit );
        Register r = def( error );
        _masm->test( r, 0 );
        jcc( Assembler::Condition::notZero, node, node->next( 1 ) );
    }
}


void CodeGenerator::anInlinedPrimitiveNode( InlinedPrimitiveNode *node ) {
    switch ( node->op() ) {
        case InlinedPrimitiveNode::Operation::obj_klass: {
            Label                is_smi;
            PseudoRegisterLocker lock( node->src() );
            Register             obj_reg   = use( node->src() );
            Register             klass_reg = def( node->dst() );
            _masm->movl( klass_reg, Universe::smiKlassObject() );
            _masm->test( obj_reg, MEMOOP_TAG );
            _masm->jcc( Assembler::Condition::zero, is_smi );
            _masm->movl( klass_reg, Address( obj_reg, MemOopDescriptor::klass_byte_offset() ) );
            _masm->bind( is_smi );
        };
            break;
        case InlinedPrimitiveNode::Operation::obj_hash: {
            Unimplemented();
            // Implemented for the smi_t klass only by now - can be resolved in
            // the PrimitiveInliner for that case without using an InlinedPrimitiveNode.
        };
            break;
        case InlinedPrimitiveNode::Operation::proxy_byte_at: {
            PseudoRegister       *proxy  = node->src();
            PseudoRegister       *index  = node->arg1();
            PseudoRegister       *result = node->dst();
            PseudoRegister       *error  = node->error();
            PseudoRegisterLocker lock( proxy, index );
            // use Temporary register for proxy & index - will be modified
            Temporary            base( _currentMapping, proxy );
            Temporary            offset( _currentMapping, index );
            // do index smi_t check if necessary
            Label                indexNotSmi;
            if ( not node->arg1_is_smi() ) {
                _masm->test( offset.reg(), MEMOOP_TAG );
                jcc_error( Assembler::Condition::notZero, node, indexNotSmi );
            }
            // load element
            st_assert( TAG_SIZE == 2, "check this code" );
            Register result_reg = def( result );
            _masm->movl( base.reg(), Address( base.reg(), pointer_offset ) );    // unbox proxy
            _masm->sarl( offset.reg(), TAG_SIZE );                // adjust index
            if ( result_reg.hasByteRegister() ) {
                // result_reg has byte register -> can use byte load instruction
                _masm->xorl( result_reg, result_reg );                // clear destination register
                _masm->movb( result_reg, Address( base.reg(), offset.reg(), Address::ScaleFactor::times_1, 0 ) );
            } else {
                // result_reg has no byte register -> cannot use byte load instruction
                // instead of doing better register selection use word load & mask for now
                _masm->movl( result_reg, Address( base.reg(), offset.reg(), Address::ScaleFactor::times_1, 0 ) );
                _masm->andl( result_reg, 0x000000FF );                // clear uppper 3 bytes
            }
            _masm->shll( result_reg, TAG_SIZE );                // make result a smi_t
            // handle error cases if not uncommon
            if ( node->canFail() and not node->next( 1 )->isUncommonNode() ) {
                Label exit;
                _masm->jmp( exit );
                // error messages
                if ( not node->arg1_is_smi() ) {
                    _masm->bind( indexNotSmi );
                    _masm->hlt();
                }
                // hack for now - jcc so mapping stays alive
                // must do all the mapping in the program path taken - otherwise
                // mappings are inconsistent
                _masm->bind( exit );
                Register r = def( error );
                _masm->test( r, 0 );
                jcc( Assembler::Condition::notZero, node, node->next( 1 ) );
            }
        }
            break;

        case InlinedPrimitiveNode::Operation::proxy_byte_at_put: {
            bool           const_val = node->arg2()->isConstPseudoRegister();
            PseudoRegister *proxy    = node->src();
            PseudoRegister *index    = node->arg1();
            PseudoRegister *value    = node->arg2();
            PseudoRegister *error    = node->error();
            // Locking turned off for now -> blocks too many registers for
            // this code (however may add unnecessary moves) -> find a better
            // solution for this
            //
            // PseudoRegisterLocker lock(proxy, index, value);
            // use Temporary register for proxy & index - will be modified
            Temporary      base( _currentMapping, proxy );
            Temporary      offset( _currentMapping, index );
            // use temporary register for value - will be modified
            // (actually only needed if not const_val - however right now
            // we can only allocate temps via constructors (i.e., they have
            // to be allocated/deallocated in a nested manner)).
            Temporary      val( _currentMapping );
            if ( const_val ) {
                // value doesn't have to be loaded -> do nothing here
                if ( not node->arg2_is_smi() ) st_fatal( "proxy_byte_at_put: should not happen - internal error" );
                //if (not node->arg2_is_smi()) fatal("proxy_byte_at_put: should not happen - tell Robert");
            } else {
                _masm->movl( val.reg(), use( value ) );
            }
            // do index smi_t check if necessary
            Label indexNotSmi;
            if ( not node->arg1_is_smi() ) {
                _masm->test( offset.reg(), MEMOOP_TAG );
                jcc_error( Assembler::Condition::notZero, node, indexNotSmi );
            }
            // do value smi_t check if necessary
            Label valueNotSmi;
            if ( not node->arg2_is_smi() ) {
                st_assert( not const_val, "constant shouldn't need a smi_t check" );
                _masm->test( val.reg(), MEMOOP_TAG );
                jcc_error( Assembler::Condition::notZero, node, valueNotSmi );
            }
            // store element
            st_assert( TAG_SIZE == 2, "check this code" );
            _masm->movl( base.reg(), Address( base.reg(), pointer_offset ) );    // unbox proxy
            _masm->sarl( offset.reg(), TAG_SIZE );                // adjust index
            if ( const_val ) {
                SMIOop constant = SMIOop( ( (ConstPseudoRegister *) value )->constant );
                st_assert( constant->is_smi(), "should be a smi_t" );
                _masm->movb( Address( base.reg(), offset.reg(), Address::ScaleFactor::times_1, 0 ), constant->value() & 0xFF );
            } else {
                _masm->sarl( val.reg(), TAG_SIZE );                // convert (smi_t)value into std::int32_t
                if ( val.reg().hasByteRegister() ) {
                    // val.reg() has byte register -> can use byte store instruction
                    _masm->movb( Address( base.reg(), offset.reg(), Address::ScaleFactor::times_1, 0 ), val.reg() );
                } else {
                    // val.reg() has no byte register -> cannot use byte store instruction
                    // instead of doing a better register selection use word load/store & mask for now
                    Temporary field( _currentMapping );
                    _masm->movl( field.reg(), Address( base.reg(), offset.reg(), Address::ScaleFactor::times_1, 0 ) );
                    _masm->andl( val.reg(), 0x000000FF );            // make sure value is one byte only
                    _masm->andl( field.reg(), 0xFFFFFF00 );            // mask out lower byte of target field
                    _masm->orl( field.reg(), val.reg() );            // move value byte into target field
                    _masm->movl( Address( base.reg(), offset.reg(), Address::ScaleFactor::times_1, 0 ), field.reg() );
                }
            }

            // handle error cases if not uncommon
            if ( node->canFail() and not node->next( 1 )->isUncommonNode() ) {
                Label exit;
                _masm->jmp( exit );

                // error messages
                if ( not node->arg1_is_smi() ) {
                    _masm->bind( indexNotSmi );
                    _masm->hlt();
                }

                if ( not node->arg2_is_smi() ) {
                    _masm->bind( valueNotSmi );
                    _masm->hlt();
                }

                // hack for now - jcc so mapping stays alive must do all the mapping in the program path taken - otherwise mappings are inconsistent
                _masm->bind( exit );

                Register r = def( error );
                _masm->test( r, 0 );
                jcc( Assembler::Condition::notZero, node, node->next( 1 ) );
            }
        }
            break;
        default: ShouldNotReachHere();
    }
}


void CodeGenerator::anUncommonNode( UncommonNode *node ) {
    _currentMapping->saveRegisters();
    _currentMapping->killRegisters();
    updateDebuggingInfo( node );
    _masm->call( StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type );
    setMapping( nullptr );
}


void CodeGenerator::aFixedCodeNode( FixedCodeNode *node ) {
    switch ( node->kind() ) {
        case FixedCodeNode::FixedCodeKind::dead_end:
            _masm->hlt();
            setMapping( nullptr );
            break;
        case FixedCodeNode::FixedCodeKind::inc_counter:
            incrementInvocationCounter();
            break;
        default: st_fatal1( "unexpected FixedCodeNode kind %d", node->kind() );
    }
}
