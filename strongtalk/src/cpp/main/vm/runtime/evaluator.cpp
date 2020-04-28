//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#ifndef __MINGW32__

#include <readline/readline.h>
#include <readline/history.h>

#endif

#include "vm/runtime/evaluator.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/system/os.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/runtime/ResourceMark.hpp"


// The single_step_handler is called from single_step_stub when a single step has taken place
// return the distance between the current and the next_byteCodeIndex
extern "C" void single_step_handler() {
    ResourceMark resourceMark;
    evaluator::single_step( DeltaProcess::active()->last_Delta_fp() );
}

int * saved_frame;


bool_t patch_last_delta_frame( int * fr, int * dist ) {

    // change the current to next byteCodeIndex;
    Frame v( nullptr, fr, nullptr );

    MethodOop method = MethodOopDescriptor::methodOop_from_hcode( v.hp() );

    // The interpreter is in the middle of executing a byte code
    // and the hp pointer points to the current byteCodeIndex and NOT the next byteCodeIndex.
    int byteCodeIndex = method->next_byteCodeIndex_from( v.hp() );

    // in case of single step we ignore the case with
    // an empty inline cache since the send is reexecuted.
    InterpretedInlineCache * ic = method->ic_at( byteCodeIndex );
    if ( ic and ic->is_empty() )
        return false;

    *dist = method->next_byteCodeIndex( byteCodeIndex ) - byteCodeIndex;
    v.set_hp( v.hp() + *dist );
    return true;
}


void restore_hp( int * fr, int dist ) {
    Frame v( nullptr, fr, nullptr );
    v.set_hp( v.hp() - dist );
}


static bool_t is_aborting = false;


void evaluator::single_step( int * fr ) {
    int dist;
    if ( not patch_last_delta_frame( fr, &dist ) )
        return;

    DeltaVirtualFrame * df = DeltaProcess::active()->last_delta_vframe();
    st_assert( df, "delta frame must be present" );

    { // Always show code at single step
        FlagSetting( ActivationShowCode, true );
        df->print_activation( 1 );
    }
    saved_frame = fr;
    read_eval_loop();

    restore_hp( fr, dist );

    if ( is_aborting ) {
        is_aborting = false;
        ErrorHandler::abort_current_process();
    }
}


bool_t evaluator::get_line( char * line ) {
    int end = 0;
    int c;

    while ( ( ( c = getchar() ) not_eq EOF ) and ( c not_eq '\n' ) )
        line[ end++ ] = c;

    while ( ( end > 0 ) and ( ( line[ end - 1 ] == ' ' ) or ( line[ end - 1 ] == '\t' ) ) )
        end--;

    line[ end ] = '\0';
    return c not_eq EOF;
}


class TokenStream : public StackAllocatedObject {

    private:
        GrowableArray <char *> * tokens;
        int                    pos;

        void tokenize( char * str );


        bool_t match( const char * str ) {
            return strcmp( current(), str ) == 0;
        }


    public:
        TokenStream( const char * line ) {
            tokens = new GrowableArray <char *>( 10 );
            tokenize( const_cast<char *>( line ) );
            pos = 0;
        }


        char * current() {
            return tokens->at( pos );
        }


        void advance() {
            pos++;
        }


        bool_t eos() {
            return pos >= tokens->length();
        }


        // testers
        bool_t is_hat() {
            return match( "^" );
        }


        bool_t is_step() {
            return match( "s" ) or match( "step" );
        }


        bool_t is_next() {
            return match( "n" ) or match( "next" );
        }


        bool_t is_end() {
            return match( "e" ) or match( "end" );
        }


        bool_t is_cont() {
            return match( "c" ) or match( "cont" );
        }


        bool_t is_stack() {
            return match( "stack" );
        }


        bool_t is_abort() {
            return match( "abort" );
        }


        bool_t is_genesis() {
            return match( "genesis" );
        }


        bool_t is_top() {
            return match( "top" );
        }


        bool_t is_show() {
            return match( "show" );
        }


        bool_t is_break() {
            return match( "break" );
        }


        bool_t is_events() {
            return match( "events" );
        }


        bool_t is_status() {
            return match( "status" );
        }


        bool_t is_help() {
            return match( "?" ) or match( "help" );
        }


        bool_t is_quit() {
            return match( "q" ) or match( "quit" );
        }


        bool_t is_plus() {
            return match( "+" );
        }


        bool_t is_minus() {
            return match( "-" );
        }


        bool_t is_smi( Oop * addr );

        bool_t is_table_entry( Oop * addr );

        bool_t is_object_search( Oop * addr );

        bool_t is_name( Oop * addr );

        bool_t is_symbol( Oop * addr );

        bool_t is_unary();

        bool_t is_binary();

        bool_t is_keyword();
};


static const char * seps = " \t\n";


void TokenStream::tokenize( char * str ) {
    char * token = strtok( str, seps );
    while ( token not_eq nullptr ) {
        tokens->push( token );
        token = strtok( nullptr, seps );
    }
}


bool_t TokenStream::is_smi( Oop * addr ) {
    int      value;
    uint32_t length;

    if ( sscanf( current(), "%d%u", &value, &length ) == 1 and strlen( current() ) == length ) {
        *addr = smiOopFromValue( value );
        return true;
    }
    return false;
}


bool_t TokenStream::is_table_entry( Oop * addr ) {
    int      value;
    uint32_t length;
    if ( sscanf( current(), "!%d%u", &value, &length ) == 1 and strlen( current() ) == length ) {
        if ( not objectIDTable::is_index_ok( value ) ) {
            _console->print_cr( "Could not find index %d in object table.", value );
            return true;
        }
        *addr = objectIDTable::at( value );
        return true;
    }
    return false;
}


bool_t TokenStream::is_object_search( Oop * addr ) {
    int      address;
    Oop      obj;
    uint32_t length;

    if ( sscanf( current(), "0x%X%u", &address, &length ) == 1 and strlen( current() ) == length ) {
        if ( obj = Oop( Universe::object_start( ( Oop * ) address ) ) ) {
            *addr = obj;
            return true;
        }
    }
    return false;
}


bool_t TokenStream::is_name( Oop * addr ) {
    char     name[200];
    Oop      obj;
    uint32_t length;
    if ( sscanf( current(), "%[a-zA-Z]%u", name, &length ) == 1 and strlen( current() ) == length ) {
        if ( obj = Universe::find_global( name ) ) {
            *addr = obj;
            return true;
        }
    }
    return false;
}


bool_t TokenStream::is_symbol( Oop * addr ) {
    char     name[200];
    uint32_t length;
    if ( sscanf( current(), "#%[a-zA-Z0-9_]%u", name, &length ) == 1 and strlen( current() ) == length ) {
        *addr = oopFactory::new_symbol( name );
        return true;
    }
    return false;
}


bool_t TokenStream::is_unary() {
    char     name[40];
    uint32_t length;
    return sscanf( current(), "%[a-zA-Z]%u", name, &length ) == 1 and strlen( current() ) == length;
}


bool_t TokenStream::is_binary() {
    return not is_unary() and not is_keyword();
}


bool_t TokenStream::is_keyword() {
    char     name[40];
    uint32_t length;
    return sscanf( current(), "%[a-zA-Z]:%u", name, &length ) == 1 and strlen( current() ) == length;
}


bool_t evaluator::get_oop( TokenStream * stream, Oop * addr ) {

    if ( stream->is_smi( addr ) ) {
        stream->advance();
        return true;
    }
    if ( stream->is_table_entry( addr ) ) {
        stream->advance();
        return true;
    }
    if ( stream->is_object_search( addr ) ) {
        stream->advance();
        return true;
    }
    if ( stream->is_name( addr ) ) {
        stream->advance();
        return true;
    }
    if ( stream->is_symbol( addr ) ) {
        stream->advance();
        return true;
    }
    _console->print_cr( "Error: could not Oop'ify [%s]", stream->current() );
    return false;
}


bool_t validate_lookup( Oop receiver, SymbolOop selector ) {
    LookupKey key( receiver->klass(), selector );
    if ( LookupCache::lookup( &key ).is_empty() ) {
        _console->print_cr( "Lookup error" );
        key.print_on( _console );
        _console->cr();
        return false;
    }
    return true;
}


void evaluator::eval_message( TokenStream * stream ) {
    Oop       receiver;
    Oop       result = nilObj;
    SymbolOop selector;

    if ( stream->eos() )
        return;
    if ( not get_oop( stream, &receiver ) )
        return;
    if ( stream->eos() ) {
        receiver->print();
    } else if ( stream->is_unary() ) {
        SymbolOop selector = oopFactory::new_symbol( stream->current() );
        if ( not validate_lookup( receiver, selector ) )
            return;
        result = Delta::call( receiver, selector );
    } else if ( stream->is_binary() ) {
        selector = oopFactory::new_symbol( stream->current() );
        if ( not validate_lookup( receiver, selector ) )
            return;
        Oop argument;
        stream->advance();
        if ( not get_oop( stream, &argument ) )
            return;
        result = Delta::call( receiver, selector, argument );
    } else if ( stream->is_keyword() ) {
        char name[100];
        Oop  arguments[10];
        int  nofArgs = 0;
        name[ 0 ] = '\0';
        while ( not stream->eos() ) {
            strcat( name, stream->current() );
            stream->advance();
            Oop arg;
            if ( not get_oop( stream, &arg ) )
                return;
            arguments[ nofArgs++ ] = arg;
        }
        selector = oopFactory::new_symbol( name );
        if ( not validate_lookup( receiver, selector ) )
            return;
        static DeltaCallCache cache;
        result = Delta::call_generic( &cache, receiver, selector, nofArgs, arguments );
    }
    result->print_value();
    _console->cr();
}


void evaluator::top_command( TokenStream * stream ) {
    int number_of_frames_to_show = 10;
    stream->advance();
    if ( not stream->eos() ) {
        Oop value;
        if ( stream->is_smi( &value ) ) {
            number_of_frames_to_show = SMIOop( value )->value();
        }
        stream->advance();
        if ( not stream->eos() ) {
            _console->print_cr( "warning: garbage at end" );
        }
    }
    DeltaProcess::active()->trace_top( 1, number_of_frames_to_show );
}


void evaluator::change_debug_flag( TokenStream * stream, bool_t value ) {
    stream->advance();
    if ( not stream->eos() ) {
        stream->current();
        bool_t r = value;
        if ( not debugFlags::boolAtPut( stream->current(), &r ) ) {
            _console->print_cr( "boolean flag %s not found", stream->current() );
        }
        stream->advance();
        if ( not stream->eos() ) {
            _console->print_cr( "warning: garbage at end" );
        }
    } else {
        _console->print_cr( "boolean flag expected" );
    }
}


void evaluator::show_command( TokenStream * stream ) {
    int start_frame              = 1;
    int number_of_frames_to_show = 1;

    stream->advance();
    if ( not stream->eos() ) {
        Oop value;
        if ( stream->is_smi( &value ) ) {
            start_frame = SMIOop( value )->value();
        }
        stream->advance();
        if ( not stream->eos() ) {
            if ( stream->is_smi( &value ) ) {
                number_of_frames_to_show = SMIOop( value )->value();
            }
            stream->advance();
            if ( not stream->eos() ) {
                _console->print_cr( "warning: garbage at end" );
            }
        }
    }
    DeltaProcess::active()->trace_top( start_frame, number_of_frames_to_show );
}


bool_t evaluator::process_line( const char * line ) {

    TokenStream stream( line );
    if ( stream.eos() )
        return true;

    if ( stream.is_hat() ) {
        stream.advance();
        DispatchTable::reset();
        eval_message( &stream );
        return true;
    } else {
        if ( stream.is_help() ) {
            print_help();
            return true;
        }
        if ( stream.is_step() ) {
            DispatchTable::intercept_for_step( nullptr );
            return false;
        }
        if ( stream.is_next() ) {
            DispatchTable::intercept_for_next( saved_frame );
            return false;
        }
        if ( stream.is_end() ) {
            DispatchTable::intercept_for_return( saved_frame );
            return false;
        }
        if ( stream.is_cont() ) {
            DispatchTable::reset();
            return false;
        }
        if ( stream.is_stack() ) {
            DeltaProcess::active()->trace_stack();
            return true;
        }
        if ( stream.is_quit() ) {
            os::fatalExit( 0 );
            return true;
        }
        if ( stream.is_break() ) {
            fatal( "evaluator break" );
            return true;
        }
        if ( stream.is_events() ) {
            eventLog->print();
            return true;
        }
        if ( stream.is_top() ) {
            top_command( &stream );
            return true;
        }
        if ( stream.is_show() ) {
            show_command( &stream );
            return true;
        }
        if ( stream.is_plus() ) {
            change_debug_flag( &stream, true );
            return true;
        }
        if ( stream.is_minus() ) {
            change_debug_flag( &stream, false );
            return true;
        }
        if ( stream.is_status() ) {
            print_status();
            return true;
        }
        if ( stream.is_abort() ) {
            if ( DeltaProcess::active()->is_scheduler() ) {
                _console->print_cr( "You cannot abort in the scheduler" );
                _console->print_cr( "Try another command" );
            } else {
                DispatchTable::reset();
                is_aborting = true;
                return false;
            }
        }
        if ( stream.is_genesis() ) {
            DispatchTable::reset();
            VM_Genesis op;
            VMProcess::execute( &op );
            return false;
        }
        Oop receiver;
        if ( get_oop( &stream, &receiver ) ) {
            stream.advance();
            if ( not stream.eos() ) {
                _console->print_cr( "warning: garbage at end" );
            }
            receiver->print();
            _console->cr();
            return true;
        }
    }
    print_mini_help();
    return true;
}


#ifndef  __MINGW32__


void evaluator::read_eval_loop() {
    ResourceMark resourceMark;

    char * line;
    while ( ( line = readline( "Eval> " ) ) not_eq nullptr ) {
        if ( strlen( line ) > 0 ) {
            add_history( line );
        }

        //
        process_line( line );

        // readline malloc's a new buffer every time.
        free( line );
    }

}


#else


void evaluator::read_eval_loop() {
    ResourceMark rm;
    char         line[200];
    do {
        _console->print( "Eval> " );
        if ( !get_line( line ) )
            return;
    } while ( process_line( line ) );
}


#endif


void evaluator::print_mini_help() {
    _console->print_cr( "Use '?' for help ('c' to continue)" );
}


class ProcessStatusClosure : public ProcessClosure {
    private:
        int index;
    public:
        ProcessStatusClosure() {
            index = 1;
        }


        void do_process( DeltaProcess * p ) {
            _console->print( " %d:%s ", index++, DeltaProcess::active() == p ? "*" : " " );
            p->print();
        }
};


void evaluator::print_status() {
    _console->print_cr( "Processes:" );
    ProcessStatusClosure iter;
    Processes::process_iterate( &iter );
}


void evaluator::print_help() {
    _console->cr();
    _console->print_cr( "<command>  ::= 'q'     | 'quit'    -> quits the system" );
    _console->print_cr( "             | 's'     | 'step'    -> single step byte code" );
    _console->print_cr( "             | 'n'     | 'next'    -> single step statement" );
    _console->print_cr( "             | 'e'     | 'end'     -> single step to end of method" );
    _console->print_cr( "             | 'c'     | 'cont'    -> continue execution" );
    _console->print_cr( "                       | 'abort'   -> aborts the current process" );
    _console->print_cr( "                       | 'genesis' -> aborts all processes and restarts the scheduler" );
    _console->print_cr( "                       | 'break'   -> provokes fatal() to get into C++ debugger" );
    _console->print_cr( "                       | 'events'  -> prints the event log" );
    _console->print_cr( "                       | 'stack'   -> prints the stack of current process" );
    _console->print_cr( "                       | 'status'  -> prints the status all processes" );
    _console->print_cr( "                       | 'top' <n> -> prints the top of current process" );
    _console->print_cr( "                       | 'show' <s> <n> -> prints some activation" );
    _console->print_cr( "             | '?'     | 'help'    -> prints this help\n" );
    _console->print_cr( "             | '^' <expr>          -> evaluates the expression" );
    _console->print_cr( "             | '-' name            -> turns off debug flag" );
    _console->print_cr( "             | '+' name            -> turns on debug flag" );
    _console->print_cr( "             | <object>            -> prints this object\n" );
    _console->cr();
    _console->print_cr( "<expr>     ::= <unary>  | <binary>  | <keyword>\n" );
    _console->print_cr( "<object>   ::= <number>            -> smi_t(number)" );
    _console->print_cr( "             | !<number>           -> objectTable[number]" );
    _console->print_cr( "             | 0x<hex_number>      -> object_start(number)" );
    _console->print_cr( "             | name                -> Smalltalk at: #name" );
    _console->print_cr( "             | #name               -> new_symbol(name)" );
    _console->cr();
}
