
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/memory/oopFactory.hpp"


// All debug entries should be wrapped with a stack allocated Command object.
// It makes sure a resource mark is set and flushes the logfile to prevent file sharing problems.

class Command {

private:
    ResourceMark resourceMark;

public:
    Command( const char *str ) :
        resourceMark{} {
        spdlog::info( "Executing[{}]", str );
    }


    ~Command() {
    }

};


void pp( void *p ) {

    Command     c( "pp" );
    FlagSetting fl( PrintVMMessages, true );
    if ( p == nullptr ) {
        spdlog::info( "0x0" );
        return;
    }

    if ( Universe::is_heap( (Oop *) p ) ) {
        MemOop obj = as_memOop( Universe::object_start( (Oop *) p ) );
        obj->print();
        if ( obj->is_method() ) {
            std::int32_t byteCodeIndex = MethodOop( obj )->byteCodeIndex_from( (std::uint8_t *) p );
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
void pv( std::int32_t p ) {
    ( (PrintableResourceObject *) p )->print();
}


void pp_short( void *p ) {
    Command     c( "pp_short" );
    FlagSetting fl( PrintVMMessages, true );
    if ( p == nullptr ) {
        spdlog::info( "0x0" );
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
        Command      c( "pd" );
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


void oat( std::int32_t index ) {
    Command c( "oat" );
    if ( ObjectIDTable::is_index_ok( index ) ) {
        Oop obj = ObjectIDTable::at( index );
        obj->print();
    } else {
        spdlog::info( "index {} out of bounds", index );
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
    //flush_logFile();
}


void ndebug() {        // undo debug()
    Command c( "ndebug" );
    PrintCompilation       = PrintInlining        = PrintSplitting = PrintCode    = PrintAssemblyCode = PrintEliminateUnnededNodes = false;
    PrintEliminateContexts = PrintCopyPropagation = PrintRScopes   = PrintExposed = PrintLoopOpts     = false;
    AlwaysFlushVMMessages  = false;
    //flush_logFile();
}


void flush() {
    Command c( "flush" );
    //flush_logFile();
}


void events() {
    Command c( "events" );
    eventLog->printPartial( 50 );
}


NativeMethod *find( std::int32_t addr ) {
    Command c( "find" );
    return findNativeMethod( (void *) addr );
}


MethodOop findm( std::int32_t hp ) {
    Command c( "findm" );
    return MethodOopDescriptor::methodOop_from_hcode( (std::uint8_t *) hp );
}

// std::int32_t versions of all methods to avoid having to type casts in the debugger

void pp( std::int32_t p ) {
    pp( (void *) p );
}


void pp_short( std::int32_t p ) {
    pp_short( (void *) p );
}


void pk( std::int32_t p ) {
    pk( (Klass *) p );
}


void ph( std::int32_t hp ) {
    Command c( "ph" );
    findm( hp )->pretty_print();
}


void pm( std::int32_t m ) {
    Command c( "pm" );
    MethodOop( m )->pretty_print();
}


void print_codes( const char *class_name, const char *selector ) {
    Command c( "print_codes" );
    spdlog::info( "Finding %s in %s.", selector, class_name );
    Oop result = Universe::find_global( class_name );
    if ( not result ) {
        spdlog::info( "Could not find global %s.", class_name );
    } else if ( not result->is_klass() ) {
        spdlog::info( "Global %s is not a class.", class_name );
    } else {
        SymbolOop sel    = oopFactory::new_symbol( selector );
        MethodOop method = KlassOop( result )->klass_part()->lookup( sel );
        if ( not method )
            method = result->blueprint()->lookup( sel );
        if ( not method ) {
            spdlog::info( "Method %s is not in %s.", selector, class_name );
        } else {
            method->pretty_print();
            method->print_codes();
        }
    }
}


void help() {
    Command c( "help" );


    spdlog::info( "basic" );
    spdlog::info( "  pp(void* p)   - try to make sense of p" );
    spdlog::info( "  pv(std::int32_t p)     - ((PrintableResourceObject*) p)->print()" );
    spdlog::info( "  ps()          - print current process stack" );
    spdlog::info( "  pss()         - print all process stacks" );
    spdlog::info( "  oat(std::int32_t i)    - print object with id = i" );

    spdlog::info( "methodOop" );
    spdlog::info( "  pm(std::int32_t m)     - pretty print methodOop(m)" );
    spdlog::info( "  ph(std::int32_t hp)    - pretty print method containing hp" );
    spdlog::info( "  findm(std::int32_t hp) - returns methodOop containing hp" );

    spdlog::info( "misc." );
    spdlog::info( "  flush()       - flushes the log file" );
    spdlog::info( "  events()      - dump last 50 event" );


    spdlog::info( "compiler debugging" );
    spdlog::info( "  debug()       - to set things up for compiler debugging" );
    spdlog::info( "  ndebug()      - undo debug" );
    spdlog::info( "  pc()          - theCompiler->print_code(false)" );
    spdlog::info( "  pscopes()     - theCompiler->topScope->printTree()" );
    spdlog::info( "  urs_ps()      - print current process stack with many flags turned on" );
}
