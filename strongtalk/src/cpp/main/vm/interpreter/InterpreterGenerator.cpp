
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/InterpreterGenerator.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/primitives/PrimitivesGenerator.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/system/dll.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/system/sizes.hpp"


// Computes the byte offset from the beginning of an Oop
static inline int byteOffset( int offset ) {
    st_assert( offset >= 0, "bad offset" );
    return offset * sizeof( Oop ) - MEMOOP_TAG;
}


extern "C" void trace_send( Oop receiver, MethodOop method ) {

    if ( not TraceMessageSend )
        return;

    ResourceMark resourceMark;
    _console->print( "Trace " );
    receiver->print_value();
    method->selector()->print_value();
    _console->cr();

}



// -----------------------------------------------------------------------------

void InterpreterGenerator::check_ebx() {
    if ( not _debug )
        return;
    // check if ebx is 000000xx
    _macroAssembler->testl( ebx, 0xFFFFFF00 );
    _macroAssembler->jcc( Assembler::Condition::notZero, _ebx_wrong );
}


void InterpreterGenerator::check_oop( Register reg ) {
    if ( not _debug )
        return;
    // check if reg contains an Oop
    _macroAssembler->testb( reg, MARK_TAG_BIT );
    _macroAssembler->jcc( Assembler::Condition::notZero, _obj_wrong );
}


// -----------------------------------------------------------------------------

//
// Stack checker
//
// The stack checker pushes a magic number on the stack and checks for it when it is popped.
// The stack checker is useful for detecting inconsistent numbers of pushes and pops within a
// structured construct (e.g., such as loops).
// Check code is only generated if stack_checks are enabled.

static constexpr int STACK_CHECKER_MAGIC_VALUE = 0x0FCFCFCFC; // must be a smi_t

void InterpreterGenerator::stack_check_push() {
    if ( not _stack_check )
        return;
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( eax, STACK_CHECKER_MAGIC_VALUE );
}


void InterpreterGenerator::stack_check_pop() {
    if ( not _stack_check )
        return;
    Label L;
    // ;_print "pop:  esp = 0x%x", esp, 0
    // ;_print "      tos = 0x%x", eax, 0
    _macroAssembler->cmpl( eax, STACK_CHECKER_MAGIC_VALUE );
    _macroAssembler->jcc( Assembler::Condition::notEqual, _stack_missaligned );
    _macroAssembler->bind( L );
    _macroAssembler->popl( eax );
}


void InterpreterGenerator::should_not_reach_here() {
    // make sure the Interpreter traps if this point is ever reached
    _macroAssembler->hlt();
}


// Arguments, temporaries & instance variables

Address InterpreterGenerator::arg_addr( int i ) {
    st_assert( 1 <= i, "argument number must be positive" );
    return Address( ebp, arg_n_offset + i * oopSize );
}


Address InterpreterGenerator::arg_addr( Register arg_no ) {
    return Address( ebp, arg_no, Address::ScaleFactor::times_4, arg_n_offset + 1 * oopSize, RelocationInformation::RelocationType::none );
}


Address InterpreterGenerator::temp_addr( int i ) {
    st_assert( 0 <= i, "temporary number must be positive" );
    return Address( ebp, temp_0_offset - i * oopSize );
}


Address InterpreterGenerator::temp_addr( Register temp_no ) {
    return Address( ebp, temp_no, Address::ScaleFactor::times_4, temp_0_offset - ( max_nof_temps - 1 ) * oopSize, RelocationInformation::RelocationType::none );
}


Address InterpreterGenerator::float_addr( Register float_no ) {
    return Address( ebp, float_no, Address::ScaleFactor::times_8, float_0_offset - ( max_nof_floats - 1 ) * SIZEOF_FLOAT, RelocationInformation::RelocationType::none );
}


Address InterpreterGenerator::field_addr( Register obj, int i ) {
    st_assert( 2 <= i, "illegal field offset" );
    return Address( obj, byteOffset( i ) );
}


Address InterpreterGenerator::field_addr( Register obj, Register smi_offset ) {
    return Address( obj, smi_offset, Address::ScaleFactor::times_1, -MEMOOP_TAG, RelocationInformation::RelocationType::none );
}


// Instruction sequencing

void InterpreterGenerator::skip_words( int n ) {
    _macroAssembler->addl( esi, ( n + 1 ) * oopSize ); // advance
    _macroAssembler->andl( esi, -oopSize ); // align
}


void InterpreterGenerator::advance_aligned( int n ) {
    _macroAssembler->addl( esi, n + oopSize - 1 ); // advance
    _macroAssembler->andl( esi, -oopSize ); // align
}


void InterpreterGenerator::load_ebx() {
    check_ebx();
    _macroAssembler->movb( ebx, Address( esi ) );
}


void InterpreterGenerator::next_ebx() {
    check_ebx();
    _macroAssembler->movb( ebx, Address( esi, 1 ) );
    _macroAssembler->incl( esi );
}


void InterpreterGenerator::generateStopInterpreterAt() {
    if ( StopInterpreterAt <= 0 )
        return;
    Label cont;
    _macroAssembler->pushl( edx );
    _macroAssembler->movl( edx, Address( int( &StopInterpreterAt ), RelocationInformation::RelocationType::external_word_type ) );
    _macroAssembler->cmpl( edx, Address( int( &NumberOfBytecodesExecuted ), RelocationInformation::RelocationType::external_word_type ) );
    _macroAssembler->popl( edx );
    _macroAssembler->jcc( Assembler::Condition::above, cont );
    _macroAssembler->int3();
    _macroAssembler->bind( cont );

    if ( StopInterpreterAt > 0 ) {
        Label cont2;
        _macroAssembler->cmpl( Address( int( &NumberOfBytecodesExecuted ), RelocationInformation::RelocationType::external_word_type ), StopInterpreterAt );
        _macroAssembler->jcc( Assembler::Condition::less, cont2 );
        _macroAssembler->int3();
        _macroAssembler->bind( cont2 );
    }
}


void InterpreterGenerator::jump_ebx() {

    if ( TraceBytecodes or CountBytecodes or StopInterpreterAt > 0 ) {
        _macroAssembler->incl( Address( int( &NumberOfBytecodesExecuted ), RelocationInformation::RelocationType::external_word_type ) );
        generateStopInterpreterAt();
    }

    if ( TraceBytecodes ) {
        _macroAssembler->pushl( eax );    // save tos
        call_C( ( const char * ) Interpreter::trace_bytecode );
        _macroAssembler->popl( eax );    // restore tos
        load_ebx();
    }
    check_oop( eax );
    _macroAssembler->jmp( Address( noreg, ebx, Address::ScaleFactor::times_4, ( int ) DispatchTable::table() ) );
}


void InterpreterGenerator::load_edi() {
    _macroAssembler->movl( edi, Address( noreg, ebx, Address::ScaleFactor::times_4, ( int ) DispatchTable::table() ) );
}


void InterpreterGenerator::jump_edi() {

    if ( TraceBytecodes or CountBytecodes or StopInterpreterAt > 0 ) {
        _macroAssembler->incl( Address( int( &NumberOfBytecodesExecuted ), RelocationInformation::RelocationType::external_word_type ) );
        generateStopInterpreterAt();
    }

    if ( TraceBytecodes ) {
        _macroAssembler->pushl( eax );    // save tos
        call_C( ( const char * ) Interpreter::trace_bytecode );
        _macroAssembler->popl( eax );    // restore tos
        load_ebx();
    }
    check_oop( eax );
    _macroAssembler->jmp( edi );
}


const char * InterpreterGenerator::entry_point() {
    const char * ep = _macroAssembler->pc();
    if ( _debug ) {
        check_ebx();
        check_oop( eax );
    }
    return ep;
}


// C calls
//
// Always use an ic info so that NonLocalReturns can be handled since
// an NonLocalReturn may be issued via an abort to terminate a thread.

void InterpreterGenerator::call_C( Label & L ) {
    save_esi();
    _macroAssembler->call_C( L, _C_nlr_testpoint );
    restore_esi();
    restore_ebx();
}


void InterpreterGenerator::call_C( const char * entry ) {
    save_esi();
    _macroAssembler->call_C( entry, RelocationInformation::RelocationType::runtime_call_type, _C_nlr_testpoint );
    restore_esi();
    restore_ebx();
}


void InterpreterGenerator::call_C( Register entry ) {
    save_esi();
    _macroAssembler->call_C( entry, _C_nlr_testpoint );
    restore_esi();
    restore_ebx();
}


// Calling conventions for sends
//
// For general sends the receiver and the arguments are pushed on the stack in the order
// in which they appear in the source code (i.e., receiver first, first argument, etc.).
// For self and super sends, the receiver is *not* pushed but taken directly from the
// callers stack frame, i.e. in these cases only the arguments are on the stack.
//
// The callee is responsible for removing the arguments from the stack, i.e., its return
// instructions have to know how many arguments there are.
//
// load_recv is loading the receiver into eax and makes sure that the receiver
// (except for self and super sends) as well as the arguments are on the stack.


void InterpreterGenerator::load_recv( ByteCodes::ArgumentSpec arg_spec ) {
    _macroAssembler->pushl( eax ); // make sure receiver & all arguments are on the stack
    switch ( arg_spec ) {
        case ByteCodes::ArgumentSpec::recv_0_args:
            break; // recv already in eax
        case ByteCodes::ArgumentSpec::recv_1_args:
            _macroAssembler->movl( eax, Address( esp, 1 * oopSize ) );
            break;
        case ByteCodes::ArgumentSpec::recv_2_args:
            _macroAssembler->movl( eax, Address( esp, 2 * oopSize ) );
            break;
        case ByteCodes::ArgumentSpec::recv_n_args:
            // byte after send byte code specifies the number of arguments (0..255)
            _macroAssembler->movb( ebx, Address( esi, 1 ) );
            _macroAssembler->movl( eax, Address( esp, ebx, Address::ScaleFactor::times_4 ) );
            break;
        case ByteCodes::ArgumentSpec::args_only:
            _macroAssembler->movl( eax, self_addr() );
            break;
        default: ShouldNotReachHere();
    }
}


//-----------------------------------------------------------------------------------------
// Instructions

const char * InterpreterGenerator::push_temp( int i ) {
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->pushl( eax );
    load_edi();
    _macroAssembler->movl( eax, temp_addr( i ) );
    jump_edi();
    return ep;
}


const char * InterpreterGenerator::push_temp_n() {
    const char * ep = entry_point();
    _macroAssembler->addl( esi, 2 );
    _macroAssembler->movb( ebx, Address( esi, -1 ) );
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( eax, temp_addr( ebx ) );
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::push_arg( int i ) {
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->pushl( eax );
    load_edi();
    _macroAssembler->movl( eax, arg_addr( i ) );
    jump_edi();
    return ep;
}


const char * InterpreterGenerator::push_arg_n() {
    const char * ep = entry_point();
    _macroAssembler->addl( esi, 2 );
    _macroAssembler->movb( ebx, Address( esi, -1 ) );
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( eax, arg_addr( ebx ) );
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::push_smi( bool_t negative ) {
    const char * ep = entry_point();
    _macroAssembler->movb( ebx, Address( esi, 1 ) );    // get b
    _macroAssembler->addl( esi, 2 );            // advance to next bytecode
    _macroAssembler->pushl( eax );            // save tos
    if ( negative ) {
        _macroAssembler->leal( eax, Address( noreg, ebx, Address::ScaleFactor::times_4 ) );
        _macroAssembler->negl( eax );
    } else {
        _macroAssembler->leal( eax, Address( noreg, ebx, Address::ScaleFactor::times_4, 4, RelocationInformation::RelocationType::none ) );
    }
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::push_literal() {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );
    skip_words( 1 );
    load_ebx();
    _macroAssembler->movl( eax, Address( esi, -4 ) );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::push_tos() {
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->pushl( eax );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::push_self() {
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->pushl( eax );
    load_edi();
    _macroAssembler->movl( eax, self_addr() );
    jump_edi();
    return ep;
}


const char * InterpreterGenerator::push_const( Address obj_addr ) {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );
    next_ebx();
    _macroAssembler->movl( eax, obj_addr );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::push_instVar() {
    const char * ep = entry_point();
    advance_aligned( 1 + oopSize );
    _macroAssembler->movl( ecx, self_addr() );
    _macroAssembler->movl( edx, Address( esi, -oopSize ) );
    _macroAssembler->pushl( eax );
    load_ebx();
    _macroAssembler->movl( eax, field_addr( ecx, edx ) );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::store_instVar( bool_t pop ) {
    const char * ep = entry_point();
    advance_aligned( 1 + oopSize );
    _macroAssembler->movl( ecx, self_addr() );
    _macroAssembler->movl( edx, Address( esi, -oopSize ) );
    load_ebx();
    _macroAssembler->movl( field_addr( ecx, edx ), eax );
    _macroAssembler->store_check( ecx, edx );
    if ( pop )
        _macroAssembler->popl( eax );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::return_instVar() {
    const char * ep = entry_point();
    advance_aligned( 1 + oopSize );
    _macroAssembler->movl( ecx, self_addr() );
    _macroAssembler->movl( edx, Address( esi, -oopSize ) );
    _macroAssembler->movl( eax, field_addr( ecx, edx ) );
    return_tos( ByteCodes::ArgumentSpec::recv_0_args );
    return ep;
}


const char * InterpreterGenerator::only_pop() {
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->popl( eax );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::store_temp( int i, bool_t pop ) {
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->movl( temp_addr( i ), eax );
    if ( pop )
        _macroAssembler->popl( eax );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::store_temp_n( bool_t pop ) {
    const char * ep = entry_point();
    _macroAssembler->addl( esi, 2 );
    _macroAssembler->movb( ebx, Address( esi, -1 ) );
    _macroAssembler->movl( temp_addr( ebx ), eax );
    load_ebx();
    if ( pop )
        _macroAssembler->popl( eax );
    jump_ebx();
    return ep;
}


extern "C" void trace_push_global( Oop assoc, Oop value ) {
    ResourceMark resourceMark;
    _console->print_cr( "Trace push_global: " );
    assoc->print_value();
    _console->cr();
    value->print_value();
    _console->cr();
}


const char * InterpreterGenerator::push_global() {
    const char * ep = entry_point();
    skip_words( 1 );
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( ecx, Address( esi, -oopSize ) );                    // get association
    load_ebx();
    _macroAssembler->movl( eax, field_addr( ecx, AssociationOopDescriptor::value_offset() ) );    // get value via association


    if ( false ) { // trace push_global
        _macroAssembler->pushad();
        _macroAssembler->pushl( eax );    // pass arguments (C calling convention)
        _macroAssembler->pushl( ecx );
        _macroAssembler->call_C( ( const char * ) trace_push_global, RelocationInformation::RelocationType::runtime_call_type );
        _macroAssembler->addl( esp, oopSize * 2 );   // get rid of arguments
        _macroAssembler->popad();
    }

    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::store_global( bool_t pop ) {
    const char * ep = entry_point();
    skip_words( 1 );
    _macroAssembler->movl( ecx, Address( esi, -oopSize ) );                    // get association
    load_ebx();
    _macroAssembler->movl( field_addr( ecx, AssociationOopDescriptor::value_offset() ), eax );    // store value via association
    _macroAssembler->store_check( ecx, edx );
    if ( pop )
        _macroAssembler->popl( eax );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::allocate_temps( int n ) {
    const char * ep = entry_point();
    st_assert( n > 0, "just checkin'" );
    next_ebx();
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( eax, nil_addr() );
    while ( --n > 0 )
        _macroAssembler->pushl( eax );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::allocate_temps_n() {
    Label entry, loop;

    _macroAssembler->bind( loop );
    _macroAssembler->pushl( eax );
    _macroAssembler->bind( entry );
    _macroAssembler->decb( ebx );
    _macroAssembler->jcc( Assembler::Condition::notZero, loop );
    load_ebx();
    jump_ebx();

    const char * ep = entry_point();
    _macroAssembler->movb( ebx, Address( esi, 1 ) );        // get n (n = 0 ==> 256 temps)
    _macroAssembler->addl( esi, 2 );                // advance to next bytecode
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( eax, nil_addr() );
    _macroAssembler->jmp( entry );

    return ep;
}


//-----------------------------------------------------------------------------------------
// Context temporaries
//
// Note that eax must always be pushed in the beginning since it may hold the context (temp0).
// (tos is always in eax)

const char * InterpreterGenerator::set_self_via_context() {
    Label loop;
    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->movl( edx, self_addr() );        // get incoming context (stored in receiver)
    _macroAssembler->bind( loop );                // search for home context
    _macroAssembler->movl( ecx, edx );            // save current context
    _macroAssembler->movl( edx, Address( edx, ContextOopDescriptor::parent_byte_offset() ) );
    _macroAssembler->test( edx, MEMOOP_TAG );            // check if parent is_smi
    _macroAssembler->jcc( Assembler::Condition::notZero, loop );        // if not, current context is not home context
    _macroAssembler->movl( edx, Address( ecx, ContextOopDescriptor::temp0_byte_offset() ) );
    _macroAssembler->movl( self_addr(), edx );        // set self in activation frame
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::with_context_temp( bool_t store, int tempNo, int contextNo ) {
    st_assert( contextNo >= -1, "illegal context no." );
    st_assert( tempNo >= -1, "illegal temporary no." );

    Label _loop;
    int   codeSize = 1 + ( contextNo == -1 ? 1 : 0 ) + ( tempNo == -1 ? 1 : 0 );

    const char * ep = entry_point();

    if ( not store ) {
        _macroAssembler->pushl( eax );
    }

    _macroAssembler->movl( ecx, context_addr() );

    if ( contextNo == -1 ) {
        _macroAssembler->movb( ebx, Address( esi, codeSize - 1 ) );
        _macroAssembler->bind( _loop );
        _macroAssembler->movl( ecx, Address( ecx, ContextOopDescriptor::parent_byte_offset() ) );
        _macroAssembler->decb( ebx );
        _macroAssembler->jcc( Assembler::Condition::notZero, _loop );
    } else {
        for ( int i = 0; i < contextNo; i++ )
            _macroAssembler->movl( ecx, Address( ecx, ContextOopDescriptor::parent_byte_offset() ) );
    }

    //slr debugging
    if ( not store and tempNo == 1 and contextNo == 1 ) {
        Label not_frame;
        _macroAssembler->testl( ecx, 1 ); // test for a frame in the context
        _macroAssembler->jcc( Assembler::Condition::notZero, not_frame );
        _macroAssembler->int3();
        _macroAssembler->bind( not_frame );
    }


    Address slot;
    if ( tempNo == -1 ) {
        _macroAssembler->movb( ebx, Address( esi, 1 ) );
        slot = Address( ecx, ebx, Address::ScaleFactor::times_4, ContextOopDescriptor::temp0_byte_offset() );
    } else {
        slot = Address( ecx, ContextOopDescriptor::temp0_byte_offset() + tempNo * oopSize );
    }

    if ( not store ) {
        _macroAssembler->movl( eax, slot );
    } else {
        _macroAssembler->movl( slot, eax );
        _macroAssembler->store_check( ecx, eax );
        _macroAssembler->popl( eax );
    }

    _macroAssembler->addl( esi, codeSize );
    load_ebx();
    jump_ebx();

    return ep;
}

//-----------------------------------------------------------------------------------------
// Copy parameters into context
//

const char * InterpreterGenerator::copy_params_into_context( bool_t self, int paramsCount ) {
    st_assert( paramsCount >= -1, "illegal params count." );

    Label _loop;
    int   oneIfSelf = self ? 1 : 0;

    const char * ep = entry_point();

    _macroAssembler->pushl( eax );                    // save tos (make sure temp0 is in memory)
    _macroAssembler->movl( ecx, context_addr() );
    _macroAssembler->movl( eax, ecx );
    _macroAssembler->store_check( eax, edx );

    if ( self ) {
        // store recv
        _macroAssembler->movl( edx, self_addr() );
        _macroAssembler->movl( Address( ecx, ContextOopDescriptor::temp0_byte_offset() ), edx );
    }

    if ( paramsCount == -1 ) {
        _macroAssembler->addl( esi, 2 );                           // esi points to first parameter index
        _macroAssembler->movb( eax, Address( esi, -1 ) );  // get b (nof params)
        _macroAssembler->bind( _loop );
        _macroAssembler->movb( ebx, Address( esi ) );                  // get parameter index
        _macroAssembler->movl( edx, arg_addr( ebx ) );                 // get parameter
        Address slot = Address( ecx, ContextOopDescriptor::temp0_byte_offset() + oopSize * oneIfSelf );
        _macroAssembler->movl( slot, edx );                // store in context variable
        _macroAssembler->addl( ecx, 4 );
        _macroAssembler->incl( esi );
        _macroAssembler->decb( eax );
        _macroAssembler->jcc( Assembler::Condition::notZero, _loop );
    } else {
        for ( int i = 0; i < paramsCount; i++ ) {
            _macroAssembler->movb( ebx, Address( esi, 1 + i ) );    // get i.th parameter index
            _macroAssembler->movl( edx, arg_addr( ebx ) );                     // get parameter
            Address slot = Address( ecx, ContextOopDescriptor::temp0_byte_offset() + oopSize * ( i + oneIfSelf ) );
            _macroAssembler->movl( slot, edx );                // store (i+oneIfSelf).th in context variable
        }
        _macroAssembler->addl( esi, 1 + paramsCount );
    }

    load_ebx();
    _macroAssembler->popl( eax );                    // restore tos
    jump_ebx();

    return ep;
}


//-----------------------------------------------------------------------------------------
//
// Blocks
//
// push_closure allocates and pushes a (block) closure on tos. The closure's fields
// are initialized depending on the function arguments allocation_routine and use_context.
// allocation_routine is the entry point used to allocate the block. If use_context is true,
// this frame's context is used for the closure initialization, otherwise tos is used instead.
// Additionally, whenever a block is created, its surrounding block or method's invocation
// counter is incremented.

/*
extern "C" Oop allocateBlock(SMIOop nofArgs);	// Note: needs last Delta frame setup!

// Note: The following routines don't need the last Delta frame to be setup
extern "C" Oop allocateBlock0();
extern "C" Oop allocateBlock1();
extern "C" Oop allocateBlock2();
*/

const char * InterpreterGenerator::push_closure( int nofArgs, bool_t use_context ) {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );                            // save tos
    if ( nofArgs == -1 ) {
        // no. of arguments specified by 2nd byte
        _macroAssembler->movb( ebx, Address( esi, 1 ) );    //  get no. of arguments
        advance_aligned( 2 + oopSize );                      // go to next instruction
        _macroAssembler->shll( ebx, TAG_SIZE );                            // convert into smi_t (pushed on the stack!)
        save_esi();                                             // save vital registers
        _macroAssembler->pushl( ebx );                                     // pass as argument
        _macroAssembler->set_last_Delta_frame_before_call();               // allocateBlock needs last Delta frame!
        _macroAssembler->call( GeneratedPrimitives::allocateBlock( nofArgs ), RelocationInformation::RelocationType::runtime_call_type );    // eax: = block closure(nof. args)
        _macroAssembler->reset_last_Delta_frame();
        _macroAssembler->popl( ebx );                            // get rid of argument
    } else {
        // no. of arguments implied by 1st byte
        advance_aligned( 1 + oopSize );                    // go to next instruction
        save_esi();                                // no last Delta frame setup needed => save vital registers
        _macroAssembler->call( GeneratedPrimitives::allocateBlock( nofArgs ), RelocationInformation::RelocationType::runtime_call_type );    // eax: = block closure
    }
    restore_esi();                            // returning from C land => restore esi (ebx is restored later)
    _macroAssembler->movl( ecx, Address( esi, -oopSize ) );                // get block methodOop
    if ( use_context ) {                            // if full block then
        _macroAssembler->movl( edx, context_addr() );                    //   get context of this activation
        if ( _debug ) {
            // should check if edx is really pointing to a context
            // (can it ever happen that temp0 is not holding a context
            // but push_closure is used with the use_context attribute?)
        }
    } else {                                // else
        _macroAssembler->popl( edx );                            //   use tos as context information
    }


    // install methodOop and context in block closure and increment the invocation counter of the parent (aka "enclosing") methodOop
    //
    // eax: block closure
    // ecx: block methodOop
    // edx: context
    _macroAssembler->movl( ebx, Address( ecx, MethodOopDescriptor::selector_or_method_byte_offset() ) );       // get parent (= running) methodOop
    _macroAssembler->movl( Address( eax, BlockClosureOopDescriptor::method_or_entry_byte_offset() ), ecx );    // set block method
    _macroAssembler->movl( ecx, Address( ebx, MethodOopDescriptor::counters_byte_offset() ) );                 // get counter of parent methodOop
    _macroAssembler->movl( Address( eax, BlockClosureOopDescriptor::context_byte_offset() ), edx );            // set context
    _macroAssembler->addl( ecx, 1 << MethodOopDescriptor::_invocation_count_offset );                       // increment invocation counter of parent methodOop
    _macroAssembler->movl( edx, eax );                                                                             // make sure eax is not destroyed
    _macroAssembler->movl( Address( ebx, MethodOopDescriptor::counters_byte_offset() ), ecx );                 // store counter of parent methodOop
    restore_ebx();
    load_ebx();                                                                                         // get next instruction
    _macroAssembler->store_check( edx, ecx );                                                                      // do a store check on edx, use ecx as scratch register
    jump_ebx();
    return ep;
}


//-----------------------------------------------------------------------------------------
// Contexts
//
// install_context allocates and installs a (heap) context in temp0. The context's
// fields are initialized depending on the function arguments allocation_routine and
// for_method. allocation_routine is the entry point used to allocate the context. If
// for_method is true, the current frame pointer (ebp) will be the context's parent,
// otherwise the (incoming) context will be used as parent context.

/*
// Note: The following routines don't need the last Delta frame to be setup
extern "C" Oop allocateContext(SMIOop nofVars);
extern "C" Oop allocateContext0();
extern "C" Oop allocateContext1();
extern "C" Oop allocateContext2();
*/

const char * InterpreterGenerator::install_context( int nofArgs, bool_t for_method ) {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );                // save tos
    if ( nofArgs == -1 ) {
        // no. of variables specified by 2nd byte
        _macroAssembler->movb( ebx, Address( esi, 1 ) );   // get no. of variables
        _macroAssembler->addl( esi, 2 );                           // go to next instruction
        _macroAssembler->shll( ebx, TAG_SIZE );                            // convert into smi_t (pushed on the stack!)
        save_esi();                                             // no last Delta frame setup needed => save vital registers
        _macroAssembler->pushl( ebx );                                     // pass as argument
        _macroAssembler->call( GeneratedPrimitives::allocateContext( nofArgs ), RelocationInformation::RelocationType::runtime_call_type );        // eax: = context(nof. vars)
        _macroAssembler->popl( ebx );                  // get rid of argument
    } else {
        // no. of variables implied by 1st byte
        _macroAssembler->incl( esi );                  // go to next instruction
        save_esi();                         // no last Delta frame setup needed => save vital registers
        _macroAssembler->call( GeneratedPrimitives::allocateContext( nofArgs ), RelocationInformation::RelocationType::runtime_call_type );        // eax: = context
    }
    restore_esi();                // returning from C land => restore vital registers
    restore_ebx();
    if ( for_method ) {                // if method context then
        _macroAssembler->movl( Address( eax, ContextOopDescriptor::parent_byte_offset() ), ebp );    // parent points to method frame
    } else {                    // else
        _macroAssembler->movl( ecx, context_addr() );        // get (incoming) enclosing context
        if ( _debug ) {
            // should check if ecx is really pointing to a context
            // (can it ever happen that temp0 is not holding a context
            // but install_context is used with the use_context attribute?)
        }
        _macroAssembler->movl( Address( eax, ContextOopDescriptor::parent_byte_offset() ), ecx );    // parent points to enclosing context
    }
    load_ebx();                             // get next instruction
    _macroAssembler->movl( context_addr(), eax );  // install context
    _macroAssembler->store_check( eax, ecx );          // store check on eax, use ecx as scratch register
    _macroAssembler->popl( eax );                      // restore tos
    jump_ebx();

    return ep;
}

//-----------------------------------------------------------------------------------------
// Control structures and jumps
//
// Jump offsets are unsigned bytes/words. For forward jumps, the jump destination
// is the address of the next instruction + offset, for backward jumps (whileTrue
// and whileFalse) the jump destination is the current instruction's address - offset.
//

const char * InterpreterGenerator::control_cond( ByteCodes::Code code ) {

    bool_t isByte, isTrue, isCond;

    switch ( code ) {
        case ByteCodes::Code::ifTrue_byte:
            isByte = true;
            isTrue = true;
            isCond = false;
            break;
        case ByteCodes::Code::ifTrue_word:
            isByte = false;
            isTrue = true;
            isCond = false;
            break;
        case ByteCodes::Code::ifFalse_byte:
            isByte = true;
            isTrue = false;
            isCond = false;
            break;
        case ByteCodes::Code::ifFalse_word:
            isByte = false;
            isTrue = false;
            isCond = false;
            break;
        case ByteCodes::Code::and_byte:
            isByte = true;
            isTrue = true;
            isCond = true;
            break;
        case ByteCodes::Code::and_word:
            isByte = false;
            isTrue = true;
            isCond = true;
            break;
        case ByteCodes::Code::or_byte:
            isByte = true;
            isTrue = false;
            isCond = true;
            break;
        case ByteCodes::Code::or_word:
            isByte = false;
            isTrue = false;
            isCond = true;
            break;
        default: ShouldNotReachHere();
    }

    Label   _else;
    Address cond     = isTrue ? true_addr() : false_addr();
    Address not_cond = not isTrue ? true_addr() : false_addr();
    int     codeSize = ( isCond ? 1 : 2 ) + ( isByte ? 1 : 4 );

    const char * ep = entry_point();

    if ( not isByte ) {
        advance_aligned( codeSize );
    }
    _macroAssembler->cmpl( eax, cond );                    // if tos # cond
    _macroAssembler->jcc( Assembler::Condition::notEqual, _else );            // then jump to else part
    if ( isByte ) {
        _macroAssembler->addl( esi, codeSize );                    // skip info & offset byte
    }
    load_ebx();
    _macroAssembler->popl( eax );                        // discard condition
    jump_ebx();

    _macroAssembler->bind( _else );
    _macroAssembler->cmpl( eax, not_cond );                    // if tos # ~cond
    _macroAssembler->jcc( Assembler::Condition::notEqual, _boolean_expected );        // then non-boolean arguments

    // jump relative to next instr (must happen after the check for non-booleans)
    if ( isByte ) {
        _macroAssembler->movb( ebx, Address( esi, codeSize - 1 ) );
        _macroAssembler->leal( esi, Address( esi, ebx, Address::ScaleFactor::times_1, codeSize ) );
    } else {
        _macroAssembler->addl( esi, Address( esi, -oopSize ) );
    }
    load_ebx();
    if ( not isCond ) {
        _macroAssembler->popl( eax );                        // discard condition
    }
    jump_ebx();

    return ep;
}


const char * InterpreterGenerator::control_while( ByteCodes::Code code ) {

    bool_t isByte, isTrue;

    switch ( code ) {
        case ByteCodes::Code::whileTrue_byte:
            isByte = true;
            isTrue = true;
            break;
        case ByteCodes::Code::whileTrue_word:
            isByte = false;
            isTrue = true;
            break;
        case ByteCodes::Code::whileFalse_byte:
            isByte = true;
            isTrue = false;
            break;
        case ByteCodes::Code::whileFalse_word:
            isByte = false;
            isTrue = false;
            break;
        default: ShouldNotReachHere();
    }

    Label _exit, _overflow, _call_overflow;

    Address cond     = isTrue ? true_addr() : false_addr();
    Address not_cond = not isTrue ? true_addr() : false_addr();
    int     codeSize = 1 + ( isByte ? 1 : oopSize );

    const char * ep = entry_point();

    _macroAssembler->cmpl( eax, cond );                       // if tos # cond
    _macroAssembler->jcc( Assembler::Condition::notEqual, _exit ); // then jump to else part

    if ( isByte ) {
        _macroAssembler->movb( ebx, Address( esi, codeSize - 1 ) );
        _macroAssembler->subl( esi, ebx );
    } else {
        _macroAssembler->leal( edx, Address( esi, codeSize + 3 ) );         // unaligned address of next instruction
        _macroAssembler->andl( edx, ~3 );                                           // aligned address of next instruction
        _macroAssembler->subl( esi, Address( edx, -oopSize ) );
    }

    _macroAssembler->movl( edx, Address( ( int ) &Interpreter::_interpreter_loop_counter, RelocationInformation::RelocationType::external_word_type ) );
    load_ebx();
    _macroAssembler->popl( eax ); // discard loop condition
    _macroAssembler->incl( edx );
    _macroAssembler->movl( Address( ( int ) &Interpreter::_interpreter_loop_counter, RelocationInformation::RelocationType::external_word_type ), edx );
    _macroAssembler->cmpl( edx, Address( ( int ) &Interpreter::_interpreter_loop_counter_limit, RelocationInformation::RelocationType::external_word_type ) );
    _macroAssembler->jcc( Assembler::Condition::greater, _overflow );
    jump_ebx();

    _macroAssembler->bind( _exit );
    _macroAssembler->cmpl( eax, not_cond );                                     // if tos # ~cond
    _macroAssembler->jcc( Assembler::Condition::notEqual, _boolean_expected );       // then non-boolean arguments

    // advance to next instruction (must happen after the check for non-booleans)
    if ( isByte ) {
        _macroAssembler->addl( esi, codeSize );
    } else {
        _macroAssembler->leal( edx, Address( esi, codeSize + 3 ) );        // unaligned address of next instruction
        _macroAssembler->andl( edx, ~3 );                                           // aligned address of next instruction
        _macroAssembler->movl( esi, edx );
    }
    load_ebx();
    _macroAssembler->popl( eax ); // discard loop condition
    stack_check_pop();
    jump_ebx();

    _macroAssembler->bind( _overflow );
    call_C( _call_overflow );
    load_ebx();
    jump_ebx();

    _macroAssembler->bind( _call_overflow );
    _macroAssembler->pushl( eax );
    _macroAssembler->call( ( const char * ) &Interpreter::loop_counter_overflow, RelocationInformation::RelocationType::runtime_call_type );
    _macroAssembler->popl( eax );
    _macroAssembler->ret( 0 );

    return ep;
}


const char * InterpreterGenerator::control_jump( ByteCodes::Code code ) {

    bool_t isByte, isLoop;

    switch ( code ) {
        case ByteCodes::Code::jump_else_byte:
            isByte = true;
            isLoop = false;
            break;
        case ByteCodes::Code::jump_else_word:
            isByte = false;
            isLoop = false;
            break;
        case ByteCodes::Code::jump_loop_byte:
            isByte = true;
            isLoop = true;
            break;
        case ByteCodes::Code::jump_loop_word:
            isByte = false;
            isLoop = true;
            break;
        default: ShouldNotReachHere();
    }

    int codeSize = 1 + ( isByte ? 1 : oopSize ) * ( isLoop ? 2 : 1 );

    const char * ep = entry_point();

    if ( isLoop ) {
        stack_check_push();
    }

    if ( isByte ) {
        _macroAssembler->movb( ebx, Address( esi, codeSize - 1 ) ); // get jump offset
        _macroAssembler->leal( esi, Address( esi, ebx, Address::ScaleFactor::times_1, codeSize ) ); // jump destination
    } else {
        advance_aligned( codeSize );
        _macroAssembler->addl( esi, Address( esi, -oopSize ) ); // jump destination
    }

    load_ebx();
    jump_ebx();

    return ep;
}


//-----------------------------------------------------------------------------------------
// Floating-point operations
//
// These operations work in the float section of (interpreted) activation frames.
// Within that float section, floats are indexed from max_nof_floats - 1 to 0.
// Operations that work on floats have as 2nd byte the float index of the result.
//
// If there's a float section, the first 2 'normal' temporaries are always there,
// and temp1 holds a magic value which allows the GC to determine fast whether
// there is untagged data on the stack or not.
//
//
// Stack layout if there's a float section:
//
// eax	  top of stack
// esp->[ expressions	]
// 	  ...
// 	[ expressions	] <---- start of expression stack
//	[ float m-1	] <----	last float in float section
//	[ ...		]
// 	    [ float 0	] <----	first float in float section
// 	    [ temporary n-1	] <----	last 'normal' temporary (n is a multiple of 2 and n >= 2)
// 	    ...
// 	    [ temporary 1	]	always here if there's a float section (holds Floats::magic)
//	    [ temporary 0	]	always here if there's a float section
//	    ...
// ebp->[ previous ebp	]
//
//
// Note that (as always) the top of expression stack is hold in eax; i.e., if the
// expression stack is empty and there's a float section, the lower-half of the last
// float is kept in eax! This is not a problem, however one has to make sure to push
// eax before every float operation in order to have a floats completely in memory
// (this is not an extra burden since eax often has to be saved anyway).

const char * InterpreterGenerator::float_allocate() {
// Allocates (additional) temps and floats in a stack frame.
// Bytecode format:
//
// <float_allocate code> <nofTemps> <nofFloats> <floatExprStackSize>
//
// <nofTemps>			no. of additional temps to allocate (in chunks of two) besides temp0 & temp1
// <nofFloats>			no. of initialized floats to allocate
// <floatExprStackSize>		no. of uninitialized floats to allocate

    Label tLoop, tDone, fLoop, fDone;
//    st_assert( Oop( Floats::magic )->is_smi(), "InterpreterGenerator::float_allocate():  must be a smi_t" );
    const char * ep = entry_point();
    if ( _debug ) {
        // This instruction must be the first bytecode executed in a method (if there).
        Label L1, L2;
        // check stack pointer (must point to esi save location, temp0 is in eax)
        _macroAssembler->leal( ecx, Address( ebp, esi_offset ) );
        _macroAssembler->cmpl( esp, ecx );
        _macroAssembler->jcc( Assembler::Condition::equal, L1 );
        _macroAssembler->call_C( ( const char * ) Interpreter::wrong_esp, RelocationInformation::RelocationType::runtime_call_type );
        should_not_reach_here();
        _macroAssembler->bind( L1 );

        // check eax (corresponds now to temp0, must be initialized to nil)
        _macroAssembler->cmpl( eax, nil_addr() );
        _macroAssembler->jcc( Assembler::Condition::equal, L2 );
        _macroAssembler->call_C( ( const char * ) Interpreter::wrong_eax, RelocationInformation::RelocationType::runtime_call_type );
        should_not_reach_here();
        _macroAssembler->bind( L2 );
    }
    _macroAssembler->addl( esi, 4 );                // advance to next bytecode
    _macroAssembler->pushl( eax );                // save tos (i.e. temp0)
    _macroAssembler->pushl( Floats::magic );            // initialize temp1 (indicates a float section)

    // allocate additional temps in multiples of 2 (to compensate for one float)
    _macroAssembler->movb( ebx, Address( esi, -3 ) );        // get nofTemps
    _macroAssembler->testl( ebx, ebx );            // allocate no additional temps if nofTemps = 0
    _macroAssembler->jcc( Assembler::Condition::zero, tDone );
    _macroAssembler->movl( eax, nil_addr() );
    _macroAssembler->bind( tLoop );
    _macroAssembler->pushl( eax );                // push nil
    _macroAssembler->pushl( eax );                // push nil
    _macroAssembler->decl( ebx );
    _macroAssembler->jcc( Assembler::Condition::notZero, tLoop );
    _macroAssembler->bind( tDone );

    // allocate floats
    _macroAssembler->movb( ebx, Address( esi, -2 ) );        // get nofFloats
    _macroAssembler->testl( ebx, ebx );            // allocate no additional floats if nofFloats = 0
    _macroAssembler->jcc( Assembler::Condition::zero, fDone );
    _macroAssembler->xorl( eax, eax );            // use 0 to initialize the stack with 0.0
    _macroAssembler->bind( fLoop );
    _macroAssembler->pushl( eax );                // push 0.0 (allocate a double)
    _macroAssembler->pushl( eax );
    _macroAssembler->decb( ebx );
    _macroAssembler->jcc( Assembler::Condition::notZero, fLoop );
    _macroAssembler->bind( fDone );

    // allocate floats expression stack
    st_assert( SIZEOF_FLOAT == 8, "change the constant for shll below" );
    _macroAssembler->movb( ebx, Address( esi, -1 ) );        // get floats expression stack size
    _macroAssembler->shll( ebx, 3 );                // multiply with FLOAT_SIZE
    _macroAssembler->subl( esp, ebx );            // adjust esp
    restore_ebx();

    // continue with next instruction
    load_ebx();                    // continue with next instruction
    _macroAssembler->popl( eax );                // restore tos in eax
    jump_ebx();

    return ep;
}


const char * InterpreterGenerator::float_floatify() {
    Label is_smi;
    const char * ep = entry_point();
    _macroAssembler->addl( esi, 2 );                // advance to next instruction
    _macroAssembler->testb( eax, MEMOOP_TAG );            // check if smi_t
    _macroAssembler->jcc( Assembler::Condition::zero, is_smi );
    _macroAssembler->movl( ecx, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // check if float
    _macroAssembler->cmpl( ecx, doubleKlass_addr() );
    _macroAssembler->jcc( Assembler::Condition::notEqual, _float_expected );

    // unbox DoubleOop
    _macroAssembler->movb( ebx, Address( esi, -1 ) );        // get float number
    _macroAssembler->fld_d( Address( eax, byteOffset( DoubleOopDescriptor::value_offset() ) ) ); // unbox float
    _macroAssembler->fstp_d( float_addr( ebx ) );        // store float
    load_ebx();
    _macroAssembler->popl( eax );                // discard argument
    jump_ebx();

    // convert smi_t
    _macroAssembler->bind( is_smi );
    _macroAssembler->movb( ebx, Address( esi, -1 ) );        // get float number
    _macroAssembler->leal( ecx, float_addr( ebx ) );
    _macroAssembler->sarl( eax, TAG_SIZE );            // convert smi_t argument into int
    _macroAssembler->movl( Address( ecx ), eax );        // store it in memory (use float target location)
    _macroAssembler->fild_s( Address( ecx ) );            // convert it into float
    _macroAssembler->fstp_d( Address( ecx ) );            // store float
    load_ebx();
    _macroAssembler->popl( eax );                // discard argument
    jump_ebx();

    return ep;
}


const char * InterpreterGenerator::float_oopify() {
    // Implements the Floats::oopify operation. It is implemented
    // here rather than in Floats because it needs to do a C call.
    // Instead of returning regularly, it directly continues with
    // the next byte code.
    const char * ep = entry_point();
    // here the return address to float_op is on the stack
    // discard it so that C routine can be called regularly.
    _macroAssembler->popl( eax );                // discard return address
    _macroAssembler->fpop();                // pop ST (in order to avoid FPU stack overflows) -> get rid of argument
    call_C( ( const char * ) Interpreter::oopify_FloatValue );// eax: = oopify_FloatValue() (gets its argument by looking at the last bytecode)
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::float_move() {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );                // make sure last float is completely in memory
    _macroAssembler->addl( esi, 3 );                // advance to next instruction
    _macroAssembler->xorl( ecx, ecx );            // clear ecx
    _macroAssembler->movb( ebx, Address( esi, -1 ) );        // get source float number
    _macroAssembler->movb( ecx, Address( esi, -2 ) );        // get destination float number
    _macroAssembler->fld_d( float_addr( ebx ) );        // load source
    load_ebx();
    _macroAssembler->fstp_d( float_addr( ecx ) );        // store at destination
    _macroAssembler->popl( eax );                // re-adjust esp
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::float_set() {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );                // make sure last float is completely in memory
    _macroAssembler->movb( ebx, Address( esi, 1 ) );        // get float number
    advance_aligned( 2 + oopSize );            // advance to next instruction
    _macroAssembler->movl( ecx, Address( esi, -oopSize ) );    // get DoubleOop address
    _macroAssembler->fld_d( Address( ecx, byteOffset( DoubleOopDescriptor::value_offset() ) ) ); // unbox float
    _macroAssembler->fstp_d( float_addr( ebx ) );        // store it
    load_ebx();
    _macroAssembler->popl( eax );                // re-adjust esp
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::float_op( int nof_args, bool_t returns_float ) {
    st_assert( 0 <= nof_args and nof_args <= 8, "illegal nof_args specification" );
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );                // make sure all floats are completely in memory
    _macroAssembler->addl( esi, 3 );                // advance to next instruction
    _macroAssembler->movb( ebx, Address( esi, -2 ) );        // get float number
    _macroAssembler->leal( edx, float_addr( ebx ) );        // get float address
    _macroAssembler->movb( ebx, Address( esi, -1 ) );        // get function number
    _macroAssembler->movl( ecx, Address( noreg, ebx, Address::ScaleFactor::times_4, int( Floats::_function_table[ 0 ] ), RelocationInformation::RelocationType::external_word_type ) );
    for ( int i = 0; i < nof_args; i++ )
        _macroAssembler->fld_d( Address( edx, -i * SIZEOF_FLOAT ) );
    _macroAssembler->call( ecx );                // invoke operation
    load_ebx();                    // get next byte code
    if ( returns_float ) {
        _macroAssembler->fstp_d( Address( edx ) );        // store result
        _macroAssembler->popl( eax );                // re-adjust esp
    }                        // otherwise: result in eax
    jump_ebx();
    return ep;
}

//-----------------------------------------------------------------------------------------
// Primitive calls
//
// Note: In general, when calling the VM, esi should point to the next bytecode
//       instruction. This is not the case when calling primitives::lookup_and_patch()
//       in lookup_primitive(). However, esi (i.e. f.hp()) is adjusted in the
//       lookup_and_patch routine.

const char * InterpreterGenerator::predict_prim( bool_t canFail ) {
    // _predict_prim & _predict_prim_ifFail are two bytecodes that are
    // used during lookup, during execution they can be simply ignored.
    const char * ep = entry_point();
    advance_aligned( 1 + ( canFail ? 2 : 1 ) * oopSize );
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::lookup_primitive() {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );                // push last argument
    call_C( ( const char * ) Primitives::lookup_and_patch );    // do the lookup and patch call site appropriately
    load_ebx();
    _macroAssembler->popl( eax );                // restore last argument
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::call_primitive() {
    const char * ep = entry_point();
    advance_aligned( 1 + oopSize );
    _macroAssembler->pushl( eax );                // push last argument
    _macroAssembler->movl( eax, Address( esi, -oopSize ) );    // get primitive entry point
    call_C( eax );                    // eax: = primitive call(...)
    if ( _debug ) {                    // (Pascal calling conv. => args are popped by callee)
        _macroAssembler->testb( eax, MARK_TAG_BIT );
        _macroAssembler->jcc( Assembler::Condition::notZero, _primitive_result_wrong );
    }
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::call_primitive_can_fail() {
    Label failed;
    const char * ep = entry_point();
    advance_aligned( 1 + 2 * oopSize );
    _macroAssembler->pushl( eax );                // push last argument
    _macroAssembler->movl( eax, Address( esi, -2 * oopSize ) );    // get primitive entry point
    call_C( eax );                    // eax: = primitive call(...) (Pascal calling conv.)
    _macroAssembler->testb( eax, MARK_TAG_BIT );        // if not marked then
    _macroAssembler->jcc( Assembler::Condition::notZero, failed );
    _macroAssembler->movl( ecx, Address( esi, -oopSize ) );    // get jump offset
    _macroAssembler->addl( esi, ecx );            // jump over failure block
    load_ebx();
    jump_ebx();

    _macroAssembler->bind( failed );
    _macroAssembler->andl( eax, ~MARK_TAG_BIT );        // unmark result
    load_ebx();                    // and execute failure block
    jump_ebx();
    return ep;
}


//-----------------------------------------------------------------------------------------
// DLL calls
//
// Byte code structure:
//
// 	[send		]	(1 byte)
//	....alignment....	(0..3 bytes)
//	[dll_name	]	(1 dword)
//	[function_name	]	(1 dword)
//	[0/function ptr	]	(1 dword)
//	[no. of Args	]	(1 byte)
//	...	  	  <---	esi
//
// The arguments of a dll call are either smis or proxy objects, i.e. boxed 32 bit words
// held in a proxy object.
//
// Before calling the dll function, all arguments are con verted into C arguments, i.e. smis are divided by 4 and boxed 32 bit words hold
// in proxys are unboxed.
// The arguments are then pushed in reversed order to comply with the C calling conventions. The result of the dll call is boxed again into
// a proxy object which has been pushed before the arguments.
//
// Because there are Delta and C frames on the same stack and the innards of the
// DLL call are unknown, its call is set up in an additional frame that behaves
// like a C frame (and that has its code outside the interpreters code) but that
// contains a frame pointer which can be used to properly set the last Delta frame.
//
// The call_DLL_entry routine (see StubRoutines) needs 3 arguments to be passed
// in registers as follows:
//
// ebx: no. of dll function arguments
// ecx: address of last dll function argument
// edx: dll function entry point
//
// NonLocalReturns through DLL calls: Note that if the DLL is returning via an NonLocalReturn, the
// arguments don't need to be popped since the NonLocalReturn is simply returning too
// (as for ordinary NonLocalReturns). Thus, NonLocalReturns are just propagated as usual.

const char * InterpreterGenerator::call_DLL( bool_t async ) {

    const char * ep = entry_point();
    Label L;
    advance_aligned( 1 + 3 * oopSize );                 // advance to no. of arguments byte
    _macroAssembler->incl( esi );                                  // advance to next instruction (skip no. of arguments byte)
    _macroAssembler->pushl( eax );                                 // push last argument
    _macroAssembler->movl( edx, Address( esi, -1 - oopSize ) );    // get dll function ptr
    _macroAssembler->testl( edx, edx );                            // test if function has been looked up already
    _macroAssembler->jcc( Assembler::Condition::notZero, L );                 // and continue - otherwise lookup dll function & patch
    call_C( ( const char * ) DLLs::lookup_and_patch_Interpreted_DLLCache );    // eax: = returns dll function ptr
    _macroAssembler->movl( edx, eax );                             // move dll function ptr into right register
    _macroAssembler->bind( L );                                    // and continue
    _macroAssembler->movb( ebx, Address( esi, -1 ) );              // get no. of arguments
    _macroAssembler->movl( ecx, esp );                             // get address of last argument
    save_esi();                                         // don't use call_C because no last_Delta_frame setup needed
    _macroAssembler->call( StubRoutines::call_DLL_entry( async ), RelocationInformation::RelocationType::runtime_call_type ); // eax: = DLL call via a separate frame (parameter conversion)
    _macroAssembler->ic_info( _nlr_testpoint, 0 );
    restore_esi();
    restore_ebx();
    _macroAssembler->movb( ebx, Address( esi, -1 ) );                      // get no. of arguments
    _macroAssembler->leal( esp, Address( esp, ebx, Address::ScaleFactor::times_4 ) );   // pop arguments
    _macroAssembler->popl( ecx );                                          // get proxy object
    _macroAssembler->movl( Address( ecx, pointer_offset ), eax );          // box result
    load_ebx();
    _macroAssembler->movl( eax, ecx );                                     // return proxy
    jump_ebx();
    return ep;
}


//-----------------------------------------------------------------------------------------
// Redo send code
//
// Used to restart a send (issued via call_native) that called a zombie NativeMethod.
// The receiver (if not a self or super send) as well as the arguments are still
// on the stack and esi has been reset to the send byte code.

const char * Interpreter::_redo_send_entry = nullptr;


void InterpreterGenerator::generate_redo_send_code() {
    st_assert( Interpreter::_redo_send_entry == nullptr, "code generated twice" );
    Interpreter::_redo_send_entry = _macroAssembler->pc();
    restore_esi();                // has been saved by call_native
    restore_ebx();                // possibly destroyed
    load_ebx();
    _macroAssembler->popl( eax );                // get last argument into eax again
    jump_ebx();                    // restart send
}


//-----------------------------------------------------------------------------------------
// Return entry points for deoptimized interpreter frames
//
//  There is may ways of returning from an interpreter frame.
//    - from send  (with or without pop) X (with or without receiver) X (with or without restoring result value)
//    - from primitive call (with or without failure block)           X (with or without restoring result value)
//    - from DLL call                                                   (with or without restoring result value)

const char * Interpreter::_dr_from_send_without_receiver                        = nullptr;
const char * Interpreter::_dr_from_send_without_receiver_restore                = nullptr;
const char * Interpreter::_dr_from_send_without_receiver_pop                    = nullptr;
const char * Interpreter::_dr_from_send_without_receiver_pop_restore            = nullptr;
const char * Interpreter::_dr_from_send_with_receiver                           = nullptr;
const char * Interpreter::_dr_from_send_with_receiver_restore                   = nullptr;
const char * Interpreter::_dr_from_send_with_receiver_pop                       = nullptr;
const char * Interpreter::_dr_from_send_with_receiver_pop_restore               = nullptr;
const char * Interpreter::_dr_from_primitive_call_without_failure_block         = nullptr;
const char * Interpreter::_dr_from_primitive_call_without_failure_block_restore = nullptr;
const char * Interpreter::_dr_from_primitive_call_with_failure_block            = nullptr;
const char * Interpreter::_dr_from_primitive_call_with_failure_block_restore    = nullptr;
const char * Interpreter::_dr_from_dll_call                                     = nullptr;
const char * Interpreter::_dr_from_dll_call_restore                             = nullptr;

extern "C" int number_of_arguments_through_unpacking;
extern "C" Oop result_through_unpacking;


void InterpreterGenerator::generate_deoptimized_return_restore() {
    _macroAssembler->movl( eax, Address( ( int ) &number_of_arguments_through_unpacking, RelocationInformation::RelocationType::external_word_type ) );
    _macroAssembler->shll( eax, 2 );
    _macroAssembler->addl( esp, eax );
    _macroAssembler->movl( eax, Address( ( int ) &result_through_unpacking, RelocationInformation::RelocationType::external_word_type ) );
}


void InterpreterGenerator::generate_deoptimized_return_code() {
    st_assert( Interpreter::_dr_from_dll_call == nullptr, "code generated twice" );

//#define  maybeINT3() _masm->int3();
#define  maybeINT3()

    Label deoptimized_C_nlr_continuation;
    Label deoptimized_nlr_continuation;

    _macroAssembler->bind( deoptimized_C_nlr_continuation );
    _macroAssembler->reset_last_Delta_frame();
    // fall through
    _macroAssembler->bind( deoptimized_nlr_continuation );
    // mov	eax, [_nlr_result]
    _macroAssembler->jmp( _nlr_testpoint );

    Interpreter::_dr_from_send_without_receiver_restore = _macroAssembler->pc();
    generate_deoptimized_return_restore();
    // fall through

    Interpreter::_dr_from_send_without_receiver = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_nlr_continuation, 0 ); // last part of the _call_method macro
    maybeINT3()
    restore_esi();
    restore_ebx();
    load_ebx();
    jump_ebx();

    Interpreter::_dr_from_send_without_receiver_pop_restore = _macroAssembler->pc();
    generate_deoptimized_return_restore();
    // fall through

    Interpreter::_dr_from_send_without_receiver_pop = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_nlr_continuation, 0 ); // last part of the _call_method macro
    maybeINT3()
    restore_esi();
    restore_ebx();
    load_ebx();
    _macroAssembler->popl( eax );        // pop result
    jump_ebx();

    Interpreter::_dr_from_send_with_receiver_restore = _macroAssembler->pc();
    generate_deoptimized_return_restore();
    // fall through

    Interpreter::_dr_from_send_with_receiver = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_nlr_continuation, 0 ); // last part of the _call_method macro
    maybeINT3()
    restore_esi();
    restore_ebx();
    _macroAssembler->popl( ecx );        // pop receiver
    load_ebx();
    jump_ebx();

    Interpreter::_dr_from_send_with_receiver_pop_restore = _macroAssembler->pc();
    generate_deoptimized_return_restore();
    // fall through

    Interpreter::_dr_from_send_with_receiver_pop = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_nlr_continuation, 0 ); // last part of the _call_method macro
    maybeINT3()
    restore_esi();
    restore_ebx();
    _macroAssembler->popl( ecx );        // pop receiver
    load_ebx();
    _macroAssembler->popl( eax );        // pop result
    jump_ebx();

    Interpreter::_dr_from_primitive_call_without_failure_block_restore = _macroAssembler->pc();
    generate_deoptimized_return_restore();
    // fall through

    Interpreter::_dr_from_primitive_call_without_failure_block = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_C_nlr_continuation, 0 );
    maybeINT3()
    restore_esi();
    restore_ebx();                  // ebx: = 0
    _macroAssembler->reset_last_Delta_frame();
    if ( _debug ) {
        _macroAssembler->test( eax, MARK_TAG_BIT );
        _macroAssembler->jcc( Assembler::Condition::notZero, _primitive_result_wrong );
    }
    load_ebx();
    jump_ebx();

    Label _deoptimized_return_from_primitive_call_with_failure_block_failed;

    Interpreter::_dr_from_primitive_call_with_failure_block_restore = _macroAssembler->pc();
    generate_deoptimized_return_restore();
    // fall through

    Interpreter::_dr_from_primitive_call_with_failure_block = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_C_nlr_continuation, 0 );
    maybeINT3()
    restore_esi();
    restore_ebx();                  // ebx: = 0
    _macroAssembler->reset_last_Delta_frame();
    _macroAssembler->test( eax, MARK_TAG_BIT );          // if not marked then
    _macroAssembler->jcc( Assembler::Condition::notZero, _deoptimized_return_from_primitive_call_with_failure_block_failed );
    _macroAssembler->movl( ecx, Address( esi, -oopSize ) );      // load jump offset
    _macroAssembler->addl( esi, ecx );                  // and jump over failure block
    load_ebx();
    jump_ebx();

    _macroAssembler->bind( _deoptimized_return_from_primitive_call_with_failure_block_failed );
    _macroAssembler->andl( eax, ~MARK_TAG_BIT );        // else unmark result
    load_ebx();                    // and execute failure block
    jump_ebx();                    // the result will be stored
    // into a temp in the failure block

    Interpreter::_dr_from_dll_call_restore = _macroAssembler->pc();
    _macroAssembler->movl( eax, Address( ( int ) &result_through_unpacking, RelocationInformation::RelocationType::external_word_type ) );
    // fall through

    Interpreter::_dr_from_dll_call = _macroAssembler->pc();
    _macroAssembler->ic_info( deoptimized_C_nlr_continuation, 0 );
    maybeINT3()
    _macroAssembler->reset_last_Delta_frame();
    restore_esi();
    restore_ebx();                  // ebx: = 0
    // eax: DLL result
    _macroAssembler->movb( ebx, Address( esi, -1 ) );                    // get no. of arguments
    _macroAssembler->leal( esp, Address( esp, ebx, Address::ScaleFactor::times_4 ) );            // adjust sp (pop arguments)
    _macroAssembler->popl( ecx );                            // get proxy object
    _macroAssembler->movl( Address( ecx, ProxyOopDescriptor::pointer_byte_offset() ), eax );   // box result
    load_ebx();
    _macroAssembler->movl( eax, ecx );                            // return proxy
    jump_ebx();
}

//-----------------------------------------------------------------------------------------
// Blocks
//
//  primitiveValue0..9 are the primitives called in block value messages.
//  i is the number of arguments for the block.

void InterpreterGenerator::generate_primitiveValue( int i ) {
    GeneratedPrimitives::set_primitiveValue( i, _macroAssembler->pc() );
    _macroAssembler->movl( eax, Address( esp, ( i + 1 ) * oopSize ) ); // load recv (= block)
    _macroAssembler->jmp( _block_entry );
}


extern "C" int redo_send_offset;                  // offset when redoing send
extern "C" void verify_at_end_of_deoptimization();

const char * Interpreter::_restart_primitiveValue             = nullptr;
const char * Interpreter::_redo_bytecode_after_deoptimization = nullptr;
const char * Interpreter::_nlr_single_step_continuation_entry = nullptr;
Label      Interpreter::_nlr_single_step_continuation = Label();


void InterpreterGenerator::generate_forStubRoutines() {
    constexpr int invocation_counter_inc = 0x10000;

    Interpreter::_restart_primitiveValue = _macroAssembler->pc();
    _macroAssembler->enter();
    _macroAssembler->movl( ecx, Address( eax, BlockClosureOopDescriptor::context_byte_offset() ) );
    _macroAssembler->movl( edx, Address( eax, BlockClosureOopDescriptor::method_or_entry_byte_offset() ) );
    _macroAssembler->pushl( ecx );            // save recv (initialize with context)
    restore_ebx();            // if value... is called from compiled code
    _macroAssembler->addl( Address( edx, MethodOopDescriptor::counters_byte_offset() ), invocation_counter_inc );
    _macroAssembler->leal( esi, Address( edx, MethodOopDescriptor::codes_byte_offset() ) );
    _macroAssembler->movl( eax, ecx );            // initialize temp 1 with context
    _macroAssembler->pushl( esi );            // initialize esi save
    load_ebx();
    jump_ebx();

    Interpreter::_redo_bytecode_after_deoptimization = _macroAssembler->pc();

    // Call verify
    _macroAssembler->call_C( ( const char * ) verify_at_end_of_deoptimization, RelocationInformation::RelocationType::runtime_call_type );

    // Redo the send
    restore_esi();
    restore_ebx();
    _macroAssembler->movl( eax, Address( ( int ) &redo_send_offset, RelocationInformation::RelocationType::external_word_type ) );
    _macroAssembler->subl( esi, eax );
    load_ebx();
    _macroAssembler->popl( eax );          // get top of stack
    jump_ebx();

    Interpreter::_nlr_single_step_continuation_entry = _macroAssembler->pc();
    _macroAssembler->bind( Interpreter::_nlr_single_step_continuation );
    _macroAssembler->reset_last_Delta_frame();
    _macroAssembler->jmp( _nlr_testpoint );
}

//-----------------------------------------------------------------------------------------
// Method invocation
//
// call_method calls a methodOop. The registers have
// to be set up as follows:
//
// eax: receiver
// ebx: 000000xx
// ecx: methodOop
// parameters on the stack

void InterpreterGenerator::call_method() {

    // trace_send code should come here - fix this
    if ( TraceMessageSend ) {
        _macroAssembler->pushad();
        _macroAssembler->pushl( ecx );    // pass arguments (C calling convention)
        _macroAssembler->pushl( eax );
        _macroAssembler->call_C( ( const char * ) trace_send, RelocationInformation::RelocationType::runtime_call_type );
        _macroAssembler->addl( esp, oopSize * 2 );   // get rid of arguments
        _macroAssembler->popad();
    }

    save_esi();
    _macroAssembler->call( _method_entry );
    _macroAssembler->ic_info( _nlr_testpoint, 0 );
    restore_esi();
}


char * Interpreter::_last_native_called = nullptr;        // debugging only - see comment in header file

void InterpreterGenerator::call_native( Register entry ) {

    // trace_send code should come here - fix this
    if ( TraceMessageSend ) {
        _macroAssembler->pushad();
        _macroAssembler->pushl( eax );    // pass arguments (C calling convention)
        _macroAssembler->pushl( ecx );
        _macroAssembler->call_C( ( const char * ) trace_send, RelocationInformation::RelocationType::runtime_call_type );
        _macroAssembler->addl( esp, oopSize * 2 );   // get rid of arguments
        _macroAssembler->popad();
    }

    save_esi();
    _macroAssembler->movl( Address( int( &Interpreter::_last_native_called ), RelocationInformation::RelocationType::external_word_type ), entry );
    _macroAssembler->call( entry );
    _macroAssembler->ic_info( _nlr_testpoint, 0 );            // ordinary inline cache info
    restore_esi();
    restore_ebx();
}


extern "C" {

const char * method_entry_point = nullptr;   // for interpreter_asm.asm (remove if not used anymore)
const char * block_entry_point  = nullptr;   // for interpreter_asm.asm (remove if not used anymore)
const char * active_stack_limit();           // address of pointer to the current process' stack limit
void check_stack_overflow();                 //

}


void InterpreterGenerator::generate_method_entry_code() {

    // This generates the code sequence called to activate methodOop execution.
    // It is usually called via the call_method() macro, which saves the old
    // instruction counter (esi) and provides the ic_info word for NonLocalReturns.

    const int counter_offset = MethodOopDescriptor::counters_byte_offset();
    const int code_offset    = MethodOopDescriptor::codes_byte_offset();

    st_assert( not _method_entry.is_bound(), "code has been generated before" );
    Label start_setup, counter_overflow, start_execution, handle_counter_overflow, is_interpreted, handle_stack_overflow, continue_from_stack_overflow;

    // eax: receiver
    // ebx: 000000xx
    // ecx: methodOop
    // parameters on the stack
    method_entry_point = _macroAssembler->pc();
    _macroAssembler->bind( _method_entry );
    _macroAssembler->movl( edi, nil_addr() );

    // eax: receiver
    // ebx: 000000xx
    // ecx: methodOop
    // edi: initial value for temp0
    // parameters on the stack
    _macroAssembler->bind( start_setup );                                                   //
    _macroAssembler->enter();                                                               // setup new stack frame
    _macroAssembler->pushl( eax );                                                          // install receiver
    _macroAssembler->movl( edx, Address( ecx, counter_offset ) );                           // get method invocation counter
    _macroAssembler->leal( esi, Address( ecx, code_offset ) );                              // set bytecode pointer to first instruction
    _macroAssembler->addl( edx, 1 << MethodOopDescriptor::_invocation_count_offset );       // increment invocation counter (only upper word)
    _macroAssembler->pushl( esi );                                                          // initialize esi stack location for profiler
    _macroAssembler->movl( Address( ecx, counter_offset ), edx );                           // store method invocation counter
    load_ebx();                                                                             // get first byte code of method
    _macroAssembler->cmpl( edx, 0xFFFF << MethodOopDescriptor::_invocation_count_offset );  // make sure cmpl uses imm32 field
    Interpreter::_invocation_counter_addr = ( int * ) ( _macroAssembler->pc() - oopSize );  // compute invocation counter address
    _macroAssembler->jcc( Assembler::Condition::aboveEqual, counter_overflow );             // treat invocation counter overflow
    _macroAssembler->bind( start_execution );                                               // continuation point after overflow
    _macroAssembler->movl( eax, edi );                                                      // initialize temp0
    _macroAssembler->cmpl( esp, Address( int( active_stack_limit() ), RelocationInformation::RelocationType::external_word_type ) );
    _macroAssembler->jcc( Assembler::Condition::lessEqual, handle_stack_overflow );         //
    _macroAssembler->bind( continue_from_stack_overflow );                                  //
    jump_ebx();                                                                             // start execution

    // invocation counter overflow
    _macroAssembler->bind( counter_overflow );
    // not necessary to store esi since it has been just initialized
    _macroAssembler->pushl( edi );                                                          // move tos on stack (temp0, always here)
    _macroAssembler->set_last_Delta_frame_before_call();                                    //
    _macroAssembler->call( handle_counter_overflow );                                       // introduce extra frame to pass arguments
    _macroAssembler->reset_last_Delta_frame();                                              //
    _macroAssembler->popl( edi );                                                           // restore edi, used to initialize eax

    // Should check here if recompilation created a NativeMethod for this methodOop. If so, one should redo the send and thus start the NativeMethod.
    // If an NativeMethod has been created, invocation_counter_overflow returns the continuation pc, otherwise it returns nullptr.
    // For now: simply continue with interpreted version.
    restore_esi();
    restore_ebx();
    load_ebx();
    _macroAssembler->jmp( start_execution );

    // handle invocation counter overflow, use extra frame to pass arguments
    // eax: receiver
    // ecx: methodOop
    _macroAssembler->bind( handle_counter_overflow );
    _macroAssembler->pushl( ecx );                            // pass methodOop argument
    _macroAssembler->pushl( eax );                            // pass receiver argument
    _macroAssembler->call( ( const char * ) Recompilation::methodOop_invocation_counter_overflow, RelocationInformation::RelocationType::runtime_call_type ); // methodOop_invocation_counter_overflow(receiver, methodOop)
    _macroAssembler->addl( esp, 2 * oopSize );                        // discard arguments
    _macroAssembler->ret( 0 );


    // This generates the code sequence called to activate block execution.
    // It is jumped to from one of the primitiveValue primitives. eax is
    // expected to hold the receiver (i.e., the block closure).

    // eax: receiver (block closure)
    // primitiveValue parameters on the stack
    block_entry_point = _macroAssembler->pc();
    _macroAssembler->bind( _block_entry );
    _macroAssembler->movl( ecx, Address( eax, BlockClosureOopDescriptor::method_or_entry_byte_offset() ) );    // get methodOop/jump table entry out of closure
    _macroAssembler->reset_last_Delta_frame();                                                  // if called from the interpreter, the last Delta frame is setup
    _macroAssembler->test( ecx, MEMOOP_TAG ); // if methodOop then
    _macroAssembler->jcc( Assembler::Condition::notZero, is_interpreted ); // start methodOop execution
    _macroAssembler->jmp( ecx ); // else jump to jump table entry

    _macroAssembler->bind( is_interpreted );
    // eax: receiver (block closure)
    // ecx: block methodOop
    restore_ebx();                            // if value... is called from compiled code, ebx may be not zero
    _macroAssembler->movl( eax, Address( eax, BlockClosureOopDescriptor::context_byte_offset() ) );        // get context out of closure
    _macroAssembler->movl( edi, eax );                        // initial value for temp0 is (incoming) context/value
    // eax: context (= receiver)
    // ebx: 00000000
    // ecx: block methodOop
    // edi: context (= initial value for temp0)
    // parameters on stack
    _macroAssembler->jmp( start_setup );

    _macroAssembler->bind( handle_stack_overflow );
    _macroAssembler->pushl( eax );
    // _masm->call_C((char*)&check_stack_overflow, RelocationInformation::RelocationType::external_word_type);
    _macroAssembler->popl( eax );
    restore_esi();
    restore_ebx();
    load_ebx();
    _macroAssembler->jmp( continue_from_stack_overflow );
}


//-----------------------------------------------------------------------------------------
// Inline cache misses

void InterpreterGenerator::generate_inline_cache_miss_handler() {
    Label _normal_return;
    st_assert( not _inline_cache_miss.is_bound(), "code has been generated before" );
    _macroAssembler->bind( _inline_cache_miss );
    // We need an inline cache for NonLocalReturn evaluation.
    // This can happen because the inline cache miss may call "doesNotUnderstand:"
    call_C( ( const char * ) InterpretedInlineCache::inline_cache_miss );
    _macroAssembler->testl( eax, eax );
    _macroAssembler->jcc( Assembler::Condition::equal, _normal_return );
    _macroAssembler->movl( ecx, Address( eax ) ); // pop arguments
    _macroAssembler->movl( eax, Address( eax, oopSize ) ); // doesNotUnderstand: result
    _macroAssembler->addl( esp, ecx ); // pop arguments
    load_ebx();
    jump_ebx();
    _macroAssembler->bind( _normal_return );
    load_ebx();
    _macroAssembler->popl( eax );
    jump_ebx();
}


//-----------------------------------------------------------------------------------------
// smi_t predicted sends


void InterpreterGenerator::generate_predicted_smi_send_failure_handler() {
    st_assert( not _smi_send_failure.is_bound(), "code has been generated before" );
    const char * ep = normal_send( ByteCodes::Code::interpreted_send_1, true, false );
    // Note: Has to jump to normal_send entry point because the entry point is
    //       not necessarily in the beginning of the normal send code pattern.
    _macroAssembler->bind( _smi_send_failure );
    _macroAssembler->pushl( edx );                    // push receiver back on tos
    _macroAssembler->jmp( ep, RelocationInformation::RelocationType::runtime_call_type );
}


void InterpreterGenerator::check_smi_tags() {
    // tos: receiver
    // eax: argument
    _macroAssembler->popl( edx );           // get receiver
    _macroAssembler->movl( ecx, eax );      // copy it to ecx
    _macroAssembler->orl( ecx, edx );       // or tag bits
    _macroAssembler->test( ecx, MEMOOP_TAG );  // if one of them is set then
    _macroAssembler->jcc( Assembler::Condition::notZero, _smi_send_failure );    // arguments are not bot smis
    // edx: receiver
    // eax: argument
}


const char * InterpreterGenerator::smi_add() {
    Label overflow;
    const char * ep = entry_point();
    check_smi_tags();
    _macroAssembler->addl( eax, edx );
    _macroAssembler->jcc( Assembler::Condition::overflow, overflow );
    advance_aligned( 1 + 2 * oopSize );
    load_ebx();
    jump_ebx();

    _macroAssembler->bind( overflow );
    // eax: argument + receiver
    // edx: receiver
    _macroAssembler->subl( eax, edx );
    _macroAssembler->jmp( _smi_send_failure );
    return ep;
}


const char * InterpreterGenerator::smi_sub() {
    Label overflow;
    const char * ep = entry_point();
    check_smi_tags();
    _macroAssembler->subl( edx, eax );
    _macroAssembler->jcc( Assembler::Condition::overflow, overflow );
    advance_aligned( 1 + 2 * oopSize );
    _macroAssembler->movl( eax, edx );
    load_ebx();
    jump_ebx();

    _macroAssembler->bind( overflow );
    // eax: argument
    // edx: receiver - argument
    _macroAssembler->addl( edx, eax );
    _macroAssembler->jmp( _smi_send_failure );
    return ep;
}


const char * InterpreterGenerator::smi_mul() {
    Label overflow;
    const char * ep = entry_point();
    check_smi_tags();
    _macroAssembler->movl( ecx, eax );                // save argument for overflow case
    _macroAssembler->sarl( edx, TAG_SIZE );
    _macroAssembler->imull( eax, edx );
    _macroAssembler->jcc( Assembler::Condition::overflow, overflow );
    advance_aligned( 1 + 2 * oopSize );
    load_ebx();
    jump_ebx();

    _macroAssembler->bind( overflow );
    // eax: argument * (receiver >> TAG_SIZE)
    // ecx: argument
    // edx: receiver >> TAG_SIZE
    _macroAssembler->movl( eax, ecx );                // restore argument
    _macroAssembler->shll( edx, TAG_SIZE );                // undo shift
    _macroAssembler->jmp( _smi_send_failure );
    return ep;
}


const char * InterpreterGenerator::smi_compare_op( ByteCodes::Code code ) {
    Label is_true;
    const char * ep = entry_point();
    check_smi_tags();
    advance_aligned( 1 + 2 * oopSize );
    load_ebx();
    _macroAssembler->cmpl( edx, eax );
    Assembler::Condition cc;
    switch ( code ) {
        case ByteCodes::Code::smi_equal:
            cc = Assembler::Condition::equal;
            break;
        case ByteCodes::Code::smi_not_equal:
            cc = Assembler::Condition::notEqual;
            break;
        case ByteCodes::Code::smi_less:
            cc = Assembler::Condition::less;
            break;
        case ByteCodes::Code::smi_less_equal:
            cc = Assembler::Condition::lessEqual;
            break;
        case ByteCodes::Code::smi_greater:
            cc = Assembler::Condition::greater;
            break;
        case ByteCodes::Code::smi_greater_equal:
            cc = Assembler::Condition::greaterEqual;
            break;
        default: ShouldNotReachHere();
    }
    _macroAssembler->jcc( cc, is_true );
    _macroAssembler->movl( eax, false_addr() );
    jump_ebx();

    _macroAssembler->bind( is_true );
    _macroAssembler->movl( eax, true_addr() );
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::smi_logical_op( ByteCodes::Code code ) {
    const char * ep = entry_point();
    check_smi_tags();
    advance_aligned( 1 + 2 * oopSize );
    load_ebx();
    switch ( code ) {
        case ByteCodes::Code::smi_and:
            _macroAssembler->andl( eax, edx );
            break;
        case ByteCodes::Code::smi_or:
            _macroAssembler->orl( eax, edx );
            break;
        case ByteCodes::Code::smi_xor:
            _macroAssembler->xorl( eax, edx );
            break;
        default: ShouldNotReachHere();
    }
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::smi_shift() {
    // overflow is ignored for now (as in smi_prims.cpp)
    st_assert( INTEGER_TAG == 0, "check this code" );
    Label shift_right;

    const char * ep = entry_point();
    check_smi_tags();
    advance_aligned( 1 + 2 * oopSize );
    load_ebx();
    _macroAssembler->sarl( eax, TAG_SIZE );                // convert argument (shift count) into int (sets zero flag)
    _macroAssembler->movl( ecx, eax );                // move shift count into CL
    _macroAssembler->jcc( Assembler::Condition::negative, shift_right );        // shift right or shift left?

    // shift left
    _macroAssembler->shll( edx );                    // else receiver << (argument mod 32)
    _macroAssembler->movl( eax, edx );                // set result
    jump_ebx();

    // shift right
    _macroAssembler->bind( shift_right );
    _macroAssembler->negl( ecx );
    _macroAssembler->sarl( edx );                    // receiver >> (argument mod 32)
    _macroAssembler->andl( edx, -1 << TAG_SIZE );            // clear tag bits
    _macroAssembler->movl( eax, edx );                // set result
    jump_ebx();
    return ep;
}


//-----------------------------------------------------------------------------------------
// objArray predicted sends
//
// Problem: Implementation requires InterpretedICIterator to be adjusted: in case of
//          special predicted primitive methods, the (non-cached) receiver is not simply
//          a smi_t but can be an object array (or something else) depending on the primitive
//          that is predicted. Think about this.

const char * InterpreterGenerator::objArray_size() {
    const char * ep = entry_point();
    Unimplemented();
    return ep;
}


const char * InterpreterGenerator::objArray_at() {
    const char * ep = entry_point();
    Unimplemented();
    return ep;
}


const char * InterpreterGenerator::objArray_at_put() {
    const char * ep = entry_point();
    Unimplemented();
    return ep;
}


//-----------------------------------------------------------------------------------------
// Returns
//
// _return_tos pops the arguments and returns from a method or block.

void InterpreterGenerator::return_tos( ByteCodes::ArgumentSpec arg_spec ) {
    _macroAssembler->leave();
    switch ( arg_spec ) {
        case ByteCodes::ArgumentSpec::recv_0_args:
            _macroAssembler->ret( 0 * oopSize );
            break;
        case ByteCodes::ArgumentSpec::recv_1_args:
            _macroAssembler->ret( 1 * oopSize );
            break;
        case ByteCodes::ArgumentSpec::recv_2_args:
            _macroAssembler->ret( 2 * oopSize );
            break;
        case ByteCodes::ArgumentSpec::recv_n_args: {
            // no. of arguments is in the next byte
            _macroAssembler->movb( ebx, Address( esi, 1 ) );            // get no. of arguments
            _macroAssembler->popl( ecx );                        // get return address
            _macroAssembler->leal( esp, Address( esp, ebx, Address::ScaleFactor::times_4 ) );    // adjust esp (remove arguments)
            _macroAssembler->jmp( ecx );                        // return
            break;
        }
        default: ShouldNotReachHere();
    }
}

//-----------------------------------------------------------------------------------------
// Zap context
//
// The context is zapped by storing 0 into the home field of the context.
// Only method contexts must be zapped.

void InterpreterGenerator::zap_context() {
    _macroAssembler->pushl( eax );            // make sure temp0 (context) is in memory
    _macroAssembler->movl( ecx, context_addr() );
    _macroAssembler->popl( eax );
    _macroAssembler->movl( Address( ecx, ContextOopDescriptor::parent_byte_offset() ), 0 );
}

//-----------------------------------------------------------------------------------------
// Local returns
//

const char * InterpreterGenerator::local_return( bool_t push_self, int nofArgs, bool_t zap ) {
    const char * ep = entry_point();
    if ( zap ) {
        zap_context();
    }
    if ( push_self ) {
        _macroAssembler->movl( eax, self_addr() );
    }

    // return bytecodes aren't sends, so they don't have any ArgumentSpec, yet
    // return_tos takes one as argument ... hence this weird device		-Marc 4/07
    ByteCodes::ArgumentSpec arg_spec;
    switch ( nofArgs ) {
        case 0:
            arg_spec = ByteCodes::ArgumentSpec::recv_0_args;
            break;
        case 1:
            arg_spec = ByteCodes::ArgumentSpec::recv_1_args;
            break;
        case 2:
            arg_spec = ByteCodes::ArgumentSpec::recv_2_args;
            break;
        case -1:
            arg_spec = ByteCodes::ArgumentSpec::recv_n_args;
            break;
        default: ShouldNotReachHere();
    }

    return_tos( arg_spec );
    return ep;
}

//-----------------------------------------------------------------------------------------
// Error handling
//
// Entry points for run-time errors. eax must contain the value it had
// before starting to execute the instruction that issued the error,
// esi must point to the next instruction.

extern "C" void suspend_on_error( InterpreterErrorConstants error_code );

const char * Interpreter::_illegal = nullptr;


void InterpreterGenerator::generate_error_handler_code() {
    st_assert( not _boolean_expected.is_bound(), "code has been generated before" );

    Label suspend, call_suspend;
    // eax: top of expression stack
    // ecx: error code
    // esi: points to next instruction
    _macroAssembler->bind( suspend );
    _macroAssembler->pushl( eax );                // save tos
    call_C( call_suspend );
    should_not_reach_here();

    _macroAssembler->bind( call_suspend );            // extra stack frame to pass error code in C land
    _macroAssembler->pushl( ecx );                // pass error code
    _macroAssembler->call( ( const char * ) suspend_on_error, RelocationInformation::RelocationType::runtime_call_type );
    should_not_reach_here();

    _macroAssembler->bind( _boolean_expected );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::boolean_expected ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _float_expected );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::float_expected ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _NonLocalReturn_to_dead_frame );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::nonlocal_return_error ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _halted );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::halted ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _stack_missaligned );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::stack_missaligned ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _ebx_wrong );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::ebx_wrong ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _obj_wrong );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::obj_wrong ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _last_Delta_fp_wrong );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::last_Delta_fp_wrong ) );
    _macroAssembler->jmp( suspend );

    _macroAssembler->bind( _primitive_result_wrong );
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::primitive_result_wrong ) );
    _macroAssembler->jmp( suspend );

    _illegal = _macroAssembler->pc();
    _macroAssembler->movl( ecx, static_cast<int>( InterpreterErrorConstants::illegal_code ) );
    _macroAssembler->jmp( suspend );

    Interpreter::_illegal = _illegal;
}


//-----------------------------------------------------------------------------------------
// Non-local returns
//
// nonlocal_return does a non-local return starting in the current activation frame
// and returning through stack frames until it reaches the method activation frame
// that corresponds to the home stack frame of the (heap) context of the current
// activation.
//
// The heap contexts of all method frames (that have a heap context)
// encountered inbetween are zapped (incl. the context of the home stack frame).

// After determining the home stack frame via a loop, the NonLocalReturn sequence is issued
// as in compiled code.
//
// Note: Non-local returns that start in an interpreted block must end in an interpreted
// method; i.e. even if the NonLocalReturn goes through compiled (C) frames, it will eventually end
// in an interpreter frame.
//
// Furthermore, if a NonLocalReturn is possible, there is always a chain of
// contexts that eventually will end with the context allocated for the NonLocalReturn target method
// (since NonLocalReturn require contexts to be there always).
//
//
// Registers used in compiled code to do NonLocalReturns (defined in Location.hpp):
//
// Note: When doing NonLocalReturns in compiled code, the nlr_home_id is used to identify the home
// scope in case of inlined methods. When doing NonLocalReturn starting in interpreted code and
// therefore ending in interpreted code, this register is used to pass the the number of
// arguments to pop because this information is needed to correctly finish the NonLocalReturn. In
// order to be able to distinguish between compiled and interpreted NonLocalReturns, this value is
// made negative (compiled NonLocalReturn home ids are always >= 0).
//
//

extern "C" {
const char * nlr_testpoint_entry = nullptr;    // for interpreter_asm.asm (remove if not used anymore)
extern ContextOop nlr_home_context;
}


void InterpreterGenerator::generate_nonlocal_return_code() {

    st_assert( eax == NonLocalReturn_result_reg, "NonLocalReturn register use changed" );
    st_assert( edi == NonLocalReturn_home_reg, "NonLocalReturn register use changed" );
    st_assert( esi == NonLocalReturn_homeId_reg, "NonLocalReturn register use changed" );

    st_assert( not _issue_NonLocalReturn.is_bound(), "code has been generated before" );
    st_assert( not _nlr_testpoint.is_bound(), "code has been generated before" );

    Label zapped_context, loop, no_zapping, compiled_code_NonLocalReturn;

    // context already zapped
    _macroAssembler->bind( zapped_context );
    _macroAssembler->popl( eax );                      // get NonLocalReturn result back
    _macroAssembler->addl( esi, 2 );            // adjust esi (must point to next instruction)
    _macroAssembler->jmp( _NonLocalReturn_to_dead_frame );

    _macroAssembler->bind( _issue_NonLocalReturn );
    _macroAssembler->pushl( eax );                     // make sure context (temp0) is in memory
    _macroAssembler->movl( edi, context_addr() );  // get context
    if ( _debug ) {
        // should check here if edx is really a context
    }

    // find home stack frame by following the context chain
    // edi: current context in chain
    _macroAssembler->bind( loop );                // repeat
    _macroAssembler->movl( eax, edi );            //   eax: = last context used
    _macroAssembler->movl( edi, Address( edi, ContextOopDescriptor::parent_byte_offset() ) );
    _macroAssembler->test( edi, MEMOOP_TAG );            //   edi: = edi.home
    _macroAssembler->jcc( Assembler::Condition::notZero, loop );        // until is_smi(edi)
    _macroAssembler->testl( edi, edi );            // if edi = 0 then
    _macroAssembler->jcc( Assembler::Condition::zero, zapped_context );    //   context has been zapped
    _macroAssembler->movl( Address( int( &nlr_home_context ), RelocationInformation::RelocationType::external_word_type ), eax );
    // else save the context containing the home (edi points to home stack frame)
    _macroAssembler->movb( ebx, Address( esi, 1 ) );        // get no. of arguments to pop
    _macroAssembler->popl( eax );                // get NonLocalReturn result back
    _macroAssembler->movl( esi, ebx );            // keep no. of arguments in esi
    _macroAssembler->notl( esi );                // make negative to distinguish from compiled NonLocalReturns

    // entry point for all methods to do NonLocalReturn test & continuation,
    // first check if context zap is necessary
    //
    // eax: NonLocalReturn result
    // edi: NonLocalReturn home
    // esi: no. of arguments to pop (1s complement)

    _macroAssembler->bind( _C_nlr_testpoint );
    _macroAssembler->reset_last_Delta_frame();
    nlr_testpoint_entry = _macroAssembler->pc();
    _macroAssembler->bind( _nlr_testpoint );

    // check if esi is indeed negative, otherwise this would be a NonLocalReturn with
    // target in compiled code, which would be a bug - leave it in here to
    // find deoptimization bugs (request from Lars)
    //_masm->testl(esi, esi);
    //_masm->jcc(Assembler::positive, compiled_code_NonLocalReturn);

    _macroAssembler->movl( ecx, context_addr() );        // get potential context
    _macroAssembler->test( ecx, MEMOOP_TAG );            // if is_smi(ecx) then
    _macroAssembler->jcc( Assembler::Condition::zero, no_zapping );    //   can't be a context pointer
    _macroAssembler->movl( edx, Address( ecx, MemOopDescriptor::klass_byte_offset() ) );    // else isOop: get its class
    _macroAssembler->cmpl( edx, contextKlass_addr() );    // if class # ContextKlass then
    _macroAssembler->jcc( Assembler::Condition::notEqual, no_zapping );    //   is not a context
    _macroAssembler->movl( ebx, Address( ecx, ContextOopDescriptor::parent_byte_offset() ) );    // else is context: get home
    _macroAssembler->cmpl( ebx, ebp );            // if home # ebp then
    _macroAssembler->jcc( Assembler::Condition::notEqual, no_zapping );    //   is not a methoc context
    _macroAssembler->movl( Address( ecx, ContextOopDescriptor::parent_byte_offset() ), 0 );    // else method context: zap home

    _macroAssembler->bind( no_zapping );
    _macroAssembler->cmpl( edi, ebp );
    _macroAssembler->jcc( Assembler::Condition::notEqual, StubRoutines::continue_NonLocalReturn_entry() );

    // home found
    // eax: NonLocalReturn result
    // edi: NonLocalReturn home
    // esi: no. of arguments to pop (1s complement)
    restore_ebx();                // make sure ebx = 0
    _macroAssembler->leave();                // remove stack frame
    _macroAssembler->popl( ecx );                // get return address
    _macroAssembler->notl( esi );                // make positive again
    _macroAssembler->leal( esp, Address( esp, esi, Address::ScaleFactor::times_4 ) );    // pop arguments
    _macroAssembler->jmp( ecx );                // return

    // error handler for compiled code NonLocalReturns - can be removed as soon as that test has been removed.
    // For now just use magic imm32 to indicate this problem.
    //_masm->bind(compiled_code_NonLocalReturn);
    //_masm->hlt();
    //_masm->testl(eax, 0x0badcafe);
    //_masm->testl(eax, 0x0badcafe);
    //_masm->testl(eax, 0x0badcafe);
    //_masm->testl(eax, 0x0badcafe);
}


const char * InterpreterGenerator::nonlocal_return_tos() {
    const char * ep = entry_point();
    _macroAssembler->jmp( _issue_NonLocalReturn );
    return ep;
}


const char * InterpreterGenerator::nonlocal_return_self() {
    const char * ep = entry_point();
    _macroAssembler->pushl( eax );
    _macroAssembler->movl( eax, self_addr() );
    _macroAssembler->jmp( _issue_NonLocalReturn );
    return ep;
}

//-----------------------------------------------------------------------------------------
// Normal sends, access methods
//
// HCode structure:
//
// 	[send		]	(1 byte)
// 	[method		]	(1 dword)
//	[icache (class)	]	(1 dword)
//	...	  	  <---	esi
//
// Access methods cannot exist for the smi_t class which simplifies
// the inline cache test: if the receiver is a smi_t it is always
// a cache miss because the cache never caches the smi_t class.
//
// Note: Currently _load_recv is used to get the receiver. This
// is suboptimal since the receiver is also pushed on the stack
// and has to be popped again at the end (except for self sends).
// Should change this at some point (optimization).
//

const char * InterpreterGenerator::access_send( bool_t self ) {

    const char * ep                     = entry_point();

    ByteCodes::ArgumentSpec arg_spec;
    Address                 method_addr = Address( esi, -2 * oopSize );
    Address                 klass_addr  = Address( esi, -1 * oopSize );

    if ( self ) {
        arg_spec = ByteCodes::ArgumentSpec::args_only;
    } else {
        arg_spec = ByteCodes::ArgumentSpec::recv_0_args;
    }

    load_recv( arg_spec );
    advance_aligned( 1 + 2 * oopSize );

    // mov ecx, [method]		        ; get method
    // lea edx, [ecx._hcodes(4)]    	; start address of hcode + 4
    // and edx, NOT 3			        ; align
    // mov edx, [edx]
    // mov eax, [eax + edx - MEMOOP_TAG]	; load instVar at offset

    _macroAssembler->test( eax, MEMOOP_TAG );                // check if smi_t
    _macroAssembler->movl( ecx, method_addr );                // get cached method (assuming infrequent cache misses)
    _macroAssembler->movl( edx, klass_addr );                // get cached klass
    _macroAssembler->jcc( Assembler::Condition::zero, _inline_cache_miss );    // if smi_t then it's a cache miss
    _macroAssembler->movl( edi, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // get recv class

    // eax: receiver
    // ebx: 000000xx (load_recv may modify bl)
    // ecx: cached methodOop
    // edx: cached klass
    // edi: receiver klass
    // esi: next instruction
    _macroAssembler->cmpl( edx, edi );            // compare with inline cache
    _macroAssembler->jcc( Assembler::Condition::notEqual, _inline_cache_miss );

    Address primitive_addr = Address( ecx, MethodOopDescriptor::codes_byte_offset() + oopSize );
    _macroAssembler->movl( edx, primitive_addr );        // get instVar offset
    _macroAssembler->movl( eax, field_addr( eax, edx ) );

    if ( not self ) {
        _macroAssembler->popl( ecx );                //    receiver still on stack: remove it
    }
    load_ebx();
    jump_ebx();

    return ep;
}


//-----------------------------------------------------------------------------------------
// Inline cache structure for non-polymorphic sends
//
// [send byte code ]      1 byte
// [no. of args    ]      1 byte, only if arg_spec == recv_n_args
// alignment .......      0..3 bytes
// [selector/method]      1 word
// [0/class        ]      1 word
// next instr ...... <--- esi, after advance
//
// normal_send generates the code for normal sends that can
// deal with either methodOop or NativeMethod entries, or both.
//
// Note: As of now (7/30/96) the method sweeper is running asynchronously and might modify
//       (cleanup) the inline cache while a send is in progress. Thus, the inline cache might
//       have changed in the meantime which may cause problems.
//       Right now we try to minimize the chance for this to happen by loading the cached method as soon as possible, thereby reducing the time frame for the sweeper (gri).

const char * InterpreterGenerator::normal_send( ByteCodes::Code code, bool_t allow_methodOop, bool_t allow_nativeMethod, bool_t primitive_send ) {
    st_assert( allow_methodOop or allow_nativeMethod or primitive_send, "must allow at least one method representation" );

    Label is_smi, compare_class, is_methodOop, is_nativeMethod;

    ByteCodes::ArgumentSpec arg_spec = ByteCodes::argument_spec( code );
    bool_t                  pop_tos  = ByteCodes::pop_tos( code );

    // inline cache layout
    int     length      = ( arg_spec == ByteCodes::ArgumentSpec::recv_n_args ? 2 : 1 ) + 2 * oopSize;
    Address method_addr = Address( esi, -2 * oopSize );
    Address klass_addr  = Address( esi, -1 * oopSize );

    _macroAssembler->bind( is_smi );                // smi_t case (assumed to be infrequent)
    _macroAssembler->movl( edi, smiKlass_addr() );        // load smi_t klass
    _macroAssembler->jmp( compare_class );

    const char * ep = entry_point();
    load_recv( arg_spec );
    advance_aligned( length );
    _macroAssembler->test( eax, MEMOOP_TAG );            // check if smi_t
    _macroAssembler->movl( ecx, method_addr );            // get cached method (assuming infrequent cache misses)
    _macroAssembler->movl( edx, klass_addr );            // get cached klass
    _macroAssembler->jcc( Assembler::Condition::zero, is_smi );
    _macroAssembler->movl( edi, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // get recv class

    _macroAssembler->bind( compare_class );
    // eax: receiver
    // ebx: 000000xx (load_recv may modify bl)
    // ecx: cached selector/methodOop/NativeMethod
    // edx: cached klass
    // edi: receiver klass
    // esi: next instruction
    _macroAssembler->cmpl( edx, edi );            // compare with inline cache
    _macroAssembler->jcc( Assembler::Condition::notEqual, _inline_cache_miss );

    if ( allow_methodOop and allow_nativeMethod ) {
        // make case distinction at run-time
        _macroAssembler->test( ecx, MEMOOP_TAG );            // check if NativeMethod
        _macroAssembler->jcc( Assembler::Condition::zero, is_nativeMethod );    // nativeMethods (jump table entries) are 4-byte alligned
    }

    if ( allow_methodOop ) {
        _macroAssembler->bind( is_methodOop );
        call_method();
        if ( arg_spec not_eq ByteCodes::ArgumentSpec::args_only )
            _macroAssembler->popl( ecx );// discard receiver if on stack
        load_ebx();
        if ( pop_tos )
            _macroAssembler->popl( eax );        // discard result if not used
        jump_ebx();
    }

    if ( allow_nativeMethod ) {
        _macroAssembler->bind( is_nativeMethod );
        call_native( ecx );
        if ( arg_spec not_eq ByteCodes::ArgumentSpec::args_only )
            _macroAssembler->popl( ecx );// discard receiver if on stack
        load_ebx();
        if ( pop_tos )
            _macroAssembler->popl( eax );        // discard result if not used
        jump_ebx();
    }

    if ( primitive_send ) {
        Label   _failed;
        Address primitive_addr = Address( ecx, MethodOopDescriptor::codes_byte_offset() + oopSize );

        _macroAssembler->movl( edx, primitive_addr );        // get primitive address
        call_C( edx );                // eax: = primitive call
        _macroAssembler->test( eax, MARK_TAG_BIT );
        _macroAssembler->jcc( Assembler::Condition::notZero, _failed );
        load_ebx();
        if ( pop_tos )
            _macroAssembler->popl( eax );        // discard result if not used
        jump_ebx();
        _macroAssembler->bind( _failed );
        //_print 'predicted primitive failed - not yet implemented', 0, 0
        _macroAssembler->hlt();
    }

    return ep;
}


const char * InterpreterGenerator::primitive_send( ByteCodes::Code code ) {
    return normal_send( code, false, false, true );
}


const char * InterpreterGenerator::interpreted_send( ByteCodes::Code code ) {
    return normal_send( code, true, false );
}


const char * InterpreterGenerator::compiled_send( ByteCodes::Code code ) {
    return normal_send( code, false, true );
}


const char * InterpreterGenerator::megamorphic_send( ByteCodes::Code code ) {
    // Handle super sends conventionally - most probably infrequent anyway
    if ( ByteCodes::is_super_send( code ) )
        return normal_send( code, true, true );

    Label                   is_smi, probe_primary_cache, is_methodOop, is_nativeMethod, probe_secondary_cache;
    ByteCodes::ArgumentSpec arg_spec = ByteCodes::argument_spec( code );

    // inline cache layout
    int     length        = ( arg_spec == ByteCodes::ArgumentSpec::recv_n_args ? 2 : 1 ) + 2 * oopSize;
    bool_t  pop_tos       = ByteCodes::pop_tos( code );
    Address selector_addr = Address( esi, -2 * oopSize );
    Address klass_addr    = Address( esi, -1 * oopSize );

    _macroAssembler->bind( is_smi );                // smi_t case (assumed to be infrequent)
    _macroAssembler->movl( ecx, smiKlass_addr() );        // load smi_t klass
    _macroAssembler->jmp( probe_primary_cache );

    const char * ep = entry_point();

    load_recv( arg_spec );
    advance_aligned( length );
    _macroAssembler->test( eax, MEMOOP_TAG );            // check if smi_t
    _macroAssembler->jcc( Assembler::Condition::zero, is_smi );        // otherwise
    _macroAssembler->movl( ecx, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // get recv class

    // probe primary cache
    //
    // eax: receiver
    // ebx: 000000xx
    // ecx: receiver klass
    // esi: next instruction
    _macroAssembler->bind( probe_primary_cache );        // compute hash value
    _macroAssembler->movl( edx, selector_addr );        // get selector
    // compute hash value
    _macroAssembler->movl( edi, ecx );
    _macroAssembler->xorl( edi, edx );
    _macroAssembler->andl( edi, ( primary_cache_size - 1 ) << 4 );
    // probe cache
    _macroAssembler->cmpl( ecx, Address( edi, LookupCache::primary_cache_address() + 0 * oopSize ) );
    _macroAssembler->jcc( Assembler::Condition::notEqual, probe_secondary_cache );
    _macroAssembler->cmpl( edx, Address( edi, LookupCache::primary_cache_address() + 1 * oopSize ) );
    _macroAssembler->jcc( Assembler::Condition::notEqual, probe_secondary_cache );
    _macroAssembler->movl( ecx, Address( edi, LookupCache::primary_cache_address() + 2 * oopSize ) );
    _macroAssembler->test( ecx, MEMOOP_TAG );            // check if NativeMethod
    _macroAssembler->jcc( Assembler::Condition::zero, is_nativeMethod );    // nativeMethods (jump table entries) are 4-byte aligned

    _macroAssembler->bind( is_methodOop );
    call_method();
    if ( arg_spec not_eq ByteCodes::ArgumentSpec::args_only )
        _macroAssembler->popl( ecx );// discard receiver if on stack
    load_ebx();
    if ( pop_tos )
        _macroAssembler->popl( eax );        // discard result if not used
    jump_ebx();

    _macroAssembler->bind( is_nativeMethod );
    call_native( ecx );
    if ( arg_spec not_eq ByteCodes::ArgumentSpec::args_only )
        _macroAssembler->popl( ecx );// discard receiver if on stack
    load_ebx();
    if ( pop_tos )
        _macroAssembler->popl( eax );        // discard result if not used
    jump_ebx();

    // probe secondary cache
    //
    // eax: receiver
    // ebx: 000000xx
    // ecx: receiver klass
    // edx: selector
    // edi: primary cache index
    // esi: next instruction
    _macroAssembler->bind( probe_secondary_cache );
    _macroAssembler->andl( edi, ( secondary_cache_size - 1 ) << 4 );
    // probe cache
    _macroAssembler->cmpl( ecx, Address( edi, LookupCache::secondary_cache_address() + 0 * oopSize ) );
    _macroAssembler->jcc( Assembler::Condition::notEqual, _inline_cache_miss );
    _macroAssembler->cmpl( edx, Address( edi, LookupCache::secondary_cache_address() + 1 * oopSize ) );
    _macroAssembler->jcc( Assembler::Condition::notEqual, _inline_cache_miss );
    _macroAssembler->movl( ecx, Address( edi, LookupCache::secondary_cache_address() + 2 * oopSize ) );
    _macroAssembler->test( ecx, MEMOOP_TAG );            // check if NativeMethod
    _macroAssembler->jcc( Assembler::Condition::zero, is_nativeMethod );    // nativeMethods (jump table entries) are 4-byte aligned
    _macroAssembler->jmp( is_methodOop );

    return ep;
}

//-----------------------------------------------------------------------------------------
// Inline cache structure for polymorphic sends
//
// [send byte code ]      1 byte
// [no. of args    ]      1 byte, only if arg_spec == recv_n_args
// alignment .......      0..3 bytes
// [selector       ]      1 word
// [pic            ]      1 word
// next instr ...... <--- esi, after advance
//
// normal_send generates the code for normal sends that can
// deal with either methodOop or NativeMethod entries, or both.

const char * InterpreterGenerator::polymorphic_send( ByteCodes::Code code ) {
    Label loop, found, is_nativeMethod;

    ByteCodes::ArgumentSpec arg_spec = ByteCodes::argument_spec( code );
    bool_t                  pop_tos  = ByteCodes::pop_tos( code );

    // inline cache layout
    int     length        = ( arg_spec == ByteCodes::ArgumentSpec::recv_n_args ? 2 : 1 ) + 2 * oopSize;
    Address selector_addr = Address( esi, -2 * oopSize );
    Address pic_addr      = Address( esi, -1 * oopSize );

    // pic layout
    int length_offset = 2 * oopSize - MEMOOP_TAG;    // these constants should be mapped to the objectArrayOop definition
    int data_offset   = 3 * oopSize - MEMOOP_TAG;    // these constants should be mapped to the objectArrayOop definition

    const char * ep = entry_point();
    load_recv( arg_spec );
    advance_aligned( length );
    _macroAssembler->movl( ebx, pic_addr );            // get pic
    _macroAssembler->movl( ecx, Address( ebx, length_offset ) );// get pic length (smi_t)
    _macroAssembler->sarl( ecx, TAG_SIZE + 1 );        // get pic length (int)
    // verifyPIC here

    _macroAssembler->movl( edx, smiKlass_addr() );        // preload smi_t klass
    _macroAssembler->testl( eax, MEMOOP_TAG );            // check if smi_t
    _macroAssembler->jcc( Assembler::Condition::zero, loop );        // otherwise
    _macroAssembler->movl( edx, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // get receiver klass

    // search pic for appropriate entry
    _macroAssembler->bind( loop );
    // eax: receiver
    // ebx: pic (objArrayOop)
    // ecx: counter
    // edx: receiver class
    // esi: next instruction
    _macroAssembler->cmpl( edx, Address( ebx, ecx, Address::ScaleFactor::times_8, data_offset - 1 * oopSize, RelocationInformation::RelocationType::none ) );
    _macroAssembler->jcc( Assembler::Condition::equal, found );
    _macroAssembler->decl( ecx );
    _macroAssembler->jcc( Assembler::Condition::notZero, loop );

    // cache miss
    _macroAssembler->jmp( _inline_cache_miss );

    _macroAssembler->bind( found );
    // eax: receiver
    // ebx: pic (objArrayOop)
    // ecx: counter (> 0)
    // edx: receiver class
    // esi: next instruction
    _macroAssembler->movl( ecx, Address( ebx, ecx, Address::ScaleFactor::times_8, data_offset - 2 * oopSize, RelocationInformation::RelocationType::none ) );
    _macroAssembler->testl( ecx, MEMOOP_TAG );
    _macroAssembler->jcc( Assembler::Condition::zero, is_nativeMethod );
    restore_ebx();

    // methodOop found
    call_method();
    if ( arg_spec not_eq ByteCodes::ArgumentSpec::args_only )
        _macroAssembler->popl( ecx );    // discard receiver if on stack
    load_ebx();
    if ( pop_tos )
        _macroAssembler->popl( eax );                // discard result if not used
    jump_ebx();

    // NativeMethod found
    _macroAssembler->bind( is_nativeMethod );
    call_native( ecx );
    if ( arg_spec not_eq ByteCodes::ArgumentSpec::args_only )
        _macroAssembler->popl( ecx );    // discard receiver if on stack
    load_ebx();
    if ( pop_tos )
        _macroAssembler->popl( eax );                // discard result if not used
    jump_ebx();

    return ep;
}


//-----------------------------------------------------------------------------------------
// Miscellaneous

const char * InterpreterGenerator::special_primitive_send_hint() {
    const char * ep = entry_point();
    st_assert( ByteCodes::format( ByteCodes::Code::special_primitive_send_1_hint ) == ByteCodes::Format::BB, "unexpected format" );
    _macroAssembler->addl( esi, 2 );                // simply skip this instruction
    load_ebx();
    jump_ebx();
    return ep;
}


const char * InterpreterGenerator::compare( bool_t equal ) {
    Assembler::Condition cond = equal ? Assembler::Condition::equal : Assembler::Condition::notEqual;

    Label _return_true;

    const char * ep = entry_point();
    next_ebx();
    _macroAssembler->popl( edx );            // get receiver
    _macroAssembler->cmpl( eax, edx );            // compare with argument
    _macroAssembler->jcc( cond, _return_true );

    _macroAssembler->movl( eax, false_addr() );
    jump_ebx();

    _macroAssembler->bind( _return_true );
    _macroAssembler->movl( eax, true_addr() );
    jump_ebx();

    return ep;
}


const char * InterpreterGenerator::halt() {
    const char * ep = entry_point();
    _macroAssembler->incl( esi );                // advance to next instruction
    _macroAssembler->jmp( _halted );
    return ep;
}

//-----------------------------------------------------------------------------------------
// Instruction generator

const char * InterpreterGenerator::generate_instruction( ByteCodes::Code code ) {

    // constants for readability
    const bool_t  pop           = true;
    const bool_t  returns_float = true;
    const bool_t  push          = false;
    const bool_t  store_pop     = true;
    constexpr int n             = -1;

    switch ( code ) {

        case ByteCodes::Code::push_temp_0:
            return push_temp( 0 );
        case ByteCodes::Code::push_temp_1:
            return push_temp( 1 );
        case ByteCodes::Code::push_temp_2:
            return push_temp( 2 );
        case ByteCodes::Code::push_temp_3:
            return push_temp( 3 );
        case ByteCodes::Code::push_temp_4:
            return push_temp( 4 );
        case ByteCodes::Code::push_temp_5:
            return push_temp( 5 );
        case ByteCodes::Code::push_temp_n:
            return push_temp_n();

        case ByteCodes::Code::store_temp_0_pop:
            return store_temp( 0, pop );
        case ByteCodes::Code::store_temp_1_pop:
            return store_temp( 1, pop );
        case ByteCodes::Code::store_temp_2_pop:
            return store_temp( 2, pop );
        case ByteCodes::Code::store_temp_3_pop:
            return store_temp( 3, pop );
        case ByteCodes::Code::store_temp_4_pop:
            return store_temp( 4, pop );
        case ByteCodes::Code::store_temp_5_pop:
            return store_temp( 5, pop );
        case ByteCodes::Code::store_temp_n_pop:
            return store_temp_n( pop );
        case ByteCodes::Code::store_temp_n:
            return store_temp_n();

            // arguments
        case ByteCodes::Code::push_arg_1:
            return push_arg( 1 );
        case ByteCodes::Code::push_arg_2:
            return push_arg( 2 );
        case ByteCodes::Code::push_arg_3:
            return push_arg( 3 );
        case ByteCodes::Code::push_arg_n:
            return push_arg_n();

            // Space allocation
        case ByteCodes::Code::allocate_temp_1:
            return allocate_temps( 1 );
        case ByteCodes::Code::allocate_temp_2:
            return allocate_temps( 2 );
        case ByteCodes::Code::allocate_temp_3:
            return allocate_temps( 3 );
        case ByteCodes::Code::allocate_temp_n:
            return allocate_temps_n();

            // literals / expressions
        case ByteCodes::Code::push_neg_n:
            return push_smi( true );
        case ByteCodes::Code::push_succ_n:
            return push_smi( false );
        case ByteCodes::Code::push_literal:
            return push_literal();
        case ByteCodes::Code::push_tos:
            return push_tos();
        case ByteCodes::Code::push_self:
            return push_self();
        case ByteCodes::Code::push_nil:
            return push_const( nil_addr() );
        case ByteCodes::Code::push_true:
            return push_const( true_addr() );
        case ByteCodes::Code::push_false:
            return push_const( false_addr() );
        case ByteCodes::Code::only_pop:
            return only_pop();

            // instance variables
        case ByteCodes::Code::return_instVar:
            return return_instVar();
        case ByteCodes::Code::push_instVar:
            return push_instVar();
        case ByteCodes::Code::store_instVar_pop:
            return store_instVar( pop );
        case ByteCodes::Code::store_instVar:
            return store_instVar();

            // context temporaries
        case ByteCodes::Code::push_temp_0_context_0:
            return with_context_temp( push, 0, 0 );
        case ByteCodes::Code::push_temp_1_context_0:
            return with_context_temp( push, 1, 0 );
        case ByteCodes::Code::push_temp_2_context_0:
            return with_context_temp( push, 2, 0 );
        case ByteCodes::Code::push_temp_n_context_0:
            return with_context_temp( push, n, 0 );

        case ByteCodes::Code::push_temp_0_context_1:
            return with_context_temp( push, 0, 1 );
        case ByteCodes::Code::push_temp_1_context_1:
            return with_context_temp( push, 1, 1 );
        case ByteCodes::Code::push_temp_2_context_1:
            return with_context_temp( push, 2, 1 );
        case ByteCodes::Code::push_temp_n_context_1:
            return with_context_temp( push, n, 1 );

        case ByteCodes::Code::push_temp_0_context_n:
            return with_context_temp( push, 0, n );
        case ByteCodes::Code::push_temp_1_context_n:
            return with_context_temp( push, 1, n );
        case ByteCodes::Code::push_temp_2_context_n:
            return with_context_temp( push, 2, n );
        case ByteCodes::Code::push_temp_n_context_n:
            return with_context_temp( push, n, n );

        case ByteCodes::Code::store_temp_0_context_0_pop:
            return with_context_temp( store_pop, 0, 0 );
        case ByteCodes::Code::store_temp_1_context_0_pop:
            return with_context_temp( store_pop, 1, 0 );
        case ByteCodes::Code::store_temp_2_context_0_pop:
            return with_context_temp( store_pop, 2, 0 );
        case ByteCodes::Code::store_temp_n_context_0_pop:
            return with_context_temp( store_pop, n, 0 );

        case ByteCodes::Code::store_temp_0_context_1_pop:
            return with_context_temp( store_pop, 0, 1 );
        case ByteCodes::Code::store_temp_1_context_1_pop:
            return with_context_temp( store_pop, 1, 1 );
        case ByteCodes::Code::store_temp_2_context_1_pop:
            return with_context_temp( store_pop, 2, 1 );
        case ByteCodes::Code::store_temp_n_context_1_pop:
            return with_context_temp( store_pop, n, 1 );

        case ByteCodes::Code::store_temp_0_context_n_pop:
            return with_context_temp( store_pop, 0, n );
        case ByteCodes::Code::store_temp_1_context_n_pop:
            return with_context_temp( store_pop, 1, n );
        case ByteCodes::Code::store_temp_2_context_n_pop:
            return with_context_temp( store_pop, 2, n );
        case ByteCodes::Code::store_temp_n_context_n_pop:
            return with_context_temp( store_pop, n, n );

        case ByteCodes::Code::copy_1_into_context:
            return copy_params_into_context( false, 1 );
        case ByteCodes::Code::copy_2_into_context:
            return copy_params_into_context( false, 2 );
        case ByteCodes::Code::copy_n_into_context:
            return copy_params_into_context( false, n );

        case ByteCodes::Code::copy_self_into_context:
            return copy_params_into_context( true, 0 );
        case ByteCodes::Code::copy_self_1_into_context:
            return copy_params_into_context( true, 1 );
        case ByteCodes::Code::copy_self_2_into_context:
            return copy_params_into_context( true, 2 );
        case ByteCodes::Code::copy_self_n_into_context:
            return copy_params_into_context( true, n );

            // self/context initialization
        case ByteCodes::Code::set_self_via_context:
            return set_self_via_context();

            // block closure allocation
        case ByteCodes::Code::push_new_closure_context_0:
            return push_closure( 0, true );
        case ByteCodes::Code::push_new_closure_context_1:
            return push_closure( 1, true );
        case ByteCodes::Code::push_new_closure_context_2:
            return push_closure( 2, true );
        case ByteCodes::Code::push_new_closure_context_n:
            return push_closure( n, true );

        case ByteCodes::Code::push_new_closure_tos_0:
            return push_closure( 0, false );
        case ByteCodes::Code::push_new_closure_tos_1:
            return push_closure( 1, false );
        case ByteCodes::Code::push_new_closure_tos_2:
            return push_closure( 2, false );
        case ByteCodes::Code::push_new_closure_tos_n:
            return push_closure( n, false );

            // context allocation
        case ByteCodes::Code::install_new_context_method_0:
            return install_context( 0, true );
        case ByteCodes::Code::install_new_context_method_1:
            return install_context( 1, true );
        case ByteCodes::Code::install_new_context_method_2:
            return install_context( 2, true );
        case ByteCodes::Code::install_new_context_method_n:
            return install_context( n, true );

        case ByteCodes::Code::install_new_context_block_1:
            return install_context( 1, false );
        case ByteCodes::Code::install_new_context_block_2:
            return install_context( 2, false );
        case ByteCodes::Code::install_new_context_block_n:
            return install_context( n, false );

            // primitive calls
        case ByteCodes::Code::prim_call:                 // fall through
        case ByteCodes::Code::primitive_call_self:
            return call_primitive();

        case ByteCodes::Code::primitive_call_failure:             // fall through
        case ByteCodes::Code::primitive_call_self_failure:
            return call_primitive_can_fail();

        case ByteCodes::Code::primitive_call_lookup:             // fall through
        case ByteCodes::Code::primitive_call_self_lookup:         // fall through
        case ByteCodes::Code::primitive_call_failure_lookup:         // fall through
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return lookup_primitive();

        case ByteCodes::Code::dll_call_sync:
            return call_DLL( false );
        case ByteCodes::Code::dll_call_async:
            return call_DLL( true );

        case ByteCodes::Code::predict_primitive_call:             // fall through
        case ByteCodes::Code::predict_primitive_call_lookup:         // fall through
        case ByteCodes::Code::predict_primitive_call_failure:         // fall through
        case ByteCodes::Code::predict_primitive_call_failure_lookup:
            return predict_prim( true );

            // control flow
        case ByteCodes::Code::ifTrue_byte:            // fall through
        case ByteCodes::Code::ifFalse_byte:           // fall through
        case ByteCodes::Code::and_byte:               // fall through
        case ByteCodes::Code::or_byte:                // fall through
        case ByteCodes::Code::ifTrue_word:            // fall through
        case ByteCodes::Code::ifFalse_word:           // fall through
        case ByteCodes::Code::and_word:               // fall through
        case ByteCodes::Code::or_word:
            return control_cond( code );

        case ByteCodes::Code::whileTrue_byte:         // fall through
        case ByteCodes::Code::whileFalse_byte:        // fall through
        case ByteCodes::Code::whileTrue_word:         // fall through
        case ByteCodes::Code::whileFalse_word:
            return control_while( code );

        case ByteCodes::Code::jump_else_byte:         // fall through
        case ByteCodes::Code::jump_loop_byte:         // fall through
        case ByteCodes::Code::jump_else_word:         // fall through
        case ByteCodes::Code::jump_loop_word:
            return control_jump( code );

            // floating-point operations
        case ByteCodes::Code::float_allocate:
            return float_allocate();
        case ByteCodes::Code::float_floatify_pop:
            return float_floatify();
        case ByteCodes::Code::float_move:
            return float_move();
        case ByteCodes::Code::float_set:
            return float_set();
        case ByteCodes::Code::float_nullary_op:
            return float_op( 0, returns_float );
        case ByteCodes::Code::float_unary_op:
            return float_op( 1, returns_float );
        case ByteCodes::Code::float_binary_op:
            return float_op( 2, returns_float );
        case ByteCodes::Code::float_unary_op_to_oop:
            return float_op( 1 );
        case ByteCodes::Code::float_binary_op_to_oop:
            return float_op( 2 );

            // accessor sends
        case ByteCodes::Code::access_send_0:
            return access_send( false );
        case ByteCodes::Code::access_send_self:
            return access_send( true );

            // primitive sends
        case ByteCodes::Code::primitive_send_0:               // fall through
        case ByteCodes::Code::primitive_send_1:               // fall through
        case ByteCodes::Code::primitive_send_2:               // fall through
        case ByteCodes::Code::primitive_send_n:               // fall through

        case ByteCodes::Code::primitive_send_0_pop:           // fall through
        case ByteCodes::Code::primitive_send_1_pop:           // fall through
        case ByteCodes::Code::primitive_send_2_pop:           // fall through
        case ByteCodes::Code::primitive_send_n_pop:           // fall through

        case ByteCodes::Code::primitive_send_self:            // fall through
        case ByteCodes::Code::primitive_send_self_pop:        // fall through

        case ByteCodes::Code::primitive_send_super:           // fall through
        case ByteCodes::Code::primitive_send_super_pop:
            return primitive_send( code );

            // interpreted sends
        case ByteCodes::Code::interpreted_send_0:             // fall through
        case ByteCodes::Code::interpreted_send_1:             // fall through
        case ByteCodes::Code::interpreted_send_2:             // fall through
        case ByteCodes::Code::interpreted_send_n:             // fall through

        case ByteCodes::Code::interpreted_send_0_pop:         // fall through
        case ByteCodes::Code::interpreted_send_1_pop:         // fall through
        case ByteCodes::Code::interpreted_send_2_pop:         // fall through
        case ByteCodes::Code::interpreted_send_n_pop:         // fall through

        case ByteCodes::Code::interpreted_send_self:         // fall through
        case ByteCodes::Code::interpreted_send_self_pop:         // fall through

        case ByteCodes::Code::interpreted_send_super:         // fall through
        case ByteCodes::Code::interpreted_send_super_pop:
            return interpreted_send( code );

            // compiled sends
        case ByteCodes::Code::compiled_send_0:             // fall through
        case ByteCodes::Code::compiled_send_1:             // fall through
        case ByteCodes::Code::compiled_send_2:             // fall through
        case ByteCodes::Code::compiled_send_n:             // fall through

        case ByteCodes::Code::compiled_send_0_pop:             // fall through
        case ByteCodes::Code::compiled_send_1_pop:             // fall through
        case ByteCodes::Code::compiled_send_2_pop:             // fall through
        case ByteCodes::Code::compiled_send_n_pop:             // fall through

        case ByteCodes::Code::compiled_send_self:             // fall through
        case ByteCodes::Code::compiled_send_self_pop:         // fall through

        case ByteCodes::Code::compiled_send_super:             // fall through
        case ByteCodes::Code::compiled_send_super_pop:
            return compiled_send( code );

            // polymorphic sends
        case ByteCodes::Code::polymorphic_send_0:             // fall through
        case ByteCodes::Code::polymorphic_send_1:             // fall through
        case ByteCodes::Code::polymorphic_send_2:             // fall through
        case ByteCodes::Code::polymorphic_send_n:             // fall through

        case ByteCodes::Code::polymorphic_send_0_pop:         // fall through
        case ByteCodes::Code::polymorphic_send_1_pop:         // fall through
        case ByteCodes::Code::polymorphic_send_2_pop:         // fall through
        case ByteCodes::Code::polymorphic_send_n_pop:         // fall through

        case ByteCodes::Code::polymorphic_send_self:         // fall through
        case ByteCodes::Code::polymorphic_send_self_pop:         // fall through

        case ByteCodes::Code::polymorphic_send_super:         // fall through
        case ByteCodes::Code::polymorphic_send_super_pop:
            return polymorphic_send( code );

            // megamorphic sends
        case ByteCodes::Code::megamorphic_send_0:             // fall through
        case ByteCodes::Code::megamorphic_send_1:             // fall through
        case ByteCodes::Code::megamorphic_send_2:             // fall through
        case ByteCodes::Code::megamorphic_send_n:             // fall through

        case ByteCodes::Code::megamorphic_send_0_pop:         // fall through
        case ByteCodes::Code::megamorphic_send_1_pop:         // fall through
        case ByteCodes::Code::megamorphic_send_2_pop:         // fall through
        case ByteCodes::Code::megamorphic_send_n_pop:         // fall through

        case ByteCodes::Code::megamorphic_send_self:          // fall through
        case ByteCodes::Code::megamorphic_send_self_pop:      // fall through

        case ByteCodes::Code::megamorphic_send_super:         // fall through
        case ByteCodes::Code::megamorphic_send_super_pop:
            return megamorphic_send( code );

            // predicted smi_t sends
        case ByteCodes::Code::smi_add:
            return smi_add();
        case ByteCodes::Code::smi_sub:
            return smi_sub();
        case ByteCodes::Code::smi_mult:
            return smi_mul();

        case ByteCodes::Code::smi_equal:                      // fall through
        case ByteCodes::Code::smi_not_equal:                  // fall through
        case ByteCodes::Code::smi_less:                       // fall through
        case ByteCodes::Code::smi_less_equal:                 // fall through
        case ByteCodes::Code::smi_greater:                    // fall through
        case ByteCodes::Code::smi_greater_equal:
            return smi_compare_op( code );

        case ByteCodes::Code::smi_and:                        // fall through
        case ByteCodes::Code::smi_or:                         // fall through
        case ByteCodes::Code::smi_xor:
            return smi_logical_op( code );
        case ByteCodes::Code::smi_shift:
            return smi_shift();

            // local returns
        case ByteCodes::Code::return_tos_pop_0:
            return local_return( false, 0 );
        case ByteCodes::Code::return_tos_pop_1:
            return local_return( false, 1 );
        case ByteCodes::Code::return_tos_pop_2:
            return local_return( false, 2 );
        case ByteCodes::Code::return_tos_pop_n:
            return local_return( false, n );

        case ByteCodes::Code::return_self_pop_0:
            return local_return( true, 0 );
        case ByteCodes::Code::return_self_pop_1:
            return local_return( true, 1 );
        case ByteCodes::Code::return_self_pop_2:
            return local_return( true, 2 );
        case ByteCodes::Code::return_self_pop_n:
            return local_return( true, n );

        case ByteCodes::Code::return_tos_zap_pop_n:
            return local_return( false, n, true );
        case ByteCodes::Code::return_self_zap_pop_n:
            return local_return( true, n, true );

            // non-local returns
        case ByteCodes::Code::non_local_return_tos_pop_n:
            return nonlocal_return_tos();
        case ByteCodes::Code::non_local_return_self_pop_n:
            return nonlocal_return_self();

            // globals
        case ByteCodes::Code::push_global:
            return push_global();
        case ByteCodes::Code::store_global_pop:
            return store_global( pop );
        case ByteCodes::Code::store_global:
            return store_global();

        case ByteCodes::Code::push_classVar:
            return push_global();        // same as for globals
        case ByteCodes::Code::store_classVar_pop:
            return store_global( pop );    // same as for globals
        case ByteCodes::Code::store_classVar:
            return store_global();    // same as for globals

            // miscellaneous
        case ByteCodes::Code::special_primitive_send_1_hint:
            return special_primitive_send_hint();
        case ByteCodes::Code::double_equal:
            return compare( true );
        case ByteCodes::Code::double_tilde:
            return compare( false );
        case ByteCodes::Code::halt:
            return halt();

            // not implemented yet
        case ByteCodes::Code::smi_div:                    // fall through
        case ByteCodes::Code::smi_mod:                    // fall through
        case ByteCodes::Code::smi_create_point:           // fall through

        case ByteCodes::Code::objArray_at:                // fall through
        case ByteCodes::Code::objArray_at_put:            // fall through

        case ByteCodes::Code::return_instVar_name:        // fall through
        case ByteCodes::Code::push_instVar_name:          // fall through
        case ByteCodes::Code::store_instVar_pop_name:     // fall through
        case ByteCodes::Code::store_instVar_name:         // fall through

        case ByteCodes::Code::push_classVar_name:         // fall through
        case ByteCodes::Code::store_classVar_pop_name:    // fall through
        case ByteCodes::Code::store_classVar_name:        // fall through

            // unimplemented
        case ByteCodes::Code::unimplemented_06:           // fall through

        case ByteCodes::Code::unimplemented_20:           // fall through
        case ByteCodes::Code::unimplemented_21:           // fall through
        case ByteCodes::Code::unimplemented_22:           // fall through
        case ByteCodes::Code::unimplemented_23:           // fall through
        case ByteCodes::Code::unimplemented_24:           // fall through
        case ByteCodes::Code::unimplemented_25:           // fall through
        case ByteCodes::Code::unimplemented_26:           // fall through
        case ByteCodes::Code::unimplemented_27:           // fall through

        case ByteCodes::Code::unimplemented_39:           // fall through
        case ByteCodes::Code::unimplemented_3a:           // fall through
        case ByteCodes::Code::unimplemented_3b:           // fall through
        case ByteCodes::Code::unimplemented_3c:           // fall through

        case ByteCodes::Code::unimplemented_b7:           // fall through
        case ByteCodes::Code::unimplemented_bc:           // fall through

        case ByteCodes::Code::unimplemented_c7:           // fall through
        case ByteCodes::Code::unimplemented_cc:           // fall through

        case ByteCodes::Code::unimplemented_dc:           // fall through
        case ByteCodes::Code::unimplemented_de:           // fall through
        case ByteCodes::Code::unimplemented_df:           // fall through

        case ByteCodes::Code::unimplemented_fa:           // fall through
        case ByteCodes::Code::unimplemented_fb:           // fall through
        case ByteCodes::Code::unimplemented_fc:           // fall through
        case ByteCodes::Code::unimplemented_fd:           // fall through
        case ByteCodes::Code::unimplemented_fe:
            return nullptr;

#ifdef TEST_GENERATION
            default:						 ShouldNotReachHere();
#else
        default:
            return nullptr;
#endif // TEST_GENERATION

    }
    ShouldNotReachHere();
}


void InterpreterGenerator::info( const char * name ) {

    _console->print_cr( "%%interpreter-generate [%s]", name );

    if ( not PrintInterpreter ) {
        return;
    }

    _macroAssembler->code()->decode();
    _console->cr();
}


void InterpreterGenerator::generate_all() {
    Interpreter::_code_begin_addr = _macroAssembler->pc();

    generate_forStubRoutines();
    info( "code for StubRoutines" );

    StubRoutines::init();

    // generate code for Floats
    Floats::init( _macroAssembler );
    Floats::_function_table[ Floats::oopify ] = float_oopify();    // patch - no code generated in Floats for oopify
    info( "Floats::oopify patch" );

    // generate helper routines/code fragments
    generate_error_handler_code();
    info( "error handler code" );

    generate_nonlocal_return_code();
    info( "non-local return code" );

    generate_method_entry_code();
    info( "method entry code" );

    generate_inline_cache_miss_handler();
    info( "inline cache miss handler" );

    generate_predicted_smi_send_failure_handler();
    info( "predicted smi_t send failure handler" );

    generate_redo_send_code();
    info( "redo send code" );

    generate_deoptimized_return_code();
    info( "deoptimized return code" );

    for ( int n = 0; n < 10; n++ )
        generate_primitiveValue( n );
    info( "primitiveValues" );


    // generate individual instructions
    _console->cr();

    for ( int i = 0; i < static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES); i++ ) {

        const char * start = _macroAssembler->pc();
        const char * entry = generate_instruction( ( ByteCodes::Code ) i );
        if ( not entry ) {
            continue;
        }


        ByteCodes::set_entry_point( ByteCodes::Code( i ), entry );
        if ( PrintInterpreter ) {
            int length = _macroAssembler->pc() - start;
            const char * name = ByteCodes::name( ( ByteCodes::Code ) i );
            _console->print_cr( "bytecode # [0x%02x], address [0x%0x], size [0x%04x], name [%s]", i, entry, length, name );
            _macroAssembler->code()->decode();
            _console->cr();
        }

    }

    _macroAssembler->finalize();
    Interpreter::_code_end_addr = _macroAssembler->pc();
}


InterpreterGenerator::InterpreterGenerator( CodeBuffer * code, bool_t debug ) {
    _macroAssembler = new MacroAssembler( code );
    _debug          = debug;
    _stack_check    = Interpreter::has_stack_checks();
}


static constexpr int interpreter_size = 40000;
static const char * interpreter_code;


void interpreter_init() {
    _console->print_cr( "%%system-init:  interpreter_init" );

    interpreter_code = os::exec_memory( interpreter_size );

    CodeBuffer * code = new CodeBuffer( &interpreter_code[ 0 ], interpreter_size );

    const bool_t debug = true; // change this to switch between debug/optimized version

    InterpreterGenerator( code, debug ).generate_all();
    _console->print_cr( "%%interpreter-size [%d] [0x%0x] bytes", code->code_size(), code->code_size() );

    Interpreter::init();
}
