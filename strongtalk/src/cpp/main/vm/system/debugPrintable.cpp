
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/memory/oopFactory.hpp"


// All debug entries should be wrapped with a stack allocated Command object.
// It makes sure a resource mark is set and flushes the logfile to prevent file sharing problems.

class Command {

private:
    ResourceMark resourceMark;

public:
    Command( const char *str ) {
        _console->cr();
        _console->print_cr( "Executing [%s]", str );
    }


    ~Command() {
        flush_logFile();
    }
};


void pp( void *p ) {

    Command     c( "pp" );
    FlagSetting fl( PrintVMMessages, true );
    if ( p == nullptr ) {
        lprintf( "0x0" );
        return;
    }

    if ( Universe::is_heap( (Oop *) p ) ) {
        MemOop obj = as_memOop( Universe::object_start( (Oop *) p ) );
        obj->print();
        if ( obj->is_method() ) {
            int byteCodeIndex = MethodOop( obj )->byteCodeIndex_from( (std::uint8_t *) p );
            PrettyPrinter::print( MethodOop( obj ), nullptr, byteCodeIndex );
        }
        return;
    }

    if ( Oop( p )->is_smi() or Oop( p )->is_mark() ) {
        Oop( p )->print();
        return;
    }
}


// pv: print vm-printable object
void pv( int p ) {
    ( (PrintableResourceObject *) p )->print();
}


void pp_short( void *p ) {
    Command     c( "pp_short" );
    FlagSetting fl( PrintVMMessages, true );
    if ( p == nullptr ) {
        lprintf( "0x0" );
    } else if ( Oop( p )->is_mem() ) {
        // guess that it's a MemOop
        Oop( p )->print();
    } else {
        // guess that it's a VMObject*
        // FIX LATER ((VMObject*) p)->print_short_zero();
    }
}


void pk( Klass *p ) {
    Command     c( "pk" );
    FlagSetting fl( PrintVMMessages, true );
    p->print_klass();
}


void pr( void *m ) {
    Command c( "pr" );
    Universe::remembered_set->print_set_for_object( MemOop( m ) );
}


void ps() { // print stack
    {
        // Prints the stack of the current Delta process
        DeltaProcess *p = DeltaProcess::active();
        if ( not p )
            return;
        Command c( "ps" );
        _console->print( " for process: " );
        p->print();
        _console->cr();

        if ( p->last_Delta_fp() not_eq nullptr ) {
            // If the last_Delta_fp is set we are in C land and can call the standard stack_trace function.
            p->trace_stack();
        } else {
            // fp point to the frame of the ps stub routine
            Frame f = p->profile_top_frame();
            f = f.sender();
            p->trace_stack_from( VirtualFrame::new_vframe( &f ) );
        }
    }
}


void pss() { // print all stack
    Command c( "pss" );
    Processes::print();
}


void pd() { // print stack
    // Retrieve the frame pointer of the current frame
    {
        Command c( "pd" );
        // Prints the stack of the current Delta process
        DeltaProcess *p = DeltaProcess::active();
        _console->print( " for process: " );
        p->print();
        _console->cr();

        if ( p->last_Delta_fp() not_eq nullptr ) {
            // If the last_Delta_fp is set we are in C land and can call the standard stack_trace function.
            p->trace_stack_for_deoptimization();
        } else {
            // fp point to the frame of the ps stub routine
            Frame f = p->profile_top_frame();
            f = f.sender();
            p->trace_stack_for_deoptimization( &f );
        }
    }
}


void oat( int index ) {
    Command c( "oat" );
    if ( objectIDTable::is_index_ok( index ) ) {
        Oop obj = objectIDTable::at( index );
        obj->print();
    } else {
        _console->print_cr( "index %d out of bounds", index );
    }
}


// please use this to print stacks when reporting compiler bugs
void urs_ps() {
    FlagSetting f1( WizardMode, true );
    FlagSetting f2( PrintOopAddress, true );
    FlagSetting f3( ActivationShowCode, true );
    FlagSetting f4( MaterializeEliminatedBlocks, false );
    FlagSetting f5( BreakAtWarning, false );
    ps();
}


void pc() {
    Command c( "pc" );
    theCompiler->print_code( false );
}


void pscopes() {
    Command c( "pscopes" );
    theCompiler->topScope->printTree();
}


void debug() {        // to set things up for compiler debugging
    Command c( "debug" );
    WizardMode             = true;
    PrintVMMessages        = true;
    PrintCompilation       = PrintInlining        = PrintSplitting = PrintCode    = PrintAssemblyCode = PrintEliminateUnnededNodes = true;
    PrintEliminateContexts = PrintCopyPropagation = PrintRScopes   = PrintExposed = PrintLoopOpts     = true;
    AlwaysFlushVMMessages  = true;
    flush_logFile();
}


void ndebug() {        // undo debug()
    Command c( "ndebug" );
    PrintCompilation       = PrintInlining        = PrintSplitting = PrintCode    = PrintAssemblyCode = PrintEliminateUnnededNodes = false;
    PrintEliminateContexts = PrintCopyPropagation = PrintRScopes   = PrintExposed = PrintLoopOpts     = false;
    AlwaysFlushVMMessages  = false;
    flush_logFile();
}


void flush() {
    Command c( "flush" );
    flush_logFile();
}


void events() {
    Command c( "events" );
    eventLog->printPartial( 50 );
}


NativeMethod *find( int addr ) {
    Command c( "find" );
    return findNativeMethod( (void *) addr );
}


MethodOop findm( int hp ) {
    Command c( "findm" );
    return MethodOopDescriptor::methodOop_from_hcode( (std::uint8_t *) hp );
}

// int versions of all methods to avoid having to type casts in the debugger

void pp( int p ) {
    pp( (void *) p );
}


void pp_short( int p ) {
    pp_short( (void *) p );
}


void pk( int p ) {
    pk( (Klass *) p );
}


void ph( int hp ) {
    Command c( "ph" );
    findm( hp )->pretty_print();
}


void pm( int m ) {
    Command c( "pm" );
    MethodOop( m )->pretty_print();
}


void print_codes( const char *class_name, const char *selector ) {
    Command c( "print_codes" );
    _console->print_cr( "Finding %s in %s.", selector, class_name );
    Oop result = Universe::find_global( class_name );
    if ( not result ) {
        _console->print_cr( "Could not find global %s.", class_name );
    } else if ( not result->is_klass() ) {
        _console->print_cr( "Global %s is not a class.", class_name );
    } else {
        SymbolOop sel    = oopFactory::new_symbol( selector );
        MethodOop method = KlassOop( result )->klass_part()->lookup( sel );
        if ( not method )
            method = result->blueprint()->lookup( sel );
        if ( not method ) {
            _console->print_cr( "Method %s is not in %s.", selector, class_name );
        } else {
            method->pretty_print();
            method->print_codes();
        }
    }
}


void help() {
    Command c( "help" );


    _console->print_cr( "basic" );
    _console->print_cr( "  pp(void* p)   - try to make sense of p" );
    _console->print_cr( "  pv(int p)     - ((PrintableResourceObject*) p)->print()" );
    _console->print_cr( "  ps()          - print current process stack" );
    _console->print_cr( "  pss()         - print all process stacks" );
    _console->print_cr( "  oat(std::size_t i)    - print object with id = i" );

    _console->print_cr( "methodOop" );
    _console->print_cr( "  pm(int m)     - pretty print methodOop(m)" );
    _console->print_cr( "  ph(int hp)    - pretty print method containing hp" );
    _console->print_cr( "  findm(int hp) - returns methodOop containing hp" );

    _console->print_cr( "misc." );
    _console->print_cr( "  flush()       - flushes the log file" );
    _console->print_cr( "  events()      - dump last 50 event" );


    _console->print_cr( "compiler debugging" );
    _console->print_cr( "  debug()       - to set things up for compiler debugging" );
    _console->print_cr( "  ndebug()      - undo debug" );
    _console->print_cr( "  pc()          - theCompiler->print_code(false)" );
    _console->print_cr( "  pscopes()     - theCompiler->topScope->printTree()" );
    _console->print_cr( "  urs_ps()      - print current process stack with many flags turned on" );
}
