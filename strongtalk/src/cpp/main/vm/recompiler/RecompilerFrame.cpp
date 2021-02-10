
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/recompiler/RecompilerFrame.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/interpreter/MethodClosure.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"

const RecompilerFrame *noCaller    = reinterpret_cast<RecompilerFrame *>(0x1 );        // no caller (i.e., initial frame)
const RecompilerFrame *noCallerYet = reinterpret_cast<RecompilerFrame *>(0x0 );     // caller not yet computed

RecompilerFrame::RecompilerFrame( Frame frame, const RecompilerFrame *callee ) :
    _frame( frame ),
    _caller{ (RecompilerFrame *) noCallerYet },
    _callee{ (RecompilerFrame *) callee },
    _num{ callee ? callee->num() + 1 : 0 },
    _distance{ -1 },
    _invocations{ _sends = _cumulSends = _loopDepth = 0 },
    _ncallers{},
    _sends{},
    _cumulSends{},
    _loopDepth{} {
}


void RecompilerFrame::set_distance( std::int32_t d ) {
    _distance = d;
}


InterpretedRecompilerFrame::InterpretedRecompilerFrame( Frame fr, const RecompilerFrame *callee ) :
    RecompilerFrame( fr, callee ),
    _method{},
    _byteCodeIndex{},
    _receiverKlass{},
    _deltaVirtualFrame{ nullptr },
    _lookupKey{ nullptr } {

    VirtualFrame *vf1 = VirtualFrame::new_vframe( &_frame );
    st_assert( vf1->is_interpreted_frame(), "must be interpreted" );
    InterpretedVirtualFrame *vf = (InterpretedVirtualFrame *) vf1;
    _method = vf->method();
    st_assert( _method->codes() <= _frame.hp() and _frame.hp() < _method->codes_end(), "frame doesn't match method" );
    _byteCodeIndex     = vf->byteCodeIndex();
    _receiverKlass     = theRecompilation->receiverOf( vf )->klass();
    _deltaVirtualFrame = vf;
}


InterpretedRecompilerFrame::InterpretedRecompilerFrame( Frame fr, MethodOop m, KlassOop rcvrKlass ) :
    RecompilerFrame( fr, nullptr ),
    _method{ m },
    _byteCodeIndex{ PrologueByteCodeIndex },
    _receiverKlass{ rcvrKlass },
    _deltaVirtualFrame{ nullptr },
    _lookupKey{ nullptr } {

    st_assert( _method->codes() <= _frame.hp() and _frame.hp() < _method->codes_end(), "frame doesn't match method" );
    VirtualFrame *vf1 = VirtualFrame::new_vframe( &_frame );
    st_assert( vf1->is_interpreted_frame(), "must be interpreted" );

    InterpretedVirtualFrame *vf = (InterpretedVirtualFrame *) vf1;
    _deltaVirtualFrame = vf;

    init();

}


CompiledRecompilerFrame::CompiledRecompilerFrame( Frame fr, const RecompilerFrame *callee ) :
    RecompilerFrame( fr, callee ),
    _nativeMethod{},
    _deltaVirtualFrame{} {
}


CompiledRecompilerFrame::CompiledRecompilerFrame( Frame fr ) :
    RecompilerFrame( fr, nullptr ),
    _nativeMethod{},
    _deltaVirtualFrame{} {
    init();
}


RecompilerFrame *RecompilerFrame::new_RFrame( Frame frame, const RecompilerFrame *callee ) {

    RecompilerFrame *rf{ nullptr };
    std::int32_t    dist = callee ? callee->distance() : -1;

    if ( frame.is_interpreted_frame() ) {
        rf = new InterpretedRecompilerFrame( frame, callee );
        dist++;
    } else if ( frame.is_compiled_frame() ) {
        rf = new CompiledRecompilerFrame( frame, callee );
    } else {
        st_fatal( "not a Delta frame" );
    }
    rf->init();
    rf->set_distance( dist );
    return rf;
}


bool RecompilerFrame::is_blockMethod() const {
    return top_method()->is_blockMethod();
}


RecompilerFrame *RecompilerFrame::caller() {
    if ( _caller not_eq noCallerYet )
        return ( _caller == noCaller ) ? nullptr : _caller;    // already computed caller

    // caller not yet computed; do it now
    if ( _frame.is_first_delta_frame() ) {
        _caller = (RecompilerFrame *) noCaller;
        return nullptr;
    } else {
        _caller = new_RFrame( _frame.delta_sender(), this );
        return _caller;
    }
}


MethodOop CompiledRecompilerFrame::top_method() const {
    return _nativeMethod->method();
}


bool RecompilerFrame::is_super() const {
    if ( is_blockMethod() )
        return false;
    InlineCacheIterator *it = _frame.sender_ic_iterator();
    return it ? it->is_super_send() : false;
}


bool RecompilerFrame::hasBlockArgs() const {
    DeltaVirtualFrame *vf = top_vframe();
    if ( not vf )
        return false;
    std::int32_t       nargs = vf->method()->number_of_arguments();
    for ( std::int32_t i     = 0; i < nargs; i++ ) {
        Oop b = vf->argument_at( i );
        if ( b->is_block() )
            return true;
    }
    return false;
}


GrowableArray<BlockClosureOop> *RecompilerFrame::blockArgs() const {
    DeltaVirtualFrame              *vf     = top_vframe();
    std::int32_t                   nargs   = top_method()->number_of_arguments();
    GrowableArray<BlockClosureOop> *blocks = new GrowableArray<BlockClosureOop>( nargs );
    if ( not vf )
        return blocks;
    for ( std::size_t i = 0; i < nargs; i++ ) {
        Oop b = vf->argument_at( i );
        if ( b->is_block() )
            blocks->append( BlockClosureOop( b ) );
    }
    return blocks;
}


LookupKey *CompiledRecompilerFrame::key() const {
    return &_nativeMethod->_lookupKey;
}


LookupKey *InterpretedRecompilerFrame::key() const {
    if ( _lookupKey )
        return _lookupKey;            // cached result
    if ( _method->is_blockMethod() )
        return nullptr;    // has no lookup key
    SymbolOop sel = _method->selector();
    ( (InterpretedRecompilerFrame *) this )->_lookupKey = LookupKey::allocate( _receiverKlass, sel );
    // Note: this code should really be factored out somewhere; it's duplicated (at least) in LookupCache
    if ( is_super() ) {
        DeltaVirtualFrame *senderVF           = _deltaVirtualFrame ? _deltaVirtualFrame->sender_delta_frame() : DeltaProcess::active()->last_delta_vframe();
        MethodOop         sendingMethod       = senderVF->method()->home();
        KlassOop          sendingMethodHolder = _receiverKlass->klass_part()->lookup_method_holder_for( sendingMethod );
        if ( sendingMethodHolder ) {
            KlassOop superKlass = sendingMethodHolder->klass_part()->superKlass();
            st_assert( _method == LookupCache::method_lookup( superKlass, sel ), "inconsistent lookup result" );
            _lookupKey->initialize( superKlass, sel );
        } else {
            if ( WizardMode )
                SPDLOG_WARN( "sending method holder not found??" );
            ( (InterpretedRecompilerFrame *) this )->_lookupKey = nullptr;
        }
    } else {
        MethodOop method = LookupCache::compile_time_normal_lookup( _receiverKlass, sel );
        if ( _method not_eq method ) {
            if ( _method not_eq nullptr )
                _method->print();
            else
                _console->print( "method is null" );
            _console->cr();
            if ( method not_eq nullptr )
                method->print();
            else
                _console->print( "looked up method is null" );
        }
        st_assert( _method == LookupCache::compile_time_normal_lookup( _receiverKlass, sel ), "inconsistent lookup result" );
    }
    return _lookupKey;
}


std::int32_t InterpretedRecompilerFrame::cost() const {
    return _method->estimated_inline_cost( _receiverKlass );
}


std::int32_t CompiledRecompilerFrame::cost() const {
    return _nativeMethod->instructionsLength();
}


void CompiledRecompilerFrame::cleanupStaleInlineCaches() {
    _nativeMethod->cleanup_inline_caches();
}


void InterpretedRecompilerFrame::cleanupStaleInlineCaches() {
    _method->cleanup_inline_caches();
}


std::int32_t RecompilerFrame::computeSends( MethodOop m ) {

    // how many sends did m cause?  (rough approximation)
    // add up invocation counts of all methods called by m
    std::int32_t sends = 0;
    CodeIterator iter( m );

    do {
        switch ( iter.send() ) {
            case ByteCodes::SendType::INTERPRETED_SEND:
            case ByteCodes::SendType::COMPILED_SEND:
            case ByteCodes::SendType::POLYMORPHIC_SEND:
            case ByteCodes::SendType::PREDICTED_SEND:
            case ByteCodes::SendType::ACCESSOR_SEND: {
                InterpretedInlineCache         *ic = iter.ic();
                InterpretedInlineCacheIterator it( ic );
                while ( not it.at_end() ) {
                    std::int32_t count;
                    if ( it.is_compiled() ) {
                        sends += ( count = it.compiled_method()->invocation_count() );
                    } else {
                        sends += ( count = it.interpreted_method()->invocation_count() );
                    }
                    st_assert( count >= 0 and count <= 100 * 1000 * 1000, "bad invocation count" );
                    it.advance();
                }
            }
                break;

            case ByteCodes::SendType::MEGAMORPHIC_SEND:
                // don't know how to count MEGAMORPHIC sends; for now, just ignore them
                // because compiler can't eliminate them anyway
                break;
            case ByteCodes::SendType::PRIMITIVE_SEND  : // send to method containing predicted primitive
            case ByteCodes::SendType::NO_SEND:
                break;
            default: st_fatal1( "unexpected send type 0x%08x", iter.send() );
        }
    } while ( iter.advance() );

    return sends;
}


static CompiledRecompilerFrame *this_rframe   = nullptr;
static std::int32_t            sum_ics_result = 0;


static void sum_ics( CompiledInlineCache *ic ) {
    // estimate ic's # of sends and add to sum_ics_result
    // This code is here and not in CompiledInlineCacheIterator because it contains policy decisions,
    // e.g. how to attribute sends for nativeMethods w/multiple callers or w/o counters.
    if ( ic->is_empty() )
        return;      // no sends
    CompiledInlineCacheIterator it( ic );
    while ( not it.at_end() ) {
        if ( it.is_compiled() ) {
            sum_ics_result += it.compiled_method()->invocation_count();
        } else {
            sum_ics_result += it.interpreted_method()->invocation_count();
        }
        it.advance();
    }
}


void CompiledRecompilerFrame::init() {
    VirtualFrame *vf = VirtualFrame::new_vframe( &_frame );
    st_assert( vf->is_compiled_frame(), "must be compiled" );
    _nativeMethod = ( (CompiledVirtualFrame *) vf )->code();
    _nativeMethod->verify();
    vf = vf->top();

    st_assert( vf->is_compiled_frame(), "must be compiled" );
    _deltaVirtualFrame = (DeltaVirtualFrame *) vf;
    _invocations       = _nativeMethod->invocation_count();
    _ncallers          = _nativeMethod->number_of_links();
    this_rframe        = this;
    sum_ics_result     = 0;
    _nativeMethod->CompiledICs_do( sum_ics );
    _sends += sum_ics_result;
}


void InterpretedRecompilerFrame::init() {

    // find the InlineCache
    if ( _byteCodeIndex not_eq PrologueByteCodeIndex ) {
        CodeIterator iter( _method );
        while ( byteCodeIndexLT( iter.byteCodeIndex(), _byteCodeIndex ) ) {
            switch ( iter.loopType() ) {
                case ByteCodes::LoopType::LOOP_START:
                    _loopDepth++;
                    break;
                case ByteCodes::LoopType::LOOP_END:
                    _loopDepth--;
                    break;
                case ByteCodes::LoopType::NO_LOOP:
                    break;
                default: st_fatal1( "unexpected loop type 0x%08x", iter.loopType() );
            }

            if ( not iter.advance() ) {
                break;
            }
        }
        st_assert( iter.byteCodeIndex() == _byteCodeIndex, "should have found exact byteCodeIndex" );
    }

    //
    _invocations = _method->invocation_count();
    _ncallers    = _method->sharing_count();
    _sends       = computeSends( _method );
    _cumulSends  = computeCumulSends( _method ) + _sends;
    _lookupKey   = nullptr;
}


// a helper class for computing cumulCost
class CumulCounter : public SpecializedMethodClosure {

public:
    MethodOop    method;            // the method currently being scanned for uplevel-accesses
    std::int32_t cumulSends;
    bool         top;


    CumulCounter( MethodOop m ) :
        method{ m },
        cumulSends{ 0 },
        top{ true } {
    }


    CumulCounter() = default;
    virtual ~CumulCounter() = default;
    CumulCounter( const CumulCounter & ) = default;
    CumulCounter &operator=( const CumulCounter & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void count() {
        if ( not top ) {
            cumulSends += RecompilerFrame::computeSends( method );
        }

        top = false;
        MethodIterator iter( method, this );
    }


    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) { // recursively search nested blocks
        static_cast<void>(type); // unused
        static_cast<void>(nofArgs); // unused

        MethodOop savedMethod = method;
        method = meth;
        count();
        method = savedMethod;
    }
};


std::int32_t RecompilerFrame::computeCumulSends( MethodOop m ) {
    CumulCounter c( m );
    c.count();
    return c.cumulSends;
}


void RecompilerFrame::print( const char *kind ) {

    SPDLOG_INFO( "{<16s} {:3d} {} {<15s}  invocations={:5d}  numberOfCallers={:3d}  sends={:6d}  cumulativeSends={:6d}  loopDepth={:2d}  cost={:4d}",
                 kind,
                 _num,
                 is_interpreted() ? "I" : "C",
                 top_method()->selector()->as_string(),
                 _invocations,
                 _ncallers,
                 _sends,
                 _cumulSends,
                 _loopDepth,
                 cost()
    );

}


void CompiledRecompilerFrame::print() {
    RecompilerFrame::print( "CompiledRecompilerFrame" );
}


void InterpretedRecompilerFrame::print() {
    RecompilerFrame::print( "InterpretedRecompilerFrame" );
}
