//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/oldCodeGenerator.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/utility/EventLog.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/RegisterAllocator.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oop/ContextOopDescriptor.hpp"




// -----------------------------------------------------------------------------

// Computes the byte offset from the beginning of an Oop
inline std::int32_t byteOffset( std::int32_t offset ) {
    st_assert( offset >= 0, "bad offset" );
    return offset * sizeof( Oop ) - MEMOOP_TAG;
}


static bool bb_needs_jump;
// true if basic block needs a jump at the end to its successor, false otherwise
// Note: most gen() nodes with more than one successor are implemented such that
//       next() is the fall-through case. If that's not the case, an extra jump
//       has to be generated (via endOfBasicBlock()). However, some of the nodes
//       do explicit jumps to all successors to accomodate for arbitrary node
//       reordering, in which case they may set the flag to false (it is auto-
//       matically set to true for each node).
// This flag should go away at soon as all node with more than one exit are
// implemented correctly (i.e., do all the jumping themselves).


void OldCodeGenerator::beginOfBasicBlock( Node *node ) {
    theMacroAssembler->bind( node->_label );
}


void OldCodeGenerator::endOfBasicBlock( Node *node ) {
    if ( bb_needs_jump and node->next() not_eq nullptr ) {
        theMacroAssembler->jmp( node->next()->_label );
    }
}


void OldCodeGenerator::beginOfNode( Node *node ) {
    st_unused( node ); // unused
    // assume that all nodes that may terminate a basic block need a jump at the end
    // (turned off for individual nodes by their gen() methods if no jump is needed
    // because they generate code patterns that generate the jumps already)
    bb_needs_jump = true;
}


void OldCodeGenerator::endOfNode( Node *node ) {
    st_unused( node ); // unused
    // nothing to do
}


void OldCodeGenerator::aPrologueNode( PrologueNode *node ) {
    node->gen();
}


void OldCodeGenerator::aLoadIntNode( LoadIntNode *node ) {
    node->gen();
}


void OldCodeGenerator::aLoadOffsetNode( LoadOffsetNode *node ) {
    node->gen();
}


void OldCodeGenerator::aLoadUplevelNode( LoadUplevelNode *node ) {
    node->gen();
}


void OldCodeGenerator::anAssignNode( AssignNode *node ) {
    node->gen();
}


void OldCodeGenerator::aStoreOffsetNode( StoreOffsetNode *node ) {
    node->gen();
}


void OldCodeGenerator::aStoreUplevelNode( StoreUplevelNode *node ) {
    node->gen();
}


void OldCodeGenerator::anArithRRNode( RegisterRegisterArithmeticNode *node ) {
    node->gen();
}


void OldCodeGenerator::aFloatArithRRNode( FloatArithRRNode *node ) {
    node->gen();
}


void OldCodeGenerator::aFloatUnaryArithNode( FloatUnaryArithNode *node ) {
    node->gen();
}


void OldCodeGenerator::anArithRCNode( ArithRCNode *node ) {
    node->gen();
}


void OldCodeGenerator::aTArithRRNode( TArithRRNode *node ) {
    node->gen();
}


void OldCodeGenerator::aContextCreateNode( ContextCreateNode *node ) {
    node->gen();
}


void OldCodeGenerator::aContextInitNode( ContextInitNode *node ) {
    node->gen();
}


void OldCodeGenerator::aContextZapNode( ContextZapNode *node ) {
    node->gen();
}


void OldCodeGenerator::aBlockCreateNode( BlockCreateNode *node ) {
    node->gen();
}


void OldCodeGenerator::aBlockMaterializeNode( BlockMaterializeNode *node ) {
    node->gen();
}


void OldCodeGenerator::aSendNode( SendNode *node ) {
    node->gen();
}


void OldCodeGenerator::aPrimitiveNode( PrimitiveNode *node ) {
    node->gen();
}


void OldCodeGenerator::aDLLNode( DLLNode *node ) {
    node->gen();
}


void OldCodeGenerator::aLoopHeaderNode( LoopHeaderNode *node ) {
    node->gen();
}


void OldCodeGenerator::aReturnNode( ReturnNode *node ) {
    node->gen();
}


void OldCodeGenerator::aNonLocalReturnSetupNode( NonLocalReturnSetupNode *node ) {
    node->gen();
}


void OldCodeGenerator::anInlinedReturnNode( InlinedReturnNode *node ) {
    node->gen();
}


void OldCodeGenerator::aNonLocalReturnContinuationNode( NonLocalReturnContinuationNode *node ) {
    node->gen();
}


void OldCodeGenerator::aBranchNode( BranchNode *node ) {
    node->gen();
}


void OldCodeGenerator::aTypeTestNode( TypeTestNode *node ) {
    node->gen();
}


void OldCodeGenerator::aNonLocalReturnTestNode( NonLocalReturnTestNode *node ) {
    node->gen();
}


void OldCodeGenerator::aMergeNode( MergeNode *node ) {
    node->gen();
}


void OldCodeGenerator::anArrayAtNode( ArrayAtNode *node ) {
    node->gen();
}


void OldCodeGenerator::anArrayAtPutNode( ArrayAtPutNode *node ) {
    node->gen();
}


void OldCodeGenerator::anInlinedPrimitiveNode( InlinedPrimitiveNode *node ) {
    node->gen();
}


void OldCodeGenerator::anUncommonNode( UncommonNode *node ) {
    node->gen();
}


void OldCodeGenerator::aFixedCodeNode( FixedCodeNode *node ) {
    node->gen();
}


void OldCodeGenerator::aNopNode( NopNode *node ) {
    node->gen();
}


void OldCodeGenerator::aCommentNode( CommentNode *node ) {
    node->gen();
}


//----------------------------------------------------------------------------------------------------
//
// Implementation of gen() nodes


// Inline caches
//
// An inline cache is implemented via a dummy instruction that
// follows the call immediately. The instruction's 32bit immediate
// value provides the icache information. The instruction itself
// does not modify the CPU state except the flags which are in
// an undefined state after a call, anyway.

static void inlineCache( Label &nlrTestPoint, SendInfo *info, bool super ) {
    // generates the inline cache information (must follow a call instruction immediately)
    char flags = 0;
    if ( super )
        setNthBit( flags, super_send_bit_no );
    if ( info and info->uninlinable )
        setNthBit( flags, uninlinable_bit_no );
    if ( info and info->_receiverStatic )
        setNthBit( flags, receiver_static_bit_no );
    theMacroAssembler->ic_info( nlrTestPoint, flags );
}


// Calls to C land
//
// When entering C land, the ebp & esp of the last Delta frame have to be recorded.
// When leaving C land, last_delta_fp has to be reset to 0. This is required to
// allow proper stack traversal.

static void call_C( const char *dest, RelocationInformation::RelocationType relocType, bool needsDeltaFPCode ) {
    if ( needsDeltaFPCode )
        theMacroAssembler->set_last_delta_frame_before_call();
    theMacroAssembler->call( dest, relocType );
    if ( needsDeltaFPCode )
        theMacroAssembler->reset_last_delta_frame();
}


static void call_C( const char *dest, RelocationInformation::RelocationType relocType, bool needsDeltaFPCode, Label &nlrTestPoint ) {
    if ( needsDeltaFPCode )
        theMacroAssembler->set_last_delta_frame_before_call();
    theMacroAssembler->call( dest, relocType );
    inlineCache( nlrTestPoint, nullptr, false );
    if ( needsDeltaFPCode )
        theMacroAssembler->reset_last_delta_frame();
}


// Routines for debugging
//
// The verifyXXX routines are called from within compiled code if the
// VerifyCode flag is set. The routines do plausibility checks on objects
// and trap in case of an error. The verifyXXXCode routines are used to
// generate the transparent call stubs for the verifyXXX's.

static std::int32_t callDepth     = 0;    // to indent tracing messages
static std::int32_t numberOfCalls = 0;    // # of traced calls since start

static void indent() {
    const std::int32_t maxIndent = 30;
    if ( callDepth < maxIndent ) {
        SPDLOG_INFO( "%*s", callDepth, " " );
    } else {
        SPDLOG_INFO( "%*s <%5d>", maxIndent - 9, " ", callDepth );
    }
}


static const char *nativeMethodName() {
    DeltaVirtualFrame *f = DeltaProcess::active()->last_delta_vframe();
    return f->method()->selector()->as_string();
}


static void breakpointCode() {
    // generates a transparent call to a breakpoint routine where
    // a breakpoint can be set - for debugging purposes only
    if ( not VerifyCode )
        SPDLOG_WARN( ": breakpoint should not be called" );
    theMacroAssembler->pushad();
    call_C( (const char *) breakpoint, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->popad();
}


static void verifyOopCode( Register reg ) {
    // generates transparent check code which test the contents of
    // reg for the mark bit and halts if set - for debugging purposes only
    if ( not VerifyCode )
        SPDLOG_WARN( ": verifyOop should not be called" );
    Label L;
    theMacroAssembler->test( reg, MARK_TAG_BIT );
    theMacroAssembler->jcc( Assembler::Condition::zero, L );
    theMacroAssembler->hlt();
    theMacroAssembler->bind( L );
}


extern "C" void verifyContext( Oop obj ) {
    // verify entire context chain
    ContextOop ctx = ContextOop( obj );
    while ( 1 ) {
        if ( ctx->isMarkOop() )
            error( "context should never be mark" );
        if ( not Universe::is_heap( (Oop *) ctx ) )
            error( "context outside of heap" );
        if ( not ctx->is_context() )
            error( "should be a context" );
        if ( ctx->unoptimized_context() not_eq nullptr ) {
            error( "context has been deoptimized -- shouldn't use in compiled code" );
        }
        if ( not ctx->has_outer_context() )
            break;
        ctx = ctx->outer_context();
    }
}


static void verifyContextCode( Register reg ) {
    // generates transparent check code which verifies that reg contains
    // a legal context and halts if not - for debugging purposes only
    if ( not VerifyCode ) {
        SPDLOG_WARN( ": verifyContext should not be called" );
    }
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( reg );    // pass argument (C calling convention)
    call_C( (const char *) verifyContext, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, OOP_SIZE );   // get rid of argument
    theMacroAssembler->popad();
}


extern "C" void verifyNilOrContext( Oop obj ) {
    if ( obj not_eq nilObject )
        verifyContext( obj );
}


static void verifyNilOrContextCode( Register reg ) {
    // generates transparent check code which verifies that reg contains
    // a legal context and halts if not - for debugging purposes only
    if ( not VerifyCode )
        SPDLOG_WARN( ": verifyNilOrContext should not be called" );
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( reg );    // pass argument (C calling convention)
    call_C( (const char *) verifyNilOrContext, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, OOP_SIZE );   // get rid of argument
    theMacroAssembler->popad();
}


extern "C" void verifyBlock( BlockClosureOop blk ) {
    blk->verify();
}


static void verifyBlockCode( Register reg ) {
    // generates transparent check code which verifies that reg contains
    // a legal context and halts if not - for debugging purposes only
    if ( not VerifyCode )
        SPDLOG_WARN( ": verifyBlockCode should not be called" );
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( reg );    // pass argument (C calling convention)
    call_C( (const char *) verifyBlock, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, OOP_SIZE );   // get rid of argument
    theMacroAssembler->popad();
}


static std::int32_t NumberOfReturns = 0;      // for debugging (conditional breakpoints)

extern "C" void verifyReturn( Oop obj ) {
    NumberOfReturns++;
    obj->verify();
    if ( TraceCalls ) {
        ResourceMark resourceMark;
        callDepth--;
        indent();
        SPDLOG_INFO( "return [{}] from [{}]", obj->print_value_string(), nativeMethodName() );
    }
}


static void verifyReturnCode( Register reg ) {
    // generates transparent check code which verifies that reg contains
    // a legal context and halts if not - for debugging purposes only
    if ( not VerifyCode and not GenTraceCalls ) {
        SPDLOG_WARN( ": verifyReturn should not be called" );
    }
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( reg );    // pass argument (C calling convention)
    call_C( (const char *) verifyReturn, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, OOP_SIZE );   // get rid of argument
    theMacroAssembler->popad();
}


extern "C" void verifyNonLocalReturn( const char *fp, char *nlrFrame, std::int32_t nlrScopeID, Oop nlrResult ) {
    st_unused( nlrScopeID ); // unused

    SPDLOG_INFO( "verifyNonLocalReturn(0x{0:x}, 0x{0:x}, {:d}, 0x{0:x})", static_cast<const void *>( fp ), static_cast<const void *>( nlrFrame ), static_cast<const void *>( nlrResult ) );
    if ( nlrFrame <= fp ) {
        error( "NonLocalReturn went too far: 0x{0:x} <= 0x{0:x}", nlrFrame, fp );
    }
    // treat >99 scopes as likely error -- might actually be ok
//  if (nlrScopeID < 0 or nlrScopeID > 99) error("illegal NonLocalReturn scope ID 0x{0:x}", nlrScopeID);
    if ( nlrResult->isMarkOop() )
        error( "NonLocalReturn result is a markOop" );
    if ( TraceCalls ) {
        ResourceMark resourceMark;
        callDepth--;
        indent();
        SPDLOG_INFO( "NonLocalReturn [{}] from/thru [{}]", nlrResult->print_value_string(), nativeMethodName() );
    }
}


static void verifyNonLocalReturnCode() {
    // generates transparent check code which verifies NonLocalReturn check & continuation
    if ( not VerifyCode )
        SPDLOG_WARN( ": verifyNonLocalReturnCode should not be called" );
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( Mapping::asRegister( NonLocalReturnResultLoc ) );    // pass argument (C calling convention)
    theMacroAssembler->pushl( Mapping::asRegister( NonLocalReturnHomeIdLoc ) );
    theMacroAssembler->pushl( Mapping::asRegister( NonLocalReturnHomeLoc ) );
    theMacroAssembler->pushl( ebp );
    call_C( (const char *) verifyNonLocalReturn, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, 4 * OOP_SIZE );   // get rid of arguments
    theMacroAssembler->popad();
}


extern "C" void verifySmi( Oop obj ) {
    if ( not obj->isSmallIntegerOop() ) st_fatal( "should be a small_int_t" );
}


static void verifySmiCode( Register reg ) {
    // generates transparent check code which verifies that reg contains
    // a legal small_int_t and halts if not - for debugging purposes only
    if ( not VerifyCode )
        SPDLOG_WARN( ": verifySmi should not be called" );
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( reg );    // pass argument (C calling convention)
    call_C( (const char *) verifySmi, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, OOP_SIZE );   // get rid of argument
    theMacroAssembler->popad();
}


extern "C" void verifyObject( Oop obj ) {
    if ( not obj->isSmallIntegerOop() and not obj->isMemOop() ) st_fatal( "should be an ordinary Oop" );
    KlassOop klass = obj->klass();
    if ( klass == nullptr or not klass->isMemOop() ) st_fatal( "should be an ordinary MemOop" );
    if ( obj->is_block() )
        BlockClosureOop( obj )->verify();
}


static void verifyObjCode( Register reg ) {
    // generates transparent check code which verifies that reg contains
    // a legal Oop and halts if not - for debugging purposes only
    if ( not VerifyCode )
        SPDLOG_WARN( ": verifyObject should not be called" );
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( reg );    // pass argument (C calling convention)
    call_C( (const char *) verifyObject, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, OOP_SIZE );   // get rid of argument
    theMacroAssembler->popad();
}


extern "C" void verifyArguments( Oop recv, std::int32_t ebp, std::int32_t nofArgs ) {
    ResourceMark resourceMark;
    numberOfCalls++;
    if ( TraceCalls ) {
        callDepth++;
        indent();
        SPDLOG_INFO( "calling {} {} ", nativeMethodName(), recv->print_value_string() );
    }
    verifyObject( recv );
    std::int32_t i    = nofArgs;
    Oop          *arg = (Oop *) ( ebp + ( nofArgs + 2 ) * OOP_SIZE );
    while ( i-- > 0 ) {
        arg--;
        verifyObject( *arg );
        if ( TraceCalls ) {
            ResourceMark resourceMark;
            SPDLOG_INFO( "{}, ", ( *arg )->print_value_string() );
        }
    }
    if ( VerifyDebugInfo ) {
        DeltaVirtualFrame *f = DeltaProcess::active()->last_delta_vframe();
        while ( f not_eq nullptr ) {
            f->verify_debug_info();
            f = f->sender_delta_frame();
        }
    }
}


static void verifyArgumentsCode( Register recv, std::int32_t nofArgs ) {
    // generates transparent check code which verifies that all arguments
    // are legal oops and halts if not - for debugging purposes only
    st_assert( VerifyCode or GenTraceCalls or VerifyDebugInfo, "performance bug: verifyArguments should not be called" );
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( nofArgs );    // pass arguments (C calling convention)
    theMacroAssembler->pushl( ebp );
    theMacroAssembler->pushl( recv );
    call_C( (const char *) verifyArguments, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, 3 * OOP_SIZE );   // get rid of arguments
    theMacroAssembler->popad();
}


static std::int32_t result_counter = 0;


static void trace_result( std::int32_t compilation, MethodOop method, Oop result ) {
    ResourceMark resourceMark;
    _console->print( "%6d: 0x%08x (compilation %4d, ", result_counter++, std::int32_t( result ), compilation );
    method->selector()->print_value();
    SPDLOG_INFO( ")", compilation );
}


static void call_trace_result( Register result ) {
    theMacroAssembler->pushad();
    theMacroAssembler->pushl( result );
    theMacroAssembler->pushl( theCompiler->method );
    theMacroAssembler->pushl( compilationCount );
    call_C( (const char *) trace_result, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->addl( esp, 3 * OOP_SIZE );   // get rid of arguments
    theMacroAssembler->popad();
}


// helper functions for loading/storing

static Register moveLocToReg( Location src, Register temp ) {
    // Returns the register corresponding to src if src is a register location,
    // otherwise loads the value at loc into register temp and returns temp.
    if ( src.isRegisterLocation() ) {
        return Mapping::asRegister( src );
    } else {
        Mapping::load( src, temp );
        return temp;
    }
}


static Register movePseudoRegisterToReg( PseudoRegister *src, Register temp ) {
    // Returns the src register if a register has been assigned to src,
    // otherwise loads the value src into register temp and returns temp.
    if ( src->isConstPseudoRegister() ) {
        theMacroAssembler->movl( temp, ( (ConstPseudoRegister *) src )->constant );
        return temp;
    } else {
        return moveLocToReg( src->_location, temp );
    }
}


static inline Register answerLocReg( Location src, Register temp ) {
    // Returns the register corresponding to src if src is a register location,
    // otherwise returns temp.
    return src.isRegisterLocation() ? Mapping::asRegister( src ) : temp;
}


static inline Register answerPseudoRegisterReg( PseudoRegister *src, Register temp ) {
    // Returns the src register if a register has been assigned to src,
    // otherwise returns temp.
    return answerLocReg( src->_location, temp );
}


static void load( PseudoRegister *src, Register dst ) {
    // Loads src into register dst.
    if ( src->isConstPseudoRegister() ) {
        theMacroAssembler->movl( dst, ( (ConstPseudoRegister *) src )->constant );
    } else {
        Mapping::load( src->_location, dst );
    }
}


static void fload( PseudoRegister *src, Register base, Register temp ) {
    st_assert( base not_eq temp, "registers must be different" );
    // Loads src into FPU ST
    if ( src->isConstPseudoRegister() ) {
        theMacroAssembler->movl( temp, ( (ConstPseudoRegister *) src )->constant );
        theMacroAssembler->fld_d( Address( temp, byteOffset( DoubleOopDescriptor::value_offset() ) ) ); // unbox float
    } else {
        Mapping::fload( src->_location, base );
    }
}


static void store( Register src, PseudoRegister *dst, Register temp1, Register temp2, bool needsStoreCheck = true ) {
    // Stores register src to dst.
    st_assert( not dst->isConstPseudoRegister(), "destination cannot be a constant" );
    Mapping::store( src, dst->_location, temp1, temp2, needsStoreCheck );
}


static void fstore( PseudoRegister *dst, Register base ) {
    // Stores FPU ST to dst and pops ST
    st_assert( not dst->isConstPseudoRegister(), "destination cannot be a constant" );
    Mapping::fstore( dst->_location, base );
}


static void storeO( ConstPseudoRegister *src, PseudoRegister *dst, Register temp1, Register temp2, bool needsStoreCheck = true ) {
    // Stores constant src to dst.
    st_assert( not dst->isConstPseudoRegister(), "destination cannot be a constant" );
    Mapping::storeO( src->constant, dst->_location, temp1, temp2, needsStoreCheck );
}


// The float section on the stack is 8byte-aligned. In order to access it, a base register
// instead of ebp is used. This base register holds the 8byte aligned ebp value. For now,
// all node accessing floats are using the same base register (temp3); this allows us to
// get rid of unneccessary base register setup code if the previous node set it up already.

static void set_floats_base( Node *node, Register base, bool enforce = false ) {
    // Stores aligned ebp value into base
    st_assert( SIZEOF_FLOAT == 8, "check this code" );
    st_assert( node->isAccessingFloats(), "must be a node accessing floats" );
    st_assert( base == temp3, "check this code" );
    if ( node->hasSinglePredecessor() and node->firstPrev()->isAccessingFloats() and not enforce ) {
        // previous node is also accessing floats => base register is
        // already set => no extra code necessary
    } else {
        theMacroAssembler->movl( base, ebp );
        theMacroAssembler->andl( base, -SIZEOF_FLOAT );
    }
}


static void assign( Node *node, PseudoRegister *src, PseudoRegister *dst, Register temp1, Register temp2, Register temp3, bool needsStoreCheck = true ) {
    // General assignment
    st_assert( temp1 not_eq temp2 and temp1 not_eq temp3 and temp2 not_eq temp3, "registers must be different" );
    if ( src->_location not_eq dst->_location ) {
        if ( node->isAccessingFloats() ) {
            Register base = temp3;
            set_floats_base( node, base );
            fload( src, base, temp1 );
            fstore( dst, base );
        } else if ( src->isConstPseudoRegister() ) {
            // assign constants directly without loading into temporary register first
            storeO( (ConstPseudoRegister *) src, dst, temp1, temp2, needsStoreCheck );
        } else {
            Register t = movePseudoRegisterToReg( src, answerPseudoRegisterReg( dst, temp1 ) );
            store( t, dst, temp2, temp3, needsStoreCheck );
        }
    }
}


static Register uplevelBase( PseudoRegister *startContext, std::int32_t nofLevels, Register temp ) {
    // Compute uplevel base; nofLevels is number of indirections (0 = in this context)
    Register b = nofLevels > 0 ? temp : answerPseudoRegisterReg( startContext, temp );
    load( startContext, b );
    while ( nofLevels-- > 0 ) {
        if ( VerifyCode )
            verifyContextCode( b );
        theMacroAssembler->Load( b, ContextOopDescriptor::parent_byte_offset(), b );
    }
    return b;
}


// Code generation for statistical information on nativeMethods

static const char *nativeMethodAddr() {
    // hack to compute hypothetical NativeMethod address
    // should be fixed at some point
    return (const char *) ( ( (NativeMethod *) ( theMacroAssembler->code()->code_begin() ) ) - 1 );
}


static void incCounter() {
    // Generates code to increment the NativeMethod execution counter
    const char *addr = nativeMethodAddr() + NativeMethod::invocationCountOffset();
    theMacroAssembler->incl( Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ) );
}


// Helper functions for general code generation

static Assembler::Condition mapToCC( BranchOpCode op ) {
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
        default: ShouldNotReachHere();
            return Assembler::Condition::zero;
    }
}


static void primitiveCall( InlinedScope *scope, PrimitiveDescriptor *pdesc ) {
    if ( pdesc->can_perform_NonLocalReturn() ) {
        call_C( (const char *) ( pdesc->fn() ), RelocationInformation::RelocationType::primitive_type, pdesc->needs_delta_fp_code(), scope->nlrTestPoint()->_label );
    } else {
        call_C( (const char *) ( pdesc->fn() ), RelocationInformation::RelocationType::primitive_type, pdesc->needs_delta_fp_code() );
    }
}


static void zapContext( PseudoRegister *context, Register temp ) {
    Register c = movePseudoRegisterToReg( context, temp );
    theMacroAssembler->movl( Address( c, ContextOopDescriptor::parent_byte_offset() ), 0 );
}


static void continueNonLocalReturn( Register temp1, Register temp2 ) {
    st_assert( temp1 == ::temp1 and temp2 == ::temp2, "different register usage than stub routine - check this" );
    if ( VerifyCode )
        verifyNonLocalReturnCode();
    theMacroAssembler->jmp( StubRoutines::continue_NonLocalReturn_entry(), RelocationInformation::RelocationType::runtime_call_type );
}


// Code generation for individual nodes

void BasicNode::gen() {
    ScopeDescriptorRecorder *rec = theCompiler->scopeDescRecorder();
    rec->addProgramCounterDescriptor( theMacroAssembler->offset(), _scope->getScopeInfo(), _byteCodeIndex );
}


static void checkRecompilation( Label &recompile_stub_call, Register t ) {
    if ( RecompilationPolicy::needRecompileCounter( theCompiler ) ) {
        // increment the NativeMethod execution counter and check limit
        const char *addr = nativeMethodAddr() + NativeMethod::invocationCountOffset();
        theMacroAssembler->movl( t, Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ) );
        theMacroAssembler->incl( t );
        theMacroAssembler->cmpl( t, theCompiler->get_invocation_counter_limit() );
        theMacroAssembler->movl( Address( std::int32_t( addr ), RelocationInformation::RelocationType::internal_word_type ), t );
        theMacroAssembler->jcc( Assembler::Condition::greaterEqual, recompile_stub_call );
    }
}


static void verify_context_chain( Register closure, std::int32_t chain_length, const Register & temp1, const Register & temp2 ) {
    // Generates code to verify the context chain of a block closure.
    // If the chain contains deoptimized contextOops, the block has to be deoptimized as well.
    // Method: A bit in the mark field of each context indicates whether it has been deoptimized or not.
    //         All mark fields of the contexts in the context chain are or-ed together and the bit is checked at the end.
    st_assert( closure not_eq temp1 and closure not_eq temp2 and temp1 not_eq temp2, "registers must be different" );
    st_assert( chain_length >= 1, "must have at least one context in the chain" );

    const Register context{ temp1 };
    const Register sum{ temp2 };

    // initialize sum with mark of first context
    theMacroAssembler->movl( context, Address( closure, BlockClosureOopDescriptor::context_byte_offset() ) );
    theMacroAssembler->movl( sum, Address( context, MemOopDescriptor::mark_byte_offset() ) );

    // 'or' the mark fields of the remaining contexts in the chain to sum
    for ( std::size_t i = chain_length - 1; i-- > 0; ) {
        theMacroAssembler->movl( context, Address( context, ContextOopDescriptor::parent_byte_offset() ) );
        theMacroAssembler->orl( sum, Address( context, MemOopDescriptor::mark_byte_offset() ) );
    }
    // check if there was any context that has been deoptimized
    theMacroAssembler->testl( sum, MarkOopDescriptor::context_forward_bit_mask() );
    theMacroAssembler->jcc( Assembler::Condition::notZero, StubRoutines::deoptimize_block_entry(), RelocationInformation::RelocationType::runtime_call_type );
    // otherwise continue
}


extern "C" void check_stack_overflow();
extern "C" const char *active_stack_limit();


void PrologueNode::gen() {

    BasicNode::gen();

    // stub for handling stack overflows
    Label handle_stack_overflow, continue_after_stack_overflow;
    theMacroAssembler->bind( handle_stack_overflow );
    theMacroAssembler->call_C( (const char *) &check_stack_overflow, RelocationInformation::RelocationType::runtime_call_type );
    theMacroAssembler->jmp( continue_after_stack_overflow );

    // call to recompiler - if the NativeMethod turns zombie, this will be overwritten by a call to the zombie handler
    // (see also comment in NativeMethod)
    Label recompile_stub_call;
    theMacroAssembler->bind( recompile_stub_call );
    theCompiler->set_special_handler_call_offset( theMacroAssembler->offset() );
    theMacroAssembler->call( StubRoutines::recompile_stub_entry(), RelocationInformation::RelocationType::runtime_call_type );

    // entry point for callers who need to verify receiver klass or if block
    theMacroAssembler->align( OOP_SIZE );
    theCompiler->set_entry_point_offset( theMacroAssembler->offset() );
    Register recv = moveLocToReg( selfLoc, temp1 );
    if ( scope()->isMethodScope() ) {
        // check class
        KlassOop klass = _scope->selfKlass();
        if ( klass == smiKlassObject ) {
            // receiver must be a small_int_t, check small_int_t tag only
            theMacroAssembler->test( recv, MEMOOP_TAG );
            theMacroAssembler->jcc( Assembler::Condition::notZero, CompiledInlineCache::normalLookupRoutine() );
        } else {
            // receiver could be a small_int_t, check small_int_t tag before loading class
            theMacroAssembler->test( recv, MEMOOP_TAG );
            theMacroAssembler->jcc( Assembler::Condition::zero, CompiledInlineCache::normalLookupRoutine() );
            theMacroAssembler->cmpl( Address( recv, MemOopDescriptor::klass_byte_offset() ), klass );
            theMacroAssembler->jcc( Assembler::Condition::notEqual, CompiledInlineCache::normalLookupRoutine() );
        }
    } else {
        // If this is a block method and we expect a context then the incoming context chain must be checked.
        // The context chain may contain a deoptimized contextOop. (see StubRoutines::verify_context_chain for details)
        if ( scope()->method()->block_info() == MethodOopDescriptor::expects_context ) {
            const bool use_fast_check = false;            // turn this off if it doesn't work
            if ( use_fast_check ) {
                // What happens if the context chain is not anchored in a method?
                // Probably doesn't work correctly - think about this - gri 6/26/96
                // Turned off for now - because of problems. Should fix this.
                std::size_t length = _scope->homeContext() + 1;            // includes context created within this scope
                if ( scope()->allocatesCompiledContext() )
                    length--;    // context has not been created yet -> adjust length
                verify_context_chain( recv, length, temp2, temp3 );
            } else {
                theMacroAssembler->call( StubRoutines::verify_context_chain(), RelocationInformation::RelocationType::runtime_call_type );
            }
        }
    }

    // callers who know the receiver class (e.g., PICs) should enter here
    theMacroAssembler->align( OOP_SIZE );
    theCompiler->set_verified_entry_point_offset( theMacroAssembler->offset() );
    // build stack frame
    std::int32_t frame_size = 2;    // return address & old ebp
    theMacroAssembler->enter();
    // allocate float temporaries
    std::int32_t nofFloats = theCompiler->totalNofFloatTemporaries();
    if ( nofFloats > 0 ) {
        st_assert( SIZEOF_FLOAT == OOP_SIZE * 2, "check this code" );
        st_assert( first_float_offset == -4, "check this code" );
        std::int32_t float_section_size = nofFloats * ( SIZEOF_FLOAT / OOP_SIZE ) + 2;    // 2 additional words for filler & float alignment
        frame_size += 1 + float_section_size;            // magic word & floats
        theMacroAssembler->pushl( Floats::magic );                // magic word
        theMacroAssembler->subl( esp, float_section_size * OOP_SIZE );    // add one word for float alignment
        theCompiler->set_float_section_size( float_section_size );
        theCompiler->set_float_section_start_offset( -2 );        // float_section after ebp & magic word
    }
    // allocate normal temporaries
    std::int32_t nofTemps  = theRegisterAllocator->nofStackTemps();
    if ( nofTemps > 0 ) {
        st_assert( first_temp_offset == -1, "check this code" );
        frame_size += nofTemps;
        theMacroAssembler->movl( temp2, nilObject );
        for ( std::size_t i = 0; i < nofTemps; i++ )
            theMacroAssembler->pushl( temp2 );
    }
    // make sure frame is big enough for deoptimization
    if ( frame_size < minimum_size_for_deoptimized_frame ) {
        if ( nofTemps == 0 )
            theMacroAssembler->movl( temp2, nilObject );    // make sure temp2 holds nil
        while ( frame_size < minimum_size_for_deoptimized_frame ) {
            frame_size++;
            theMacroAssembler->pushl( temp2 );
        }
    }

    if ( VerifyCode or VerifyDebugInfo or GenTraceCalls )
        verifyArgumentsCode( recv, scope()->method()->number_of_arguments() );

    // initialize self and context (for blocks)
    // recv has already been loaded (possibly into temp1)
    if ( scope()->isMethodScope() ) {
        store( recv, scope()->self()->pseudoRegister(), temp2, temp3 );
    } else {
        // recv contains block closure -> get context out of it
        Register c = answerPseudoRegisterReg( scope()->self()->pseudoRegister(), temp2 );
        theMacroAssembler->Load( recv, BlockClosureOopDescriptor::context_byte_offset(), c );
        store( c, scope()->self()->pseudoRegister(), temp1, temp3 );
        store( c, scope()->context(), temp1, temp3 );

        if ( VerifyCode ) {
            switch ( scope()->method()->block_info() ) {
                case MethodOopDescriptor::expects_nil:
                    verifyNilOrContextCode( c );
                    break;
                case MethodOopDescriptor::expects_self     :
                    [[fallthrough]];
                case MethodOopDescriptor::expects_parameter:
                    verifyOopCode( c );
                    break;
                case MethodOopDescriptor::expects_context:
                    verifyContextCode( c );
                    break;
                default: ShouldNotReachHere();
            }
        }
    }
    // check for recompilation (do this last so stack frame is initialized properly)
    checkRecompilation( recompile_stub_call, temp2 );

    theMacroAssembler->cmpl( esp, Address( std::int32_t( active_stack_limit() ), RelocationInformation::RelocationType::external_word_type ) );
    theMacroAssembler->jcc( Assembler::Condition::less, handle_stack_overflow );
    theMacroAssembler->bind( continue_after_stack_overflow );
}


void LoadIntNode::gen() {
    BasicNode::gen();
    Register t = answerPseudoRegisterReg( _dest, temp1 );
    theMacroAssembler->movl( t, _value );
    store( t, _dest, temp2, temp3 );
}


void LoadOffsetNode::gen() {
    BasicNode::gen();
    Register b = movePseudoRegisterToReg( _src, temp1 );
    Register t = answerPseudoRegisterReg( _dest, temp2 );
    theMacroAssembler->Load( b, byteOffset( _offset ), t );
    store( t, _dest, temp1, temp3 );
}


void LoadUplevelNode::gen() {
    BasicNode::gen();
    Register b = uplevelBase( _context0, _nofLevels, temp1 );
    Register t = answerPseudoRegisterReg( _dest, temp2 );
    theMacroAssembler->Load( b, byteOffset( _offset ), t );
    if ( VerifyCode )
        verifyObjCode( t );
    store( t, _dest, temp1, temp3 );
}


void StoreOffsetNode::gen() {
    BasicNode::gen();
    Register b = movePseudoRegisterToReg( _base, temp1 );
    Register t = movePseudoRegisterToReg( _src, temp2 );
    theMacroAssembler->Store( t, b, byteOffset( _offset ) );
    if ( _needsStoreCheck ) {
        // NB: make sure b is a copy of base because storeCheck overwrites it (was bug 3/9/96 -Urs)
        if ( b not_eq temp1 )
            load( _base, temp1 );
        theMacroAssembler->store_check( temp1, temp2 );
    }
}


void StoreUplevelNode::gen() {
    StoreNode::gen();
    Register b = uplevelBase( _context0, _nofLevels, temp1 );
    Register t = movePseudoRegisterToReg( _src, temp2 );
    theMacroAssembler->Store( t, b, byteOffset( _offset ) );
    if ( _needsStoreCheck ) {
        // NB: make sure b is a copy of _context0 because storeCheck overwrites it
        if ( b not_eq temp1 )
            load( _context0, temp1 );
        theMacroAssembler->store_check( temp1, temp2 );
    }
}


void MergeNode::gen() {
    BasicNode::gen();
    // nothing to do
}


void SendNode::gen() {
    BasicNode::gen();
    if ( isCounting() )
        incCounter();
    const char *entry = _superSend ? CompiledInlineCache::superLookupRoutine() : CompiledInlineCache::normalLookupRoutine();
    theMacroAssembler->call( entry, RelocationInformation::RelocationType::ic_type );
    inlineCache( scope()->nlrTestPoint()->_label, _info, _superSend );
    st_assert( _dest->_location == resultLoc, "assignment missing" );
}


void PrimitiveNode::gen() {
    BasicNode::gen();
    primitiveCall( scope(), _pdesc );
    // assign result
    Register t = moveLocToReg( resultLoc, answerPseudoRegisterReg( _dest, temp1 ) );
    store( t, _dest, temp2, temp3 );
}


void DLLNode::gen() {
    BasicNode::gen();
    st_assert( temp1 == ebx and temp2 == ecx and temp3 == edx, "registers are no temps anymore -> fix parameter passing" );
    // determine entry point depending on whether a run-time lookup is needed or not
    // Note: do not do a DLL lookup at compile time since this may cause a call back.
    const char *entry = ( function() == nullptr ) ? StubRoutines::lookup_DLL_entry( async() ) : StubRoutines::call_DLL_entry( async() );
    // pass arguments for DLL_C_frame in registers
    // adjust this code if DLL_C_frame changes:
    // ebx: no. of arguments
    // ecx: address of last argument
    // edx: dll function entry point
    theMacroAssembler->movl( ebx, nofArguments() );    // setup no. of arguments
    theMacroAssembler->movl( ecx, esp );            // setup address of last argument
    // Compiled_DLLCache
    // This code pattern must correspond to the Compiled_DLLCache layout
    // (make sure assembler is not optimizing mov reg, 0 into xor reg, reg!)
    theMacroAssembler->movl( edx, std::int32_t( function() ) );    // part of Compiled_DLLCache
    theMacroAssembler->inline_oop( dll_name() );        // part of Compiled_DLLCache
    theMacroAssembler->inline_oop( function_name() );    // part of Compiled_DLLCache
    theMacroAssembler->call( entry, RelocationInformation::RelocationType::runtime_call_type );
    // Note: should also pop arguments in case of a NonLocalReturn, could become a problem
    //       if DLL is called within a loop - fix this at some point.
    inlineCache( scope()->nlrTestPoint()->_label, nullptr, false );
    // assign result
    theMacroAssembler->addl( esp, nofArguments() * OOP_SIZE );    // get rid of arguments
    Register t = moveLocToReg( resultLoc, answerPseudoRegisterReg( _dest, temp1 ) );
    store( t, _dest, temp2, temp3 );
}


static bool producesResult( ArithOpCode op ) {
    return ( op not_eq ArithOpCode::TestArithOp ) and ( op not_eq ArithOpCode::CmpArithOp ) and ( op not_eq ArithOpCode::tCmpArithOp );
}


static bool setupseudoRegisterister( PseudoRegister *dst, PseudoRegister *arg, ArithOpCode op, Register &x, Register t ) {
    // Sets up register x such that x := x op <some constant> corresponds to dst := arg op <some constant>.
    // If the temporary register t is used at all, x will be in t.
    // Returns true if op generated a result in x; returns false otherwise.
    bool result = producesResult( op );
    if ( result ) {
        // operation generates result, try to use as few registers as possible
        if ( ( dst->_location == arg->_location ) /* or lastUsageOf(arg) */ ) {
            // arg is not used anymore afterwards, can be overwritten
            x = movePseudoRegisterToReg( arg, t );
        } else {
            // have to load arg into a temporary register
            x = t;
            load( arg, t );
        }
    } else {
        // operation generates no result, use argument register directly
        st_assert( dst->isNoPseudoRegister(), "dst should be a noPseudoRegister" );
        x = movePseudoRegisterToReg( arg, t );
    }
    return result;
}


static bool setupseudoRegisteristers( PseudoRegister *dst, PseudoRegister *arg1, ArithOpCode op, PseudoRegister *arg2, Register &x, Register &y, Register t1, Register t2 ) {
    // Sets up registers x & y such that x := x op y corresponds to dst := arg1 op arg2.
    // If the temporary registers t1 & t2 are used at all, x will be in t1 and y in t2.
    // Returns true if op generated a result in x; returns false otherwise.
    st_assert( t1 not_eq t2, "registers should be different" );
    bool result = producesResult( op );
    if ( result ) {
        // operation generates result, try to use as few registers as possible
        if ( ( dst->_location == arg1->_location ) /* or lastUsageOf(arg1) */ ) {
            // arg1 is not used anymore afterwards, can be overwritten
            x = movePseudoRegisterToReg( arg1, t1 );
            y = movePseudoRegisterToReg( arg2, t2 );
        } else {
            // have to load arg1 into a temporary register
            x = t1;
            load( arg1, t1 );
            y = movePseudoRegisterToReg( arg2, t2 );
        }
    } else {
        // operation generates no result, use argument registers directly
        st_assert( dst->isNoPseudoRegister(), "dst should be a noPseudoRegister" );
        x = movePseudoRegisterToReg( arg1, t1 );
        y = movePseudoRegisterToReg( arg2, t2 );
    }
    return result;
}


static void arithRROp( ArithOpCode op, Register x, Register y ) {
    st_assert( INTEGER_TAG == 0, "check this code" );
    switch ( op ) {
        case ArithOpCode::TestArithOp:
            theMacroAssembler->testl( x, y );
            break;
        case ArithOpCode::tAddArithOp  :
            [[fallthrough]];
        case ArithOpCode::AddArithOp:
            theMacroAssembler->addl( x, y );
            break;
        case ArithOpCode::tSubArithOp  :
            [[fallthrough]];
        case ArithOpCode::SubArithOp:
            theMacroAssembler->subl( x, y );
            break;
        case ArithOpCode::tMulArithOp:
            theMacroAssembler->sarl( x, TAG_SIZE );
        case ArithOpCode::MulArithOp:
            theMacroAssembler->imull( x, y );
            break;
        case ArithOpCode::tDivArithOp  :
            [[fallthrough]];
        case ArithOpCode::DivArithOp  : Unimplemented();
            break;
        case ArithOpCode::tModArithOp  :
            [[fallthrough]];
        case ArithOpCode::ModArithOp  : Unimplemented();
            break;
        case ArithOpCode::tAndArithOp  :
            [[fallthrough]];
        case ArithOpCode::AndArithOp:
            theMacroAssembler->andl( x, y );
            break;
        case ArithOpCode::tOrArithOp   :
            [[fallthrough]];
        case ArithOpCode::OrArithOp:
            theMacroAssembler->orl( x, y );
            break;
        case ArithOpCode::tXOrArithOp  :
            [[fallthrough]];
        case ArithOpCode::XOrArithOp:
            theMacroAssembler->xorl( x, y );
            break;
        case ArithOpCode::tShiftArithOp: Unimplemented();
        case ArithOpCode::ShiftArithOp: Unimplemented();
        case ArithOpCode::tCmpArithOp  :
            [[fallthrough]];
        case ArithOpCode::CmpArithOp:
            theMacroAssembler->cmpl( x, y );
            break;
        default           : ShouldNotReachHere();
    }
}


static void arithRCOp( ArithOpCode op, Register x, std::int32_t y ) {
    st_assert( INTEGER_TAG == 0, "check this code" );
    switch ( op ) {
        case ArithOpCode::TestArithOp:
            theMacroAssembler->testl( x, y );
            break;
        case ArithOpCode::tAddArithOp  :
            [[fallthrough]];
        case ArithOpCode::AddArithOp:
            theMacroAssembler->addl( x, y );
            break;
        case ArithOpCode::tSubArithOp  :
            [[fallthrough]];
        case ArithOpCode::SubArithOp:
            theMacroAssembler->subl( x, y );
            break;
        case ArithOpCode::tMulArithOp:
            y = arithmetic_shift_right( y, TAG_SIZE );
        case ArithOpCode::MulArithOp:
            theMacroAssembler->imull( x, x, y );
            break;
        case ArithOpCode::tDivArithOp  :
            [[fallthrough]];
        case ArithOpCode::DivArithOp  : Unimplemented();
            break;
        case ArithOpCode::tModArithOp  :
            [[fallthrough]];
        case ArithOpCode::ModArithOp  : Unimplemented();
            break;
        case ArithOpCode::tAndArithOp  :
            [[fallthrough]];
        case ArithOpCode::AndArithOp:
            theMacroAssembler->andl( x, y );
            break;
        case ArithOpCode::tOrArithOp   :
            [[fallthrough]];
        case ArithOpCode::OrArithOp:
            theMacroAssembler->orl( x, y );
            break;
        case ArithOpCode::tXOrArithOp  :
            [[fallthrough]];
        case ArithOpCode::XOrArithOp:
            theMacroAssembler->xorl( x, y );
            break;
        case ArithOpCode::tShiftArithOp:
            if ( y < 0 ) {
                // shift right
                std::int32_t shift_count = ( ( -y ) >> TAG_SIZE ) % 32;
                theMacroAssembler->sarl( x, shift_count );
                theMacroAssembler->andl( x, -1 << TAG_SIZE );  // clear Tag bits
            } else if ( y > 0 ) {
                // shift left
                std::int32_t shift_count = ( ( +y ) >> TAG_SIZE ) % 32;
                theMacroAssembler->shll( x, shift_count );
            }
            break;
        case ArithOpCode::ShiftArithOp: Unimplemented();
        case ArithOpCode::tCmpArithOp  :
            [[fallthrough]];
        case ArithOpCode::CmpArithOp:
            theMacroAssembler->cmpl( x, y );
            break;
        default           : ShouldNotReachHere();
    }
}


static void arithROOp( ArithOpCode op, Register x, Oop y ) {
    st_assert( not y->isSmallIntegerOop(), "check this code" );
    switch ( op ) {
        case ArithOpCode::CmpArithOp:
            theMacroAssembler->cmpl( x, y );
            break;
        default           : ShouldNotReachHere();
    }
}


void TArithRRNode::gen() {
    BasicNode::gen();
    PseudoRegister *arg1 = _src;
    PseudoRegister *arg2 = _oper;
    if ( arg2->isConstPseudoRegister() ) {
        Oop y = ( (ConstPseudoRegister *) arg2 )->constant;
        st_assert( y->isSmallIntegerOop() == _arg2IsInt, "flag value inconsistent" );
        if ( _arg2IsInt ) {
            // perform operation
            Register x;
            bool     result = setupseudoRegisterister( _dest, arg1, _op, x, temp1 );
            if ( not _arg1IsInt ) {
                // tag check necessary for arg1
                theMacroAssembler->test( x, MEMOOP_TAG );
                theMacroAssembler->jcc( Assembler::Condition::notZero, next( 1 )->_label );
            }
            arithRCOp( _op, x, std::int32_t( y ) );            // y is SmallIntegerOop -> needs no relocation info
            if ( result )
                store( x, _dest, temp2, temp3 );
        } else {
            // operation fails always
            theMacroAssembler->jmp( next( 1 )->_label );
        }
    } else {
        Register x, y;
        bool     result = setupseudoRegisteristers( _dest, arg1, _op, arg2, x, y, temp1, temp2 );
        // check argument tags
        Register tags   = noreg;
        if ( _arg1IsInt ) {
            if ( _arg2IsInt ) {
                // both x & y are smis => no tag check necessary
            } else {
                // x is small_int_t => check y
                tags = y;
            }
        } else {
            if ( _arg2IsInt ) {
                // y is small_int_t => check x
                tags = x;
            } else {
                // check both x & y
                tags = temp3;
                theMacroAssembler->movl( tags, x );
                theMacroAssembler->orl( tags, y );
            }
        }
        if ( tags not_eq noreg ) {
            // check tags
            theMacroAssembler->test( tags, MEMOOP_TAG );
            theMacroAssembler->jcc( Assembler::Condition::notZero, next( 1 )->_label );
        }
        // perform operation
        arithRROp( _op, x, y );
        if ( result ) {
            Register t = ( x == temp1 ) ? temp2 : temp1;
            store( x, _dest, t, temp3 );
        }
    }
}


void RegisterRegisterArithmeticNode::gen() {
    BasicNode::gen();
    PseudoRegister *arg1 = _src;
    PseudoRegister *arg2 = _oper;
    if ( arg2->isConstPseudoRegister() ) {
        Oop      y      = ( (ConstPseudoRegister *) arg2 )->constant;
        Register x;
        bool     result = setupseudoRegisterister( _dest, arg1, _op, x, temp1 );
        if ( y->isSmallIntegerOop() ) {
            arithRCOp( _op, x, std::int32_t( y ) );        // y is SmallIntegerOop -> needs no relocation info
        } else {
            arithROOp( _op, x, y );
        }
        if ( result )
            store( x, _dest, temp2, temp3 );
    } else {
        Register x, y;
        bool     result = setupseudoRegisteristers( _dest, arg1, _op, arg2, x, y, temp1, temp2 );
        arithRROp( _op, x, y );
        if ( result ) {
            Register t = ( x == temp1 ) ? temp2 : temp1;
            store( x, _dest, t, temp3 );
        }
    }
}


void ArithRCNode::gen() {
    BasicNode::gen();
    PseudoRegister *arg1  = _src;
    std::int32_t   y      = _operand;
    Register       x;
    bool           result = setupseudoRegisterister( _dest, arg1, _op, x, temp1 );
    arithRCOp( _op, x, y );
    if ( result )
        store( x, _dest, temp2, temp3 );
}


static void floatArithRROp( ArithOpCode op ) {
    switch ( op ) {
        case ArithOpCode::fAddArithOp:
            theMacroAssembler->faddp();
            break;
        case ArithOpCode::fSubArithOp:
            theMacroAssembler->fsubp();
            break;
        case ArithOpCode::fMulArithOp:
            theMacroAssembler->fmulp();
            break;
        case ArithOpCode::fDivArithOp:
            theMacroAssembler->fdivp();
            break;
        case ArithOpCode::fModArithOp:
            theMacroAssembler->fprem();
            break;
        case ArithOpCode::fCmpArithOp:
            theMacroAssembler->fcompp();
            break;
        default         : ShouldNotReachHere();
    }
}


void FloatArithRRNode::gen() {
    BasicNode::gen();
//    bool     noResult = ( _op == ArithOpCode::fCmpArithOp );
    bool     exchange = ( _op == ArithOpCode::fModArithOp or _op == ArithOpCode::fCmpArithOp );
    Register base     = temp3;
    set_floats_base( this, base );
    fload( _src, base, temp1 );
    fload( _oper, base, temp2 );
    if ( exchange )
        theMacroAssembler->fxch();    // is paired with next instruction => no extra cycles
    floatArithRROp( _op );
    if ( _op == ArithOpCode::fCmpArithOp ) {
        // operation set FPU condition codes -> result is FPU status word
        st_assert( not Mapping::isFloatTemporary( _dest->_location ), "ArithOpCode::fCmpArithOp doesn't produce a float" );
        if ( _dest->_location.isRegisterLocation() and _dest->_location.number() == eax.number() ) {
            // store FPU status word in eax
            theMacroAssembler->fwait();
            theMacroAssembler->fnstsw_ax();
        } else {
            // store FPU status word not in eax
            Unimplemented();
        }
    } else {
        // store result (must be a float)
        fstore( _dest, base );
    }
}


static Address doubleKlass_addr() {
    return Address( (std::int32_t) &doubleKlassObject, RelocationInformation::RelocationType::external_word_type );
}


/*
static Oop oopify_float() {
  double x;
  __asm fstp x							// get top of FPU stack
    BlockScavenge bs;						// because all registers are saved on the stack
  return OopFactory::new_double(x);				// box the FloatValue
}*/

static void floatArithROp( ArithOpCode op, Register reg, Register temp ) {
    st_assert( reg not_eq temp, "registers must be different" );
    switch ( op ) {
        case ArithOpCode::fNegArithOp:
            theMacroAssembler->fchs();
            break;
        case ArithOpCode::fAbsArithOp:
            theMacroAssembler->fabs();
            break;
        case ArithOpCode::fSqrArithOp:
            theMacroAssembler->fmul( 0 );
            break;
        case ArithOpCode::f2OopArithOp  : {
            theMacroAssembler->pushl( reg );                // reserve Space for the result
            theMacroAssembler->pushad();                    // make sure no register is destroyed (no scavenge)
//	theMacroAssm->int3();
            theMacroAssembler->call( StubRoutines::oopify_float(), RelocationInformation::RelocationType::runtime_call_type );
            //      theMacroAssm->call((char*)oopify_float, RelocationInformation::RelocationType::runtime_call_type);
            theMacroAssembler->movl( Address( esp, REGISTER_COUNT * OOP_SIZE ), eax );    // store result at reserved stack location
            theMacroAssembler->popad();                    // restore register contents
            theMacroAssembler->popl( reg );                // get result
        }
            break;
        case ArithOpCode::f2FloatArithOp: {
            Label isSmallIntegerOop, is_float, done;
            theMacroAssembler->test( reg, MEMOOP_TAG );            // check if small_int_t
            theMacroAssembler->jcc( Assembler::Condition::zero, isSmallIntegerOop );
            theMacroAssembler->movl( temp, Address( reg, MemOopDescriptor::klass_byte_offset() ) );    // get object klass
            theMacroAssembler->cmpl( temp, doubleKlass_addr() );        // check if floatOop
            theMacroAssembler->jcc( Assembler::Condition::equal, is_float );
            theMacroAssembler->hlt(); // not yet implemented		// cannot be converted

            // convert small_int_t
            theMacroAssembler->bind( isSmallIntegerOop );
            theMacroAssembler->sarl( reg, TAG_SIZE );            // convert small_int_t into std::int32_t
            theMacroAssembler->movl( Address( esp, -OOP_SIZE ), reg );    // store it at end of stack
            theMacroAssembler->fild_s( Address( esp, -OOP_SIZE ) );        // load & convert into FloatValue
            theMacroAssembler->jmp( done );

            // unbox DoubleOop
            theMacroAssembler->bind( is_float );
            theMacroAssembler->fld_d( Address( reg, byteOffset( DoubleOopDescriptor::value_offset() ) ) ); // unbox float

            theMacroAssembler->bind( done );
        }
            break;
        default            : ShouldNotReachHere();
    }
}


void FloatUnaryArithNode::gen() {
    BasicNode::gen();
    Register reg;
    Register base = temp3;
    set_floats_base( this, base );
    if ( Mapping::isFloatTemporary( _src->_location ) or _src->_location == Location::TOP_OF_FLOAT_STACK ) {
        // load argument on FPU stack & setup reg if result is an Oop
        fload( _src, base, temp1 );
        reg = temp1;
    } else {
        // load argument into reg
        reg = movePseudoRegisterToReg( _src, temp1 );
    }
    floatArithROp( _op, reg, temp2 );
    if ( Mapping::isFloatTemporary( _dest->_location ) or _dest->_location == Location::TOP_OF_FLOAT_STACK ) {
        // result is on FPU stack
        fstore( _dest, base );
    } else {
        // result is in reg
        store( reg, _dest, temp2, temp3 );
        set_floats_base( this, base, true );    // store may overwrite base -> make sure it is set again
    }
}


void AssignNode::gen() {
    StoreNode::gen();
    assign( this, _src, _dest, temp1, temp2, temp3 );
}


void BranchNode::gen() {
    BasicNode::gen();
    theMacroAssembler->jcc( mapToCC( _op ), next( 1 )->_label );
}


/*
void BranchNode::gen() {
  BasicNode::gen();
  theMacroAssm->jcc(mapToCC(_op), next(1)->label);
  theMacroAssm->jmp(next()->label);	// this jump will be eliminated since this is the likely successor
  bb_needs_jump = false;		// no jump necessary at end of basic block
}
*/


void ContextCreateNode::gen() {
    BasicNode::gen();

    switch ( _contextSize ) {
        case 0:
            primitiveCall( scope(), Primitives::context_allocate0() );
            break;
        case 1:
            primitiveCall( scope(), Primitives::context_allocate1() );
            break;
        case 2:
            primitiveCall( scope(), Primitives::context_allocate2() );
            break;
        default:
            st_assert( _pdesc == Primitives::context_allocate(), "bad context create prim" );
            theMacroAssembler->pushl( (std::int32_t) smiOopFromValue( _contextSize ) );
            primitiveCall( scope(), _pdesc );
            theMacroAssembler->addl( esp, OOP_SIZE );    // pop argument, this is not a Pascal call - should fix this
    }
    Register context = Mapping::asRegister( resultLoc );
    if ( _src == nullptr ) {
        st_assert( scope()->isMethodScope() or scope()->method()->block_info() == MethodOopDescriptor::expects_nil, "inconsistency" );
        theMacroAssembler->movl( Address( context, ContextOopDescriptor::parent_byte_offset() ), nullptr );
        // nullptr for now; the interpreter uses nil. However, some of the
        // context verification code called from compiled code checks for
        // parents that are either a frame pointer, nullptr or a context.
        // This should be unified at some point. (gri 5/9/96)
    } else {
        // parent is in _src (incoming)
        Register parent = movePseudoRegisterToReg( _src, temp1 );
        theMacroAssembler->movl( Address( context, ContextOopDescriptor::parent_byte_offset() ), parent );
    }
    store( context, _dest, temp2, temp3 );
    theMacroAssembler->store_check( context, temp2 );
}


void ContextInitNode::gen() {
    BasicNode::gen();
    // initialize context fields
    for ( std::size_t i = nofTemps() - 1; i >= 0; i-- ) {
        PseudoRegister *src = _initializers->at( i )->pseudoRegister();
        PseudoRegister *dest;
        if ( src->isBlockPseudoRegister() and wasEliminated() ) {
            // Blocks aren't actually assigned (at the PseudoRegister level) so that the inlining info isn't lost.
            continue;                // there's no assignment (context was eliminated)
        }
        dest = contents()->at( i )->pseudoRegister();
        assign( this, src, dest, temp1, temp2, temp3, false );
    }
    // NB: no store check necessary (done in ContextCreateNode)
    // init node must follow create immediately (since fields are uninitialized)
}


void ContextZapNode::gen() {
    // Only generated for new backend yet
    ShouldNotReachHere();
}


void FixedCodeNode::gen() {
    BasicNode::gen();
    switch ( _kind ) {
        case FixedCodeNode::FixedCodeKind::dead_end:
            theMacroAssembler->hlt();
            break;
        case FixedCodeNode::FixedCodeKind::inc_counter:
            incCounter();
            break;
        default: st_fatal1( "unexpected FixedCodeNode kind %d", _kind );
    }
}


void InlinedReturnNode::gen() {
    BasicNode::gen();
    assign( this, _src, _dest, temp1, temp2, temp3 );
    if ( scope()->needsContextZapping() ) {
        zapContext( scope()->context(), temp1 );
    }
}


void ReturnNode::gen() {
    BasicNode::gen();
    st_assert( _src->_location == resultLoc, "result in wrong location" );
    if ( scope()->needsContextZapping() ) {
        zapContext( scope()->context(), temp1 );
    }
    // remove stack frame
    if ( VerifyCode or GenTraceCalls )
        verifyReturnCode( Mapping::asRegister( resultLoc ) );
    if ( TraceResults )
        call_trace_result( result_reg );
    std::int32_t no_of_args_to_pop = scope()->nofArguments();
    if ( scope()->method()->is_blockMethod() ) {
        // blocks are called via primitiveValue => need to pop first argument
        // of primitiveValue (= block closure) as well since return happens
        // directly (and not through primitiveValue).
        no_of_args_to_pop++;
    }
    theMacroAssembler->leave();
    theMacroAssembler->ret( no_of_args_to_pop * OOP_SIZE );
}


void NonLocalReturnSetupNode::gen() {
    BasicNode::gen();
    st_assert( _src->_location == NonLocalReturnResultLoc, "result in wrong location" );
    // get, test if not zapped & assign home
    Label    NonLocalReturn_error;
    Register home = uplevelBase( scope()->context(), scope()->homeContext() + 1, temp1 );
    theMacroAssembler->testl( home, home );
    theMacroAssembler->jcc( Assembler::Condition::zero, NonLocalReturn_error ); // zero -> home has been zapped
    if ( home not_eq Mapping::asRegister( NonLocalReturnHomeLoc ) ) {
        theMacroAssembler->movl( Mapping::asRegister( NonLocalReturnHomeLoc ), home );
    }
    // assign home id
    theMacroAssembler->movl( NonLocalReturn_homeId_reg, scope()->home()->scopeID() );
    if ( TraceResults )
        call_trace_result( NonLocalReturn_result_reg );
    continueNonLocalReturn( temp1, temp2 );
    // call run-time routine in failure case
    theMacroAssembler->bind( NonLocalReturn_error );
    call_C( (const char *) suspend_on_NonLocalReturn_error, RelocationInformation::RelocationType::runtime_call_type, true );
    theMacroAssembler->hlt();
}


void NonLocalReturnContinuationNode::gen() {
    BasicNode::gen();
    if ( scope()->needsContextZapping() ) {
        zapContext( scope()->context(), temp1 );
    }
    continueNonLocalReturn( temp1, temp2 );
}


void NonLocalReturnTestNode::gen() {
    BasicNode::gen();
    //theMacroAssm->bind(scope()->_nlrTestPointLabel);
    // arrived at the right frame?
    Label L;
    theMacroAssembler->cmpl( Mapping::asRegister( NonLocalReturnHomeLoc ), ebp );
    theMacroAssembler->jcc( Assembler::Condition::notEqual, L );
    // arrived at the right scope within a frame?
    std::int32_t id = scope()->scopeID();
    if ( id == 0 ) {
        // use x86 test to compare with 0 (smaller code than with cmp)
        theMacroAssembler->testl( Mapping::asRegister( NonLocalReturnHomeIdLoc ), Mapping::asRegister( NonLocalReturnHomeIdLoc ) );
    } else {
        theMacroAssembler->cmpl( Mapping::asRegister( NonLocalReturnHomeIdLoc ), id );
    }
    theMacroAssembler->jcc( Assembler::Condition::equal, next1()->_label );
    // continue non-local return
    theMacroAssembler->bind( L );
}


void InterruptCheckNode::gen() {
    BasicNode::gen();
    Unimplemented();
}


static void testForSingleKlass( Register obj, KlassOop klass, Register klassReg, Label &success, Label &failure ) {
    if ( klass == Universe::smiKlassObject() ) {
        // check tag
        theMacroAssembler->test( obj, MEMOOP_TAG );
    } else if ( klass == Universe::trueObject()->klass() ) {
        // only one instance: compare with trueObject
        theMacroAssembler->cmpl( obj, Universe::trueObject() );
    } else if ( klass == Universe::falseObject()->klass() ) {
        // only one instance: compare with falseObject
        theMacroAssembler->cmpl( obj, Universe::falseObject() );
    } else if ( klass == Universe::nilObject()->klass() ) {
        // only one instance: compare with nilObject
        theMacroAssembler->cmpl( obj, Universe::nilObject() );
    } else {
        // compare against obj's klass - must check if small_int_t first
        theMacroAssembler->test( obj, MEMOOP_TAG );
        theMacroAssembler->jcc( Assembler::Condition::zero, failure );
        theMacroAssembler->movl( klassReg, Address( obj, MemOopDescriptor::klass_byte_offset() ) );
        theMacroAssembler->cmpl( klassReg, klass );
    }
    theMacroAssembler->jcc( Assembler::Condition::notEqual, failure );
    theMacroAssembler->jmp( success );    // this jump will be eliminated since this is the likely successor
}


static bool testForBoolKlasses( Register obj, KlassOop klass1, KlassOop klass2, Register klassReg, bool hasUnknown, Label &success1, Label &success2, Label &failure ) {
    st_unused( klassReg ); // unused

    Oop bool1 = Universe::trueObject();
    Oop bool2 = Universe::falseObject();
    if ( klass1 == bool2->klass() and klass2 == bool1->klass() ) {
        Oop t = bool1;
        bool1 = bool2;
        bool2 = t;
    }
    if ( klass1 == bool1->klass() and klass2 == bool2->klass() ) {
        if ( hasUnknown ) {
            theMacroAssembler->cmpl( obj, bool1 );
            theMacroAssembler->jcc( Assembler::Condition::equal, success1 );
            theMacroAssembler->cmpl( obj, bool2 );
            theMacroAssembler->jcc( Assembler::Condition::notEqual, failure );
            theMacroAssembler->jmp( success2 );    // this jump will be eliminated since this is the likely successor
        } else {
            theMacroAssembler->cmpl( obj, bool2 );
            theMacroAssembler->jcc( Assembler::Condition::equal, success2 );
            theMacroAssembler->jmp( success1 );    // this jump will be eliminated since this is the likely successor
        }
        return true;
    }
    return false;
}


static void generalTypeTest( Register obj, Register klassReg, bool hasUnknown, GrowableArray<KlassOop> *classes, GrowableArray<Label *> *next ) {
    // handle general case: N klasses, N+1 labels (first label = unknown case)
    std::int32_t            smi_case = -1;            // index of small_int_t case in next array (if there)
    const std::int32_t      len      = classes->length();
    GrowableArray<KlassOop> klasses( len );    // list of classes excluding small_int_t case
    GrowableArray<Label *>  labels( len );    // list of nodes   excluding small_int_t case

    // compute klasses & nodes list without small_int_t case
    std::int32_t i = 0;
    for ( ; i < len; i++ ) {
        const KlassOop klass = classes->at( i );
        if ( klass == Universe::smiKlassObject() ) {
            smi_case = i + 1;
        } else {
            klasses.append( klass );
            labels.append( next->at( i + 1 ) );
        }
    }

    if ( smi_case == -1 and hasUnknown ) {
        // small_int_t case is also unknown case
        smi_case = 0;
    }

    // generate code
    if ( smi_case >= 0 ) {
        theMacroAssembler->test( obj, MEMOOP_TAG );
        theMacroAssembler->jcc( Assembler::Condition::zero, *next->at( smi_case ) );
    }

    bool               klassHasBeenLoaded = false;
    const std::int32_t nof_cmps           = hasUnknown ? klasses.length() : klasses.length() - 1;
    for ( std::int32_t i                  = 0; i < nof_cmps; i++ ) {
        const KlassOop klass = klasses.at( i );
        if ( klass == Universe::trueObject()->klass() ) {
            // only one instance: compare with trueObject
            theMacroAssembler->cmpl( obj, Universe::trueObject() );
        } else if ( klass == Universe::falseObject()->klass() ) {
            // only one instance: compare with falseObject
            theMacroAssembler->cmpl( obj, Universe::falseObject() );
        } else if ( klass == Universe::nilObject()->klass() ) {
            // only one instance: compare with nilObject
            theMacroAssembler->cmpl( obj, Universe::nilObject() );
        } else {
            // compare with class
            st_assert( klass not_eq Universe::smiKlassObject(), "should have been excluded" );
            if ( not klassHasBeenLoaded ) {
                theMacroAssembler->movl( klassReg, Address( obj, MemOopDescriptor::klass_byte_offset() ) );
                klassHasBeenLoaded = true;
            }
            theMacroAssembler->cmpl( klassReg, klass );
        }
        theMacroAssembler->jcc( Assembler::Condition::equal, *labels.at( i ) );
    }
    if ( hasUnknown ) {
        theMacroAssembler->jmp( *( next->first() ) );
    } else {
        // must be last case, no test required
        theMacroAssembler->jmp( *labels.at( i ) );
    }
}


void TypeTestNode::gen() {
    BasicNode::gen();
    const std::int32_t len      = classes()->length();
    const Register     obj      = movePseudoRegisterToReg( _src, temp1 );
    const Register     klassReg = temp2;
    bb_needs_jump            = false;  // we generate all jumps explicitly

    if ( ReorderBBs ) {
        // generate better code for some of the frequent cases

        if ( len == 1 ) {
            // handle all cases where only one klass is involved
            st_assert( hasUnknown(), "should be eliminated if there's no unknown case" );
            st_assert( likelySuccessor() == next( 1 ), "code pattern is not optimal" );
            KlassOop klass = classes()->at( 0 );
            testForSingleKlass( obj, klass, klassReg, next( 1 )->_label, next()->_label );
            return;
        }

        if ( len == 2 ) {
            // handle pure boolean cases (ifTrue:/ifFalse:)
            KlassOop klass1 = classes()->at( 0 );
            KlassOop klass2 = classes()->at( 1 );
            if ( testForBoolKlasses( obj, klass1, klass2, klassReg, hasUnknown(), next( 1 )->_label, next( 2 )->_label, next()->_label ) ) {
                return;
            }
        }
    }

    // handle general case
    GrowableArray<Label *> labels( len + 1 );
    for ( std::int32_t     i = 0; i <= len; i++ )
        labels.append( &next( i )->_label );
    generalTypeTest( obj, klassReg, hasUnknown(), classes(), &labels );
}


/* old code
void TypeTestNode::gen() {
  BasicNode::gen();
  const std::int32_t len = classes()->length();
  const Register obj = movePseudoRegisterToReg(_src, temp1);
  const Register klassReg = temp2;

  if (ReorderBBs) {
    // generate better code for some of the frequent cases

    if (len == 1) {
      // handle all cases where only one klass is involved
      assert(hasUnknown(), "should be eliminated if there's no unknown case");
      assert(likelySuccessor() == next(1), "code pattern is not optimal");
      klassOop klass = classes()->at(0);
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
        // compare against obj's klass - must check if small_int_t first
	theMacroAssm->test(obj, MEMOOP_TAG);
	theMacroAssm->jcc(Assembler::Condition::zero, next()->label);
        theMacroAssm->movl(klassReg, Address(obj, memOopDescriptor::klass_byte_offset()));
        theMacroAssm->cmpl(klassReg, klass);
      }
      theMacroAssm->jcc(Assembler::Condition::notEqual, next()->label);
      theMacroAssm->jmp(next(1)->label);	// this jump will be eliminated since this is the likely successor
      bb_needs_jump = false;			// no jump necessary at end of basic block
      return;
    }

    if (len == 2) {
      // handle pure boolean cases (ifTrue:/ifFalse:)
      klassOop klass1 = classes()->at(0);
      klassOop klass2 = classes()->at(1);
      Oop      bool1  = Universe::trueObject();
      Oop      bool2  = Universe::falseObject();
      if (klass1 == bool2->klass() and klass2 == bool1->klass()) {
        Oop t = bool1; bool1 = bool2; bool2 = t;
      }
      if (klass1 == bool1->klass() and klass2 == bool2->klass()) {
	if (hasUnknown()) {
	  assert(likelySuccessor() == next(2), "code pattern is not optimal");
	  theMacroAssm->cmpl(obj, bool1);
	  theMacroAssm->jcc(Assembler::Condition::equal, next(1)->label);
          theMacroAssm->cmpl(obj, bool2);
          theMacroAssm->jcc(Assembler::Condition::notEqual, next()->label);
	  theMacroAssm->jmp(next(2)->label);	// this jump will be eliminated since this is the likely successor
	} else {
	  assert(likelySuccessor() == next(1), "code pattern is not optimal");
          theMacroAssm->cmpl(obj, bool2);
	  theMacroAssm->jcc(Assembler::Condition::equal, next(2)->label);
	  theMacroAssm->jmp(next(1)->label);	// this jump will be eliminated since this is the likely successor
	}
	bb_needs_jump = false;			// no jump necessary at end of basic block
        return;
      }
    }

  // handle general case
  Node* smi_case = nullptr;		// small_int_t case if there
  GrowableArray<klassOop> klasses(len);	// list of classes excluding small_int_t case
  GrowableArray<Node*>    nodes(len);	// list of nodes   excluding small_int_t case

  // compute klasses & nodes list without small_int_t case
  for (std::int32_t i = 0; i < len; i++) {
    const klassOop klass = classes()->at(i);
    if (klass == Universe::smiKlassObject()) {
      smi_case = next(i+1);
    } else {
      klasses.append(klass);
      nodes.append(next(i+1));
    }
  }

  if (smi_case == nullptr and hasUnknown()) {
    // small_int_t case is also unknown case
    smi_case = next();
  }

  // generate code
  if (smi_case not_eq nullptr) {
    theMacroAssm->test(obj, MEMOOP_TAG);
    theMacroAssm->jcc(Assembler::Condition::zero, smi_case->label);
  }

  bool klassHasBeenLoaded = false;
  const std::int32_t nof_cmps = hasUnknown() ? klasses.length() : klasses.length() - 1;
  for (std::int32_t i = 0; i < nof_cmps; i++) {
    const klassOop klass = klasses.at(i);
    if (klass == Universe::trueObject()->klass()) {
      // only one instance: compare with trueObject
      theMacroAssm->cmpl(obj, Universe::trueObject());
    } else if (klass == Universe::falseObject()->klass()) {
      // only one instance: compare with falseObject
      theMacroAssm->cmpl(obj, Universe::falseObject());
    } else if (klass == Universe::nilObject()->klass()) {
      // only one instance: compare with nilObject
      theMacroAssm->cmpl(obj, Universe::nilObject());
    } else {
      // compare with class
      assert(klass not_eq Universe::smiKlassObject(), "should have been excluded");
      if (not klassHasBeenLoaded) {
        theMacroAssm->movl(klassReg, Address(obj, memOopDescriptor::klass_byte_offset()));
	klassHasBeenLoaded = true;
      }
      theMacroAssm->cmpl(klassReg, klass);
    }
    theMacroAssm->jcc(Assembler::Condition::equal, nodes.at(i)->label);
  }
  if (hasUnknown()) {
    theMacroAssm->jmp(next()->label);
  } else {
    // must be last case, no test required
    theMacroAssm->jmp(nodes.at(i)->label);
  }
  bb_needs_jump = false;
}
*/


void UncommonNode::gen() {
    BasicNode::gen();
    theMacroAssembler->call( StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type );
}


void BlockCreateNode::copyIntoContexts( Register val, Register t1, Register t2 ) {
    // Copy newly created block (in val) into all contexts that have a copy;
    // registers t1 and t2 can be used as scratch registers.
    // The BlockPseudoRegister has a list of all contexts containing the block.  It should
    // be stored into those that are allocated (weren't eliminated) and are in
    // a sender scope.
    // Why not copy into contexts in a sibling scope?  There are two cases:
    //   (1) The sibling scope never created the block(s) that uplevel-access this
    //       block.  The context location still contains 0 but that doesn't matter
    //       because that context location is now inaccessible.
    //   (2) The sibling scope did create these block(s).  In this case, the receiver
    //       must already exist since it was materialized when the first uplevel-
    //       accessing block was created.
    // Urs 4/96
    BlockPseudoRegister       *blk    = block();
    GrowableArray<Location *> *copies = blk->contextCopies();
    if ( copies == nullptr )
        return;
    for ( std::size_t i = copies->length() - 1; i >= 0; i-- ) {
        Location       *l                = copies->at( i );
        InlinedScope   *scopeWithContext = theCompiler->scopes->at( l->scopeID() );
        PseudoRegister *r                = scopeWithContext->contextTemporaries()->at( l->tempNo() )->pseudoRegister();
        if ( r->_location == Location::UNALLOCATED_LOCATION )
            continue;      // not uplevel-accessed (eliminated)
        if ( r->isBlockPseudoRegister() )
            continue;          // ditto
        if ( not r->_location.isContextLocation() ) st_fatal( "expected context location" );
        if ( scopeWithContext->isSenderOrSame( _scope ) ) {
            store( val, r, t1, t2 );
        }
    }
}


void BlockCreateNode::materialize() {
    CompileTimeClosure *closure = block()->closure();
    if ( closure->context()->_location.isRegisterLocation() ) {
        // should not be allocated to a register, since the register will
        // be destroyed after the call - push it on the stack as temporary
        // fix - take this out after register allocation has been fixed
        theMacroAssembler->pushl( Mapping::asRegister( closure->context()->_location ) );
    }
    std::int32_t nofArgs = closure->nofArgs();
    switch ( nofArgs ) {
        case 0:
            primitiveCall( scope(), Primitives::block_allocate0() );
            break;
        case 1:
            primitiveCall( scope(), Primitives::block_allocate1() );
            break;
        case 2:
            primitiveCall( scope(), Primitives::block_allocate2() );
            break;
        default:
            st_assert( _pdesc == Primitives::block_allocate(), "bad block clone prim" );
            theMacroAssembler->pushl( (std::int32_t) smiOopFromValue( nofArgs ) );
            primitiveCall( scope(), _pdesc );
            theMacroAssembler->addl( esp, OOP_SIZE ); // pop argument, this is not a Pascal call - should fix this
    }
    // assign result
    Register t = moveLocToReg( resultLoc, answerPseudoRegisterReg( _dest, temp1 ) );
    store( t, _dest, temp2, temp3 );
    // copy into all contexts that have a copy
    if ( block()->isMemoized() )
        copyIntoContexts( t, temp2, temp3 );
    // initialize block closure fields
    Register closureReg = Mapping::asRegister( resultLoc ); // fix this: should refer to _dest->loc
    Register contextReg;
    if ( closure->context()->_location.isRegisterLocation() ) {
        // context value is on the stack -- see also comment above
        // take this out after register allocation has been fixed
        contextReg = temp1;
        theMacroAssembler->popl( contextReg );
    } else {
        contextReg = movePseudoRegisterToReg( closure->context(), temp1 );
    }
    if ( VerifyCode ) {
        switch ( closure->method()->block_info() ) {
            case MethodOopDescriptor::expects_nil:
                verifyNilOrContextCode( contextReg );
                break;
            case MethodOopDescriptor::expects_self     :
                [[fallthrough]];
            case MethodOopDescriptor::expects_parameter:
                verifyOopCode( contextReg );
                break;
            case MethodOopDescriptor::expects_context:
                verifyContextCode( contextReg );
                break;
            default: ShouldNotReachHere();
        }
    }
    theMacroAssembler->Store( contextReg, closureReg, BlockClosureOopDescriptor::context_byte_offset() );
    // assert(theCompiler->JumpTableID == closure->parent_id(), "NativeMethod id must be the same");
    // fix this: RELOCATION INFORMATION IS NEEDED WHEN MOVING THE JUMPTABLE (Snapshot reading etc.)
    theMacroAssembler->movl( Address( closureReg, BlockClosureOopDescriptor::method_or_entry_byte_offset() ), std::int32_t( closure->jump_table_entry() ) );
    if ( VerifyCode )
        verifyBlockCode( closureReg );
    theMacroAssembler->store_check( closureReg, temp1 );
}


void BlockCreateNode::gen() {
    BasicNode::gen();
    if ( block()->closure()->method()->is_clean_block() ) {
        // create the block now (doesn't need to be copied at run-time)
        CompileTimeClosure *closure = block()->closure();
        BlockClosureOop    blk      = BlockClosureOopDescriptor::create_clean_block( closure->nofArgs(), closure->jump_table_entry() );
        Mapping::storeO( blk, _dest->_location, temp1, temp3, true );
    } else if ( block()->isMemoized() ) {
        // initialize block variable
        Mapping::storeO( MemoizedBlockNameDescriptor::uncreatedBlockValue(), _dest->_location, temp1, temp3, true );
    } else {
        // actually create block
        materialize();
    }
}


void BlockMaterializeNode::gen() {
    BasicNode::gen();
    if ( block()->isMemoized() and not block()->closure()->method()->is_clean_block() ) {
        // materialize block if it is not already materialized
        // (nothing to do in case of non-memoized or clean blocks)
        Label    L;
        Register t = movePseudoRegisterToReg( block(), temp1 );
        st_assert( MemoizedBlockNameDescriptor::uncreatedBlockValue() == Oop( 0 ), "change the code generation here" );
        theMacroAssembler->testl( t, t );
        theMacroAssembler->jcc( Assembler::Condition::notZero, L );
        materialize();
        theMacroAssembler->bind( L );
    }
}


void LoopHeaderNode::gen() {
    if ( not _activated )
        return;    // loop wasn't optimized
    // the loop header node performs all checks hoisted out of the loop:
    // for general loops:
    //   - do all type tests in the list, uncommon branch if they fail
    //     (common case: true/false tests, single-klass tests)
    // additionally for integer loops:
    //   - test lowerBound (may be nullptr), upperBound, loopVar for small_int_t-ness (the first two may be ConstPseudoRegisters)
    //   - if upperBound is nullptr, upperLoad is load of the array size
    //   - if loopArray is non-nullptr, check lowerBound (if non-nullptr) or initial value of loopVar against 1
    TrivialNode::gen();
    Label ok;
    Label failure;
    generateTypeTests( ok, failure );
    generateIntegerLoopTests( ok, failure );
    generateArrayLoopTests( ok, failure );
    if ( ok.is_unbound() )
        theMacroAssembler->bind( ok );
    theMacroAssembler->jmp( next()->_label );
    // above 2 lines could be eliminated with if (ok.is_unbound()) ok.redirectTo(next()->label)
    bb_needs_jump = false;  // we generate all jumps explicitly
    theMacroAssembler->bind( failure );
    theMacroAssembler->call( StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type );
}


void LoopHeaderNode::generateTypeTests( Label &cont, Label &failure ) {
    // test all values against expected classes
    Label              *ok;
    const Register     klassReg = temp2;
    const std::int32_t len      = _tests->length() - 1;
    std::int32_t       last;                        // last case that generates a test
    for ( last = len; last >= 0 and _tests->at( last )->_testedPR->_location == Location::UNALLOCATED_LOCATION; last-- );
    if ( last < 0 )
        return;                    // no tests at all
    for ( std::size_t i = 0; i <= last; i++ ) {
        HoistedTypeTest *t = _tests->at( i );
        if ( t->_testedPR->_location == Location::UNALLOCATED_LOCATION )
            continue;    // optimized away, or ConstPseudoRegister
        if ( t->_testedPR->isConstPseudoRegister() ) {
            guarantee( t->_testedPR->_location == Location::UNALLOCATED_LOCATION, "code assumes ConstPseudoRegisters are unallocated" );
            handleConstantTypeTest( (ConstPseudoRegister *) t->_testedPR, t->_klasses );
        } else {
            const Register obj = movePseudoRegisterToReg( t->_testedPR, temp1 );
            Label          okLabel;
            ok = ( i == last ) ? &cont : &okLabel;
            if ( t->_klasses->length() == 1 ) {
                testForSingleKlass( obj, t->_klasses->at( 0 ), klassReg, *ok, failure );
            } else if ( t->_klasses->length() == 2 and testForBoolKlasses( obj, t->_klasses->at( 0 ), t->_klasses->at( 1 ), klassReg, true, *ok, *ok, failure ) ) {
                // ok, was a bool test
            } else {
                const std::int32_t     len = t->_klasses->length();
                GrowableArray<Label *> labels( len + 1 );
                labels.append( &failure );
                for ( std::size_t i = 0; i < len; i++ )
                    labels.append( ok );
                generalTypeTest( obj, klassReg, true, t->_klasses, &labels );
            }
            if ( i not_eq last )
                theMacroAssembler->bind( *ok );
        }
    }
}


void LoopHeaderNode::generateIntegerLoopTest( PseudoRegister *p, Label &prev, Label &failure ) {
    const Register klassReg = temp2;
    if ( p not_eq nullptr ) {
        if ( p->isConstPseudoRegister() ) {
            // no run-time test necessary
            handleConstantTypeTest( (ConstPseudoRegister *) p, nullptr );
        } else if ( p->_location == Location::UNALLOCATED_LOCATION ) {
            // p is never used in loop, so no test needed
            guarantee( p->cpseudoRegister() == p, "should use cpseudoRegister()" );
        } else {
            // generate run-time test
            if ( prev.is_unbound() )
                theMacroAssembler->bind( prev );
            Label          ok;
            const Register obj = movePseudoRegisterToReg( p, temp1 );
            testForSingleKlass( obj, Universe::smiKlassObject(), klassReg, ok, failure );
            theMacroAssembler->bind( ok );
        }
    }
}


void LoopHeaderNode::generateIntegerLoopTests( Label &prev, Label &failure ) {
    if ( not _integerLoop )
        return;
    generateIntegerLoopTest( _lowerBound, prev, failure );
    generateIntegerLoopTest( _upperBound, prev, failure );
    generateIntegerLoopTest( _loopVar, prev, failure );
}


void LoopHeaderNode::handleConstantTypeTest( ConstPseudoRegister *r, GrowableArray<KlassOop> *klasses ) {
    // constant r is tested against klasses (efficiency hack: klasses == nullptr means {small_int_t})
    if ( ( klasses == nullptr and r->constant->isSmallIntegerOop() ) or ( klasses and klasses->contains( r->constant->klass() ) ) ) {
        // always ok, no need to test
    } else {
        compiler_warning( "loop header type test will always fail" );
        // don't jump to failure because that would make subsequent LoopHeader code unreachable (--> breaks back end)
        theMacroAssembler->call( StubRoutines::unused_uncommon_trap_entry(), RelocationInformation::RelocationType::uncommon_type );
    }
}


void LoopHeaderNode::generateArrayLoopTests( Label &prev, Label &failure ) {
    if ( not _integerLoop )
        return;
    Register       boundReg          = temp1;
    const Register tempseudoRegister = temp2;
    if ( _upperLoad not_eq nullptr ) {
        // The loop variable iterates from lowerBound...array size; if any of the array accesses use the loop variable
        // without an index range check, we need to check it here.
        PseudoRegister      *loopArray = _upperLoad->src();
        AbstractArrayAtNode *atNode;
        std::int32_t        i          = _arrayAccesses->length() - 1;
        for ( ; i >= 0; i-- ) {
            atNode = _arrayAccesses->at( i );
            if ( atNode->src() == loopArray and not atNode->needsBoundsCheck() )
                break;
        }
        if ( i >= 0 ) {
            // loopVar is used to index into array; make sure lower & upper bound is within array range
            if ( _lowerBound not_eq nullptr and _lowerBound->isConstPseudoRegister() and ( (ConstPseudoRegister *) _lowerBound )->constant->isSmallIntegerOop() and ( (ConstPseudoRegister *) _lowerBound )->constant >= smiOopFromValue( 1 ) ) {
                // loopVar iterates from smi_const to array size, so no test necessary
            } else {
                // test lower bound
                if ( prev.is_unbound() )
                    theMacroAssembler->bind( prev );
                if ( _lowerBound->_location == Location::UNALLOCATED_LOCATION ) {
                    guarantee( _lowerBound->cpseudoRegister() == _lowerBound, "should use cpseudoRegister()" );
                } else {
                    const Register t = movePseudoRegisterToReg( _lowerBound ? _lowerBound : _loopVar, tempseudoRegister );
                    st_unused(  t  ); // unused
                    theMacroAssembler->cmpl( boundReg, smiOopFromValue( 1 ) );
                    theMacroAssembler->jcc( Assembler::Condition::less, failure );
                }
            }

            // test upper bound
            boundReg = movePseudoRegisterToReg( _upperBound, boundReg );
            const Register t = movePseudoRegisterToReg( atNode->src(), tempseudoRegister );
            theMacroAssembler->movl( t, Address( t, byteOffset( atNode->sizeOffset() ) ) );
            theMacroAssembler->cmpl( boundReg, t );
            theMacroAssembler->jcc( Assembler::Condition::above, failure );
        }
    }
}


static void jcc_error( Node *node, Assembler::Condition cond, Label &label ) {
// Used in code pattern generators that also generate code to setup error messages.
// If an uncommon trap is issued in the error situation anyway, the error message
// setup code is not needed and we can jump to the uncommon node directly => saves
// code & a jump in the commom case.
    Node *failure_start = node->next( 1 );
    if ( failure_start->isUncommonNode() ) {
        // error handling causes uncommon trap anyway, jump to uncommon node directly
        theMacroAssembler->jcc( cond, failure_start->_label );
    } else {
        // failure case is not uncommon, jump to label
        theMacroAssembler->jcc( cond, label );
    }
}


void ArrayAtNode::gen() {
    BasicNode::gen();
    // load registers in an order that reduces load delays
    Register size  = temp3;
    Register index = temp2;
    load( _arg, index );    // index is modified -> load always into register
    Register array = movePseudoRegisterToReg( _src, temp1 );    // array is read_only
    // first element is at index 1 => subtract small_int_t(1) (doesn't change small_int_t/Oop property)
    theMacroAssembler->subl( index, std::int32_t( smiOop_one ) );
    // preload size for bounds check if necessary
    if ( _needBoundsCheck ) {
        theMacroAssembler->movl( size, Address( array, byteOffset( _sizeOffset ) ) );
    }
    // do index small_int_t check if necessary (still possible, even after subtracting small_int_t(1))
    Label indexNotSmi;
    if ( not _intArg ) {
        theMacroAssembler->test( index, MEMOOP_TAG );
        jcc_error( this, Assembler::Condition::notZero, indexNotSmi );
    }
    // do bounds check if necessary
    Label indexOutOfBounds;
    if ( _needBoundsCheck ) {
        theMacroAssembler->cmpl( index, size );
        jcc_error( this, Assembler::Condition::aboveEqual, indexOutOfBounds );
    }
    // load element
    Register t = answerPseudoRegisterReg( _dest, temp3 );
    st_assert( TAG_SIZE == 2, "check this code" );
    switch ( _access_type ) {
        case byte_at:
            theMacroAssembler->sarl( index, TAG_SIZE );    // adjust index
            theMacroAssembler->xorl( t, t );            // clear destination register
            theMacroAssembler->movb( t, Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ) );
            theMacroAssembler->shll( t, TAG_SIZE );        // make result a small_int_t
            break;
        case double_byte_at:
            theMacroAssembler->sarl( index, TAG_SIZE - 1 );    // adjust index
            theMacroAssembler->movl( t, Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ) );
            theMacroAssembler->andl( t, 0x0000FFFF );    // clear upper 2 bytes
            theMacroAssembler->shll( t, TAG_SIZE );        // make result a small_int_t
            break;
        case character_at: {
            theMacroAssembler->sarl( index, TAG_SIZE - 1 );// adjust index
            theMacroAssembler->movl( t, Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ) );
            theMacroAssembler->andl( t, 0x0000FFFF );    // clear upper 2 bytes
            // use t as index into asciiCharacters()
            // check index first, must be 0 <= t < asciiCharacters()->length()
            ObjectArrayOop chars = Universe::asciiCharacters();
            theMacroAssembler->cmpl( t, chars->length() );
            jcc_error( this, Assembler::Condition::aboveEqual, indexOutOfBounds );
            // get character out of chars array
            theMacroAssembler->movl( temp1, chars );
            theMacroAssembler->movl( t, Address( temp1, t, Address::ScaleFactor::times_4, byteOffset( chars->klass()->klass_part()->non_indexable_size() + 1 ) ) );
        }
            break;
        case object_at:
            // small_int_t index is already shifted the right way => no index adjustment necessary
            theMacroAssembler->movl( t, Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ) );
            break;
        default: ShouldNotReachHere();
            break;
    }
    st_assert( t not_eq temp1 and t not_eq temp2, "just checking" );
    store( t, _dest, temp1, temp2 );
    // handle error cases if not uncommon
    if ( canFail() and not next( 1 )->isUncommonNode() ) {
        Label exit;
        theMacroAssembler->jmp( exit );
        // error messages
        if ( not _intArg ) {
            theMacroAssembler->bind( indexNotSmi );
            theMacroAssembler->movl( temp1, vmSymbols::first_argument_has_wrong_type() );
            store( temp1, _error, temp2, temp3 );
            theMacroAssembler->jmp( next( 1 )->_label );
        }
        if ( _needBoundsCheck ) {
            theMacroAssembler->bind( indexOutOfBounds );
            theMacroAssembler->movl( temp1, vmSymbols::out_of_bounds() );
            store( temp1, _error, temp2, temp3 );
            theMacroAssembler->jmp( next( 1 )->_label );
        }
        theMacroAssembler->bind( exit );
    }
}


void ArrayAtPutNode::gen() {
    BasicNode::gen();
    // load registers in an order that reduces load delays
    Register size  = temp3;
    Register index = temp2;
    load( _arg, index );    // index is modified -> load always into register
    Register array   = temp1;
    load( _src, array );    // array may be modified -> load always into register
    // first element is at index 1 => subtract small_int_t(1) (doesn't change small_int_t/Oop property)
    theMacroAssembler->subl( index, std::int32_t( smiOop_one ) );
    // preload size for bounds check if necessary
    if ( _needBoundsCheck ) {
        theMacroAssembler->movl( size, Address( array, byteOffset( _sizeOffset ) ) );
    }
    // do index small_int_t check if necessary (still possible, even after subtracting small_int_t(1))
    Label indexNotSmi;
    if ( not _intArg ) {
        theMacroAssembler->test( index, MEMOOP_TAG );
        jcc_error( this, Assembler::Condition::notZero, indexNotSmi );
    }
    // do bounds check if necessary
    Label indexOutOfBounds;
    if ( _needBoundsCheck ) {
        theMacroAssembler->cmpl( index, size );
        jcc_error( this, Assembler::Condition::aboveEqual, indexOutOfBounds );
    }
    // store element
    Label    elementNotSmi, elementOutOfRange;
    Register element = temp3;
    load( elem, element );// element may be modified -> load always into register
    st_assert( TAG_SIZE == 2, "check this code" );
    switch ( _access_type ) {
        case byte_at_put:
            if ( not _smi_element ) {
                theMacroAssembler->test( element, MEMOOP_TAG );
                jcc_error( this, Assembler::Condition::notZero, elementNotSmi );
            }
            theMacroAssembler->sarl( element, TAG_SIZE );    // convert element into (std::int32_t) byte
            if ( _needs_element_range_check ) {
                theMacroAssembler->cmpl( element, 0x100 );
                jcc_error( this, Assembler::Condition::aboveEqual, elementOutOfRange );
            }
            theMacroAssembler->sarl( index, TAG_SIZE );    // adjust index
            theMacroAssembler->movb( Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ), element );
            st_assert( not _needs_store_check, "just checking" );
            break;
        case double_byte_at_put:
            if ( not _smi_element ) {
                theMacroAssembler->test( element, MEMOOP_TAG );
                jcc_error( this, Assembler::Condition::notZero, elementNotSmi );
            }
            theMacroAssembler->sarl( element, TAG_SIZE );    // convert element into (std::int32_t) double byte
            if ( _needs_element_range_check ) {
                theMacroAssembler->cmpl( element, 0x10000 );
                jcc_error( this, Assembler::Condition::aboveEqual, elementOutOfRange );
            }
            theMacroAssembler->sarl( index, TAG_SIZE - 1 );    // adjust index
            theMacroAssembler->leal( array, Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ) );
            st_assert( temp2 not_eq array and temp2 not_eq element, "check this code" );
            theMacroAssembler->movl( temp2, element );    // copy element (since element might be used afterwards)
            theMacroAssembler->shrl( temp2, 8 );        // shift 2nd byte into low-byte position
            theMacroAssembler->movb( Address( array, 0 ), element );
            theMacroAssembler->movb( Address( array, 1 ), temp2 );
            st_assert( not _needs_store_check, "just checking" );
            // Note: could use a better code sequence without introducing the extra movl & shrl
            //       instruction here - however, currently the assembler doesn't support addressing
            //       of the the 2nd byte in a register (otherwise two movb instructions would do).
            break;
        case object_at_put:
            // small_int_t index is already shifted the right way => no index adjustment necessary
            if ( _needs_store_check ) {
                theMacroAssembler->leal( array, Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ) );
                theMacroAssembler->movl( Address( array ), element );
                theMacroAssembler->store_check( array, temp3 );
            } else {
                theMacroAssembler->movl( Address( array, index, Address::ScaleFactor::times_1, byteOffset( _dataOffset ) ), element );
            }
            break;
        default: ShouldNotReachHere();
            break;
    }
    // handle error cases if not uncommon
    if ( canFail() and not next( 1 )->isUncommonNode() ) {
        Label exit;
        theMacroAssembler->jmp( exit );
        // error messages
        if ( not _intArg ) {
            theMacroAssembler->bind( indexNotSmi );
            theMacroAssembler->movl( temp1, vmSymbols::first_argument_has_wrong_type() );
            store( temp1, _error, temp2, temp3 );
            theMacroAssembler->jmp( next( 1 )->_label );
        }
        if ( _needBoundsCheck ) {
            theMacroAssembler->bind( indexOutOfBounds );
            theMacroAssembler->movl( temp1, vmSymbols::out_of_bounds() );
            store( temp1, _error, temp2, temp3 );
            theMacroAssembler->jmp( next( 1 )->_label );
        }
        if ( not _smi_element ) {
            theMacroAssembler->bind( elementNotSmi );
            theMacroAssembler->movl( temp1, vmSymbols::second_argument_has_wrong_type() );
            store( temp1, _error, temp2, temp3 );
            theMacroAssembler->jmp( next( 1 )->_label );
        }
        if ( _needs_element_range_check ) {
            theMacroAssembler->bind( elementOutOfRange );
            theMacroAssembler->movl( temp1, vmSymbols::value_out_of_range() );
            store( temp1, _error, temp2, temp3 );
            theMacroAssembler->jmp( next( 1 )->_label );
        }
        theMacroAssembler->bind( exit );
    }
}


void InlinedPrimitiveNode::gen() {
    BasicNode::gen();
    switch ( _operation ) {
        case InlinedPrimitiveNode::Operation::OBJ_KLASS: {
            Register obj   = movePseudoRegisterToReg( _src, temp1 );            // obj is read_only
            Register klass = temp2;
            Label    isSmallIntegerOop;
            theMacroAssembler->movl( klass, Universe::smiKlassObject() );
            theMacroAssembler->test( obj, MEMOOP_TAG );
            theMacroAssembler->jcc( Assembler::Condition::zero, isSmallIntegerOop );
            theMacroAssembler->movl( klass, Address( obj, MemOopDescriptor::klass_byte_offset() ) );
            theMacroAssembler->bind( isSmallIntegerOop );
            store( klass, _dest, temp1, temp3 );
        }
            break;
        case InlinedPrimitiveNode::Operation::OBJ_HASH: {
            Unimplemented();
            // Implemented for the small_int_t klass only by now - can be resolved in
            // the PrimitiveInliner for that case without using an InlinedPrimitiveNode.
        };
            break;
        case InlinedPrimitiveNode::Operation::PROXY_BYTE_AT: {
            Register proxy = temp1;
            load( _src, proxy );            // proxy is modified
            Register index = temp2;
            load( _arg1, index );            // index is modified
            Register result = answerPseudoRegisterReg( _dest, temp3 );
            Label    indexNotSmi;
            // do index small_int_t check if necessary
            if ( not _arg1_is_SmallInteger ) {
                theMacroAssembler->test( index, MEMOOP_TAG );
                jcc_error( this, Assembler::Condition::notZero, indexNotSmi );
            }
            // load element
            theMacroAssembler->movl( proxy, Address( proxy, pointer_offset ) );    // unbox proxy
            theMacroAssembler->sarl( index, TAG_SIZE );                // adjust index
            theMacroAssembler->xorl( result, result );                // clear destination register
            theMacroAssembler->movb( result, Address( proxy, index, Address::ScaleFactor::times_1, 0 ) );
            theMacroAssembler->shll( result, TAG_SIZE );                // make result a small_int_t
            // continue
            st_assert( result not_eq temp1 and result not_eq temp2, "just checking" );
            store( result, _dest, temp1, temp2 );
            // handle error cases if not uncommon
            if ( canFail() and not next( 1 )->isUncommonNode() ) {
                Label exit;
                theMacroAssembler->jmp( exit );
                // error messages
                if ( not _arg1_is_SmallInteger ) {
                    theMacroAssembler->bind( indexNotSmi );
                    theMacroAssembler->movl( temp1, vmSymbols::first_argument_has_wrong_type() );
                    store( temp1, _error, temp2, temp3 );
                    theMacroAssembler->jmp( next( 1 )->_label );
                }
                theMacroAssembler->bind( exit );
            }
        }
            break;
        case InlinedPrimitiveNode::Operation::PROXY_BYTE_AT_PUT: {
            bool     const_val = _arg2->isConstPseudoRegister();
            Register proxy     = temp1;
            load( _src, proxy );            // proxy is modified
            Register index = temp2;
            load( _arg1, index );            // index is modified
            Register value;
            if ( const_val ) {
                // value doesn't have to be loaded -> do nothing here
                if ( not _arg2_is_SmallInteger ) st_fatal( "proxy_byte_at_put: should not happen - internal error" );
                //if (not _arg2_is_SmallInteger) fatal("proxy_byte_at_put: should not happen - tell Robert");
            } else {
                value = temp3;
                load( _arg2, value );                // value is modified
            }
            Label indexNotSmi, valueNotSmi;
            // do index small_int_t check if necessary
            if ( not _arg1_is_SmallInteger ) {
                theMacroAssembler->test( index, MEMOOP_TAG );
                jcc_error( this, Assembler::Condition::notZero, indexNotSmi );
            }
            // do value small_int_t check if necessary
            if ( not _arg2_is_SmallInteger ) {
                st_assert( not const_val, "constant shouldn't need a small_int_t check" );
                theMacroAssembler->test( value, MEMOOP_TAG );
                jcc_error( this, Assembler::Condition::notZero, valueNotSmi );
            }
            // store element
            theMacroAssembler->movl( proxy, Address( proxy, pointer_offset ) );    // unbox proxy
            theMacroAssembler->sarl( index, TAG_SIZE );                // adjust index
            if ( const_val ) {
                SmallIntegerOop constant = SmallIntegerOop( ( (ConstPseudoRegister *) _arg2 )->constant );
                st_assert( constant->isSmallIntegerOop(), "should be a small_int_t" );
                theMacroAssembler->movb( Address( proxy, index, Address::ScaleFactor::times_1, 0 ), constant->value() & 0xFF );
            } else {
                theMacroAssembler->sarl( value, TAG_SIZE );                // adjust value
                theMacroAssembler->movb( Address( proxy, index, Address::ScaleFactor::times_1, 0 ), value );
            }
            // handle error cases if not uncommon
            if ( canFail() and not next( 1 )->isUncommonNode() ) {
                Label exit;
                theMacroAssembler->jmp( exit );
                // error messages
                if ( not _arg1_is_SmallInteger ) {
                    theMacroAssembler->bind( indexNotSmi );
                    theMacroAssembler->movl( temp1, vmSymbols::first_argument_has_wrong_type() );
                    store( temp1, _error, temp2, temp3 );
                    theMacroAssembler->jmp( next( 1 )->_label );
                }
                if ( not _arg2_is_SmallInteger ) {
                    theMacroAssembler->bind( valueNotSmi );
                    theMacroAssembler->movl( temp1, vmSymbols::second_argument_has_wrong_type() );
                    store( temp1, _error, temp2, temp3 );
                    theMacroAssembler->jmp( next( 1 )->_label );
                }
                theMacroAssembler->bind( exit );
            }
        };
            break;
        default: ShouldNotReachHere();
    }
}
