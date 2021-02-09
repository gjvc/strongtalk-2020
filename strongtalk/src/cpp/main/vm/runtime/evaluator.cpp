//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/os.hpp"
#include "vm/runtime/evaluator.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/interpreter/MethodClosure.hpp"


// The single_step_handler is called from single_step_stub when a single step has taken place
// return the distance between the current and the next_byteCodeIndex
extern "C" void single_step_handler() {
    ResourceMark resourceMark;
    evaluator::single_step( DeltaProcess::active()->last_delta_fp() );
}

std::int32_t *saved_frame;


bool patch_last_delta_frame( std::int32_t *fr, std::int32_t *dist ) {

    // change the current to next byteCodeIndex;
    Frame v( nullptr, fr, nullptr );

    MethodOop method = MethodOopDescriptor::methodOop_from_hcode( v.hp() );

    // The interpreter is in the middle of executing a byte code
    // and the hp pointer points to the current byteCodeIndex and NOT the next byteCodeIndex.
    std::int32_t byteCodeIndex = method->next_byteCodeIndex_from( v.hp() );

    // in case of single step we ignore the case with
    // an empty inline cache since the send is reexecuted.
    InterpretedInlineCache *ic = method->ic_at( byteCodeIndex );
    if ( ic and ic->is_empty() )
        return false;

    *dist = method->next_byteCodeIndex( byteCodeIndex ) - byteCodeIndex;
    v.set_hp( v.hp() + *dist );
    return true;
}


void restore_hp( std::int32_t *fr, std::int32_t dist ) {
    Frame v( nullptr, fr, nullptr );
    v.set_hp( v.hp() - dist );
}


static bool is_aborting = false;


void evaluator::single_step( std::int32_t *fr ) {
    std::int32_t dist;
    if ( not patch_last_delta_frame( fr, &dist ) )
        return;

    DeltaVirtualFrame *df = DeltaProcess::active()->last_delta_vframe();
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


bool evaluator::get_line( char *line ) {
    std::int32_t end = 0;
    std::int32_t c;

    while ( ( ( c = getchar() ) not_eq EOF ) and ( c not_eq '\n' ) )
        line[ end++ ] = c;

    while ( ( end > 0 ) and ( ( line[ end - 1 ] == ' ' ) or ( line[ end - 1 ] == '\t' ) ) )
        end--;

    line[ end ] = '\0';
    return c not_eq EOF;
}


class TokenStream : public StackAllocatedObject {

private:
    GrowableArray<char *> *tokens;
    std::int32_t          pos;

    void tokenize( char *str );


    bool match( const char *str ) {
        return strcmp( current(), str ) == 0;
    }


public:
    TokenStream( const char *line ) :
        tokens{ new GrowableArray<char *>( 10 ) },
        pos{ 0 } {
        tokenize( const_cast<char *>( line ) );
    }


    TokenStream() = default;
    virtual ~TokenStream() = default;
    TokenStream( const TokenStream & ) = default;
    TokenStream &operator=( const TokenStream & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    char *current() {
        return tokens->at( pos );
    }


    void advance() {
        pos++;
    }


    bool eos() {
        return pos >= tokens->length();
    }


    // testers
    bool is_hat() {
        return match( "^" );
    }


    bool is_step() {
        return match( "s" ) or match( "step" );
    }


    bool is_next() {
        return match( "n" ) or match( "next" );
    }


    bool is_end() {
        return match( "e" ) or match( "end" );
    }


    bool is_cont() {
        return match( "c" ) or match( "cont" );
    }


    bool is_stack() {
        return match( "stack" );
    }


    bool is_abort() {
        return match( "abort" );
    }


    bool is_genesis() {
        return match( "genesis" );
    }


    bool is_top() {
        return match( "top" );
    }


    bool is_show() {
        return match( "show" );
    }


    bool is_break() {
        return match( "break" );
    }


    bool is_events() {
        return match( "events" );
    }


    bool is_status() {
        return match( "status" );
    }


    bool is_help() {
        return match( "?" ) or match( "help" );
    }


    bool is_quit() {
        return match( "q" ) or match( "quit" );
    }


    bool is_plus() {
        return match( "+" );
    }


    bool is_minus() {
        return match( "-" );
    }


    bool is_smi( Oop *addr );

    bool is_table_entry( Oop *addr );

    bool is_object_search( Oop *addr );

    bool is_name( Oop *addr );

    bool is_symbol( Oop *addr );

    bool is_unary();

    bool is_binary();

    bool is_keyword();
};


static const char *seps = " \t\n";


void TokenStream::tokenize( char *str ) {
    char *token = strtok( str, seps );
    while ( token not_eq nullptr ) {
        tokens->push( token );
        token = strtok( nullptr, seps );
    }
}


bool TokenStream::is_smi( Oop *addr ) {
    std::int32_t  value;
    std::uint32_t length;

    if ( sscanf( current(), "%d%u", &value, &length ) == 1 and strlen( current() ) == length ) {
        *addr = smiOopFromValue( value );
        return true;
    }
    return false;
}


bool TokenStream::is_table_entry( Oop *addr ) {
    std::int32_t  value;
    std::uint32_t length;
    if ( sscanf( current(), "!%d%u", &value, &length ) == 1 and strlen( current() ) == length ) {
        if ( not ObjectIDTable::is_index_ok( value ) ) {
            SPDLOG_INFO( "Could not find index {} in object table.", value );
            return true;
        }
        *addr = ObjectIDTable::at( value );
        return true;
    }
    return false;
}


bool TokenStream::is_object_search( Oop *addr ) {
    std::int32_t  address;
    Oop           obj;
    std::uint32_t length;

    if ( sscanf( current(), "0x%x%n", &address, &length ) == 1 && strlen( current() ) == length ) {
        obj = Oop( Universe::object_start( (Oop *) address ) );
        if ( obj ) {
            *addr = obj;
            return true;
        }
    }
    return false;
}


bool TokenStream::is_name( Oop *addr ) {
    char          name[200];
    Oop           obj;
    std::uint32_t length;
    if ( sscanf( current(), "%[a-zA-Z]%u", name, &length ) == 1 and strlen( current() ) == length ) {
        obj = Universe::find_global( name );
        if ( obj ) {
            *addr = obj;
            return true;
        }
    }
    return false;
}


bool TokenStream::is_symbol( Oop *addr ) {
    char          name[200];
    std::uint32_t length;
    if ( sscanf( current(), "#%[a-zA-Z0-9_]%u", name, &length ) == 1 and strlen( current() ) == length ) {
        *addr = oopFactory::new_symbol( name );
        return true;
    }
    return false;
}


bool TokenStream::is_unary() {
    char          name[40];
    std::uint32_t length;
    return sscanf( current(), "%[a-zA-Z]%u", name, &length ) == 1 and strlen( current() ) == length;
}


bool TokenStream::is_binary() {
    return not is_unary() and not is_keyword();
}


bool TokenStream::is_keyword() {
    char          name[40];
    std::uint32_t length;
    return sscanf( current(), "%[a-zA-Z]:%u", name, &length ) == 1 and strlen( current() ) == length;
}


bool evaluator::get_oop( TokenStream *stream, Oop *addr ) {

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
    SPDLOG_INFO( "Error: could not Oop'ify[{}]", stream->current() );
    return false;
}


bool validate_lookup( Oop receiver, SymbolOop selector ) {
    LookupKey key( receiver->klass(), selector );
    if ( LookupCache::lookup( &key ).is_empty() ) {
        SPDLOG_INFO( "Lookup error" );
        key.print_on( _console );
        _console->cr();
        return false;
    }
    return true;
}


void evaluator::eval_message( TokenStream *stream ) {
    Oop       receiver;
    Oop       result = nilObject;
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
        char         name[100];
        Oop          arguments[10];
        std::int32_t nofArgs = 0;
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


void evaluator::top_command( TokenStream *stream ) {
    std::int32_t number_of_frames_to_show = 10;
    stream->advance();
    if ( not stream->eos() ) {
        Oop value;
        if ( stream->is_smi( &value ) ) {
            number_of_frames_to_show = SMIOop( value )->value();
        }
        stream->advance();
        if ( not stream->eos() ) {
            SPDLOG_INFO( "warning: garbage at end" );
        }
    }
    DeltaProcess::active()->trace_top( 1, number_of_frames_to_show );
}


void evaluator::change_debug_flag( TokenStream *stream, bool value ) {
    stream->advance();
    if ( not stream->eos() ) {
        stream->current();
        bool r = value;
        if ( not debugFlags::boolAtPut( stream->current(), &r ) ) {
            SPDLOG_INFO( "boolean flag %s not found", stream->current() );
        }
        stream->advance();
        if ( not stream->eos() ) {
            SPDLOG_INFO( "warning: garbage at end" );
        }
    } else {
        SPDLOG_INFO( "boolean flag expected" );
    }
}


void evaluator::show_command( TokenStream *stream ) {
    std::int32_t start_frame              = 1;
    std::int32_t number_of_frames_to_show = 1;

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
                SPDLOG_INFO( "warning: garbage at end" );
            }
        }
    }
    DeltaProcess::active()->trace_top( start_frame, number_of_frames_to_show );
}


bool evaluator::process_line( const char *line ) {

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
            st_fatal( "evaluator break" );
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
                SPDLOG_INFO( "You cannot abort in the scheduler" );
                SPDLOG_INFO( "Try another command" );
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
                SPDLOG_INFO( "warning: garbage at end" );
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
    SPDLOG_INFO( "Use '?' for help ('c' to continue)" );
}


class ProcessStatusClosure : public ProcessClosure {
private:
    std::int32_t index;
public:
    ProcessStatusClosure() : index{ 1 } {
    }


    void do_process( DeltaProcess *p ) {
        SPDLOG_INFO( "{:d}:{}", index++, DeltaProcess::active() == p ? "*" : " " );
        p->print();
    }
};


void evaluator::print_status() {
    SPDLOG_INFO( "Processes:" );
    ProcessStatusClosure iter;
    Processes::process_iterate( &iter );
}


void evaluator::print_help() {
    _console->cr();
    SPDLOG_INFO( "<command>  ::= 'q'     | 'quit'    -> quits the system" );
    SPDLOG_INFO( "             | 's'     | 'step'    -> single step byte code" );
    SPDLOG_INFO( "             | 'n'     | 'next'    -> single step statement" );
    SPDLOG_INFO( "             | 'e'     | 'end'     -> single step to end of method" );
    SPDLOG_INFO( "             | 'c'     | 'cont'    -> continue execution" );
    SPDLOG_INFO( "                       | 'abort'   -> aborts the current process" );
    SPDLOG_INFO( "                       | 'genesis' -> aborts all processes and restarts the scheduler" );
    SPDLOG_INFO( "                       | 'break'   -> provokes fatal() to get into C++ debugger" );
    SPDLOG_INFO( "                       | 'events'  -> prints the event log" );
    SPDLOG_INFO( "                       | 'stack'   -> prints the stack of current process" );
    SPDLOG_INFO( "                       | 'status'  -> prints the status all processes" );
    SPDLOG_INFO( "                       | 'top' <n> -> prints the top of current process" );
    SPDLOG_INFO( "                       | 'show' <s> <n> -> prints some activation" );
    SPDLOG_INFO( "             | '?'     | 'help'    -> prints this help" );
    SPDLOG_INFO( "             | '^' <expr>          -> evaluates the expression" );
    SPDLOG_INFO( "             | '-' name            -> turns off debug flag" );
    SPDLOG_INFO( "             | '+' name            -> turns on debug flag" );
    SPDLOG_INFO( "             | <object>            -> prints this object" );
    _console->cr();
    SPDLOG_INFO( "<expr>     ::= <unary>  | <binary>  | <keyword>" );
    SPDLOG_INFO( "<object>   ::= <number>            -> smi_t(number)" );
    SPDLOG_INFO( "             | !<number>           -> objectTable[number]" );
    SPDLOG_INFO( "             | 0x<hex_number>      -> object_start(number)" );
    SPDLOG_INFO( "             | name                -> Smalltalk at: #name" );
    SPDLOG_INFO( "             | #name               -> new_symbol(name)" );
    _console->cr();
}
