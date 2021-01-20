//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitives/system_primitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/memory/Reflection.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/system/os.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/memory/SnapshotDescriptor.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/system/dll.hpp"
#include "vm/runtime/FlatProfiler.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/SlidingSystemAverage.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"


TRACE_FUNC( TraceSystemPrims, "system" )


std::size_t SystemPrimitives::number_of_calls;


PRIM_DECL_5( SystemPrimitives::createNamedInvocation, Oop mixin, Oop name, Oop primary, Oop superclass, Oop format ) {
    PROLOGUE_5( "createNamedInvocation", mixin, primary, name, superclass, format )

    // Check argument types
    if ( not mixin->is_mixin() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not name->is_symbol() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    if ( not( primary == trueObj or primary == falseObj ) )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );

    if ( not superclass->is_klass() )
        return markSymbol( vmSymbols::fourth_argument_has_wrong_type() );

    if ( not format->is_symbol() )
        return markSymbol( vmSymbols::fifth_argument_has_wrong_type() );

    Klass::Format f = Klass::format_from_symbol( SymbolOop( format ) );
    if ( f == Klass::Format::no_klass ) {
        return markSymbol( vmSymbols::argument_is_invalid() );
    }

    BlockScavenge bs;

    // Create the new klass
    KlassOop new_klass = KlassOop( superclass )->klass_part()->create_subclass( MixinOop( mixin ), f );

    if ( new_klass == nullptr ) {
        // Create more detailed error message
        return markSymbol( vmSymbols::argument_is_invalid() );
    }

    // Set the primary invocation if needed.
    if ( primary == trueObj ) {
        MixinOop( mixin )->set_primary_invocation( new_klass );
        MixinOop( mixin )->class_mixin()->set_primary_invocation( new_klass->klass() );
        MixinOop( mixin )->set_installed( trueObj );
        MixinOop( mixin )->class_mixin()->set_installed( trueObj );
    }

    // Make sure mixin->classMixin is present

    // Add the global
    Universe::add_global( oopFactory::new_association( SymbolOop( name ), new_klass, true ) );
    return new_klass;
}


PRIM_DECL_3( SystemPrimitives::createInvocation, Oop mixin, Oop superclass, Oop format ) {
    PROLOGUE_3( "createInvocation", mixin, superclass, format )

    // Check argument types
    if ( not mixin->is_mixin() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not superclass->is_klass() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    if ( not format->is_symbol() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );

    BlockScavenge bs;

    Klass::Format f = Klass::format_from_symbol( SymbolOop( format ) );
    if ( f == Klass::Format::no_klass ) {
        return markSymbol( vmSymbols::argument_is_invalid() );
    }

    KlassOop new_klass = KlassOop( superclass )->klass_part()->create_subclass( MixinOop( mixin ), f );

    if ( new_klass == nullptr ) {
        // Create more detailed error message
        return markSymbol( vmSymbols::argument_is_invalid() );
    }

    MixinOop( mixin )->set_installed( trueObj );
    MixinOop( mixin )->class_mixin()->set_installed( trueObj );

    return new_klass;
}


PRIM_DECL_1( SystemPrimitives::applyChange, Oop change ) {
    PROLOGUE_1( "applyChange", change )

    // Check argument types
    if ( not change->is_objArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    return Reflection::apply_change( ObjectArrayOop( change ) );
}


PRIM_DECL_0( SystemPrimitives::canScavenge ) {
    PROLOGUE_0( "canScavenge" )

    return Universe::can_scavenge() ? trueObj : falseObj;
}


PRIM_DECL_1( SystemPrimitives::scavenge, Oop receiver ) {
    PROLOGUE_1( "scavenge", receiver )
    Oop         rec = receiver;
    VM_Scavenge op( &rec );
    // The operation takes place in the vmProcess
    VMProcess::execute( &op );
    return rec;
}


PRIM_DECL_0( SystemPrimitives::oopSize ) {
    PROLOGUE_0( "oopSize" )
    return smiOopFromValue( ::oopSize );
}


PRIM_DECL_1( SystemPrimitives::garbageGollect, Oop receiver ) {
    PROLOGUE_1( "garbageGollect", receiver );
    Oop               rec = receiver;
    VM_GarbageCollect op( &rec );
    // The operation takes place in the vmProcess
    VMProcess::execute( &op );
    return rec;
}


PRIM_DECL_1( SystemPrimitives::expandMemory, Oop sizeOop ) {
    PROLOGUE_1( "expandMemory", sizeOop );
    if ( not sizeOop->is_smi() )
        return markSymbol( vmSymbols::argument_has_wrong_type() );
    std::size_t size = SMIOop( sizeOop )->value();
    if ( size < 0 )
        return markSymbol( vmSymbols::argument_is_invalid() );
    Universe::old_gen.expand( size );
    return trueObj;
}


PRIM_DECL_1( SystemPrimitives::shrinkMemory, Oop sizeOop ) {
    PROLOGUE_1( "shrinkMemory", sizeOop );
    if ( not sizeOop->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( SMIOop( sizeOop )->value() < 0 or SMIOop( sizeOop )->value() > Universe::old_gen.free() )
        return markSymbol( vmSymbols::value_out_of_range() );
    Universe::old_gen.shrink( SMIOop( sizeOop )->value() );
    return trueObj;
}


extern "C" int expansion_count;
extern "C" void single_step_handler();


PRIM_DECL_0( SystemPrimitives::expansions ) {
    PROLOGUE_0( "expansions" )
    return smiOopFromValue( expansion_count );
}


PRIM_DECL_0( SystemPrimitives::breakpoint ) {
    PROLOGUE_0( "breakpoint" )
    {
        ResourceMark resourceMark;
        StubRoutines::setSingleStepHandler( &single_step_handler );
        DispatchTable::intercept_for_step( nullptr );
    }
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::vmbreakpoint ) {
    PROLOGUE_0( "vmbreakpoint" )
    os::breakpoint();
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::getLastError ) {
    PROLOGUE_0( "getLastError" )
    return smiOopFromValue( os::error_code() ); //%TODO% fix this to support errors > 30 bits in length
}


PRIM_DECL_0( SystemPrimitives::halt ) {
    PROLOGUE_0( "halt" )

    // I think this is obsolete, hlt is a privileged instruction, and Object>>halt uses
    // Processor stopWithError: ProcessHaltError new.
    // removed because inline assembly isn't portable -Marc 04/07

    Unimplemented();
    return markSymbol( vmSymbols::not_yet_implemented() );

    //  __asm hlt
    //  return trueObj;
}


static Oop fake_time() {
    static std::size_t time = 0;
    return oopFactory::new_double( (double) time++ );
}


PRIM_DECL_0( SystemPrimitives::userTime ) {
    PROLOGUE_0( "userTime" )
    if ( UseTimers ) {
        os::updateTimes();
        return oopFactory::new_double( os::userTime() );
    } else {
        return fake_time();
    }
}


PRIM_DECL_0( SystemPrimitives::systemTime ) {
    PROLOGUE_0( "systemTime" )
    if ( UseTimers ) {
        os::updateTimes();
        return oopFactory::new_double( os::systemTime() );
    } else {
        return fake_time();
    }
}


PRIM_DECL_0( SystemPrimitives::elapsedTime ) {
    PROLOGUE_0( "elapsedTime" )
    if ( UseTimers ) {
        return oopFactory::new_double( os::elapsedTime() );
    } else {
        return fake_time();
    }
}


PRIM_DECL_1( SystemPrimitives::writeSnapshot, Oop fileName ) {
    PROLOGUE_1( "writeSnapshot", fileName );
    SnapshotDescriptor sd;
    const char *name = "fisk.snap";
    sd.write_on( name );
    if ( sd.has_error() )
        return markSymbol( sd.error_symbol() );
    return fileName;
}


PRIM_DECL_1( SystemPrimitives::globalAssociationKey, Oop receiver ) {
    PROLOGUE_1( "globalAssociationKey", receiver );
    st_assert( receiver->is_association(), "receiver must be association" );
    return AssociationOop( receiver )->key();
}


PRIM_DECL_2( SystemPrimitives::globalAssociationSetKey, Oop receiver, Oop key ) {
    PROLOGUE_2( "globalAssociationSetKey", receiver, key );
    st_assert( receiver->is_association(), "receiver must be association" );
    if ( not key->is_symbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    AssociationOop( receiver )->set_key( SymbolOop( key ) );
    return receiver;
}


PRIM_DECL_1( SystemPrimitives::globalAssociationValue, Oop receiver ) {
    PROLOGUE_1( "globalAssociationValue", receiver );
    st_assert( receiver->is_association(), "receiver must be association" );
    return AssociationOop( receiver )->value();
}


PRIM_DECL_2( SystemPrimitives::globalAssociationSetValue, Oop receiver, Oop value ) {
    PROLOGUE_2( "globalAssociationSetValue", receiver, value );
    st_assert( receiver->is_association(), "receiver must be association" );
    AssociationOop( receiver )->set_value( value );
    return receiver;
}


PRIM_DECL_1( SystemPrimitives::globalAssociationIsConstant, Oop receiver ) {
    PROLOGUE_1( "globalAssociationIsConstant", receiver );
    st_assert( receiver->is_association(), "receiver must be association" );
    return AssociationOop( receiver )->is_constant() ? trueObj : falseObj;
}


PRIM_DECL_2( SystemPrimitives::globalAssociationSetConstant, Oop receiver, Oop value ) {
    PROLOGUE_2( "globalAssociationSetConstant", receiver, value );
    st_assert( receiver->is_association(), "receiver must be association" );
    Oop old_value = AssociationOop( receiver )->is_constant() ? trueObj : falseObj;

    if ( value == trueObj )
        AssociationOop( receiver )->set_is_constant( true );
    else if ( value == falseObj )
        AssociationOop( receiver )->set_is_constant( false );
    else
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return old_value;
}


PRIM_DECL_1( SystemPrimitives::smalltalk_at, Oop index ) {
    PROLOGUE_1( "smalltalk_at", index );
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not Universe::systemDictionaryObj()->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    return Universe::systemDictionaryObj()->obj_at( SMIOop( index )->value() );
}


PRIM_DECL_2( SystemPrimitives::smalltalk_at_put, Oop key, Oop value ) {
    PROLOGUE_2( "smalltalk_at_put", key, value );

    BlockScavenge  bs;
    AssociationOop assoc = oopFactory::new_association( SymbolOop( key ), value, false );
    Universe::add_global( assoc );
    return assoc;
}


PRIM_DECL_1( SystemPrimitives::smalltalk_remove_at, Oop index ) {
    PROLOGUE_1( "smalltalk_remove_at", index );

    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not Universe::systemDictionaryObj()->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    BlockScavenge bs;

    AssociationOop assoc = AssociationOop( Universe::systemDictionaryObj()->obj_at( SMIOop( index )->value() ) );
    Universe::remove_global_at( SMIOop( index )->value() );
    return assoc;
}


PRIM_DECL_0( SystemPrimitives::smalltalk_size ) {
    PROLOGUE_0( "smalltalk_size" );
    return smiOopFromValue( Universe::systemDictionaryObj()->length() );
}


PRIM_DECL_0( SystemPrimitives::smalltalk_array ) {
    PROLOGUE_0( "smalltalk_array" );
    return Universe::systemDictionaryObj();
}


PRIM_DECL_0( SystemPrimitives::quit ) {
    PROLOGUE_0( "quit" );
    exit( EXIT_SUCCESS );
    return badOop;
}


PRIM_DECL_0( SystemPrimitives::printPrimitiveTable ) {
    PROLOGUE_0( "printPrimitiveTable" );
    Primitives::print_table();
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::print_memory ) {
    PROLOGUE_0( "print_memory" );
    Universe::print();
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::print_zone ) {
    PROLOGUE_0( "print_zone" );
    Universe::code->print();
    return trueObj;
}


PRIM_DECL_1( SystemPrimitives::defWindowProc, Oop resultProxy ) {
    PROLOGUE_1( "defWindowProc", resultProxy );
    if ( not resultProxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    _console->print_cr( "Please use the new Platform DLLLookup system to retrieve DefWindowProcA" );
    dll_func_ptr_t func = DLLs::lookup( oopFactory::new_symbol( "user" ), oopFactory::new_symbol( "DefWindowProcA" ) );
    ProxyOop( resultProxy )->set_pointer( (void *) func );
    return resultProxy;
}


PRIM_DECL_1( SystemPrimitives::windowsHInstance, Oop resultProxy ) {
    PROLOGUE_1( "windowsHInstance", resultProxy );
    if ( not resultProxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    ProxyOop( resultProxy )->set_pointer( os::get_hInstance() );
    return resultProxy;
}


PRIM_DECL_1( SystemPrimitives::windowsHPrevInstance, Oop resultProxy ) {
    PROLOGUE_1( "windowsHPrevInstance", resultProxy );
    if ( not resultProxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    ProxyOop( resultProxy )->set_pointer( os::get_prevInstance() );
    return resultProxy;
}


PRIM_DECL_0( SystemPrimitives::windowsNCmdShow ) {
    PROLOGUE_0( "windowsNCmdShow" );
    return smiOopFromValue( os::get_nCmdShow() );
}


PRIM_DECL_1( SystemPrimitives::characterFor, Oop value ) {
    PROLOGUE_1( "characterFor", value );

    // check value type
    if ( not value->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( (std::uint32_t) SMIOop( value )->value() < 256 )
        // return the n+1'th element in asciiCharacter
        return Universe::asciiCharacters()->obj_at( SMIOop( value )->value() + 1 );
    else
        return markSymbol( vmSymbols::out_of_bounds() );
}


PRIM_DECL_0( SystemPrimitives::traceStack ) {
    PROLOGUE_0( "traceStack" );
    DeltaProcess::active()->trace_stack();
    return trueObj;
}

// Flat Profiler Primitives

PRIM_DECL_0( SystemPrimitives::flat_profiler_reset ) {
    PROLOGUE_0( "flat_profiler_reset" );
    FlatProfiler::reset();
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::flat_profiler_process ) {
    PROLOGUE_0( "flat_profiler_process" );
    DeltaProcess *proc = FlatProfiler::process();
    return proc == nullptr ? nilObj : proc->processObj();
}


PRIM_DECL_1( SystemPrimitives::flat_profiler_engage, Oop process ) {
    PROLOGUE_1( "flat_profiler_engage", process );

    // check value type
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    FlatProfiler::engage( ProcessOop( process )->process() );
    return process;
}


PRIM_DECL_0( SystemPrimitives::flat_profiler_disengage ) {
    PROLOGUE_0( "flat_profiler_disengage" );
    DeltaProcess *proc = FlatProfiler::disengage();
    return proc == nullptr ? nilObj : proc->processObj();
}


PRIM_DECL_0( SystemPrimitives::flat_profiler_print ) {
    PROLOGUE_0( "flat_profiler_print" );
    FlatProfiler::print( 0 );
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::notificationQueueGet ) {
    PROLOGUE_0( "notificationQueueGet" );
    if ( NotificationQueue::is_empty() )
        return markSymbol( vmSymbols::empty_queue() );
    return NotificationQueue::get();
}


PRIM_DECL_1( SystemPrimitives::notificationQueuePut, Oop value ) {
    PROLOGUE_1( "notificationQueuePut", value );
    NotificationQueue::put( value );
    return value;
}


PRIM_DECL_1( SystemPrimitives::hadNearDeathExperience, Oop value ) {
    PROLOGUE_1( "hadNearDeathExperience", value );
    return ( value->is_mem() and MemOop( value )->mark()->is_near_death() ) ? trueObj : falseObj;
}


PRIM_DECL_2( SystemPrimitives::dll_setup, Oop receiver, Oop selector ) {
    PROLOGUE_2( "dll_setup", receiver, selector );

    if ( not selector->is_symbol() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    if ( SymbolOop( selector )->number_of_arguments() not_eq 2 )
        return markSymbol( vmSymbols::failed() );

    Universe::set_dll_lookup( receiver, SymbolOop( selector ) );
    return receiver;
}


PRIM_DECL_3( SystemPrimitives::dll_lookup, Oop name, Oop library, Oop result ) {
    PROLOGUE_3( "dll_lookup", name, library, result );

    if ( not name->is_symbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not library->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );

    dll_func_ptr_t res = DLLs::lookup( SymbolOop( name ), (DLL *) ProxyOop( library )->get_pointer() );
    if ( res ) {
        ProxyOop( result )->set_pointer( (void *) res );
        return result;
    } else {
        return markSymbol( vmSymbols::not_found() );
    }
}


PRIM_DECL_2( SystemPrimitives::dll_load, Oop name, Oop library ) {
    PROLOGUE_2( "dll_load", name, library );

    if ( not name->is_symbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not library->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    DLL *res = DLLs::load( SymbolOop( name ) );
    if ( res ) {
        ProxyOop( library )->set_pointer( res );
        return library;
    } else {
        return markSymbol( vmSymbols::not_found() );
    }
}


PRIM_DECL_1( SystemPrimitives::dll_unload, Oop library ) {
    PROLOGUE_1( "dll_unload", library );

    if ( not library->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    return DLLs::unload( (DLL *) ProxyOop( library )->get_pointer() ) ? library : markSymbol( vmSymbols::failed() );
}

// Inlining Database

PRIM_DECL_0( SystemPrimitives::inlining_database_directory ) {
    PROLOGUE_0( "inlining_database_directory" );
    return oopFactory::new_symbol( InliningDatabase::directory() );
}


PRIM_DECL_1( SystemPrimitives::inlining_database_set_directory, Oop name ) {
    PROLOGUE_1( "inlining_database_set_directory", name );

    // Check type on argument
    if ( not name->is_byteArray() and not name->is_doubleByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ResourceMark resourceMark;

    int len = name->is_byteArray() ? ByteArrayOop( name )->length() : DoubleByteArrayOop( name )->length();
    char *str = new_c_heap_array<char>( len + 1 );
    name->is_byteArray() ? ByteArrayOop( name )->copy_null_terminated( str, len + 1 ) : DoubleByteArrayOop( name )->copy_null_terminated( str, len + 1 );
    // Potential memory leak, but this is temporary
    InliningDatabase::set_directory( str );
    return trueObj;
}


PRIM_DECL_1( SystemPrimitives::inlining_database_file_out_class, Oop receiver_class ) {
    PROLOGUE_1( "inlining_database_file_out_class", receiver_class );

    // Check type on argument
    if ( not receiver_class->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // File out the class
    return smiOopFromValue( InliningDatabase::file_out( KlassOop( receiver_class ) ) );
}


PRIM_DECL_0( SystemPrimitives::inlining_database_file_out_all ) {
    PROLOGUE_0( "inlining_database_file_out_all" );

    // File out all nativeMethods
    return smiOopFromValue( InliningDatabase::file_out_all() );
}


PRIM_DECL_1( SystemPrimitives::inlining_database_compile, Oop file_name ) {
    PROLOGUE_1( "inlining_database_compile", file_name );

    // Check type on argument
    if ( not file_name->is_byteArray() and not file_name->is_doubleByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ResourceMark resourceMark;

    int len = file_name->is_byteArray() ? ByteArrayOop( file_name )->length() : DoubleByteArrayOop( file_name )->length();
    char *str = new_resource_array<char>( len + 1 );
    file_name->is_byteArray() ? ByteArrayOop( file_name )->copy_null_terminated( str, len + 1 ) : DoubleByteArrayOop( file_name )->copy_null_terminated( str, len + 1 );

    RecompilationScope *rs = InliningDatabase::file_in( str );
    if ( rs ) {
        // Remove old NativeMethod if present
        NativeMethod *old_nm = Universe::code->lookup( rs->key() );
        if ( old_nm ) {
            old_nm->makeZombie( false );
        }

        VM_OptimizeRScope op( rs );
        VMProcess::execute( &op );

        if ( TraceInliningDatabase ) {
            _console->print_cr( "compiling {%s} completed", str );
            _console->print_cr( "[Database]" );
            rs->print_inlining_database_on( _console, nullptr, -1, 0 );
            _console->print_cr( "[Compiled method]" );
            op.result()->print_inlining_database_on( _console );
        }
    } else {
        if ( TraceInliningDatabase ) {
            _console->print_cr( "compiling {%s} failed", str );
        }
    }
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::inlining_database_compile_next ) {
    PROLOGUE_0( "inlining_database_compile_next" );
    if ( not UseInliningDatabase ) {
        return falseObj;
    }

    bool_t end_of_table;
    RecompilationScope *rs = InliningDatabase::select_and_remove( &end_of_table );
    if ( rs ) {
        VM_OptimizeRScope op( rs );
        VMProcess::execute( &op );
        if ( TraceInliningDatabase ) {
            _console->print( "Compiling " );
            op.result()->_lookupKey.print_on( _console );
            _console->print_cr( " in background = 0x%lx", op.result() );
        }
    }
    return end_of_table ? falseObj : trueObj;
}


PRIM_DECL_1( SystemPrimitives::inlining_database_mangle, Oop name ) {
    PROLOGUE_1( "inlining_database_mangle", name );

    // Check type on argument
    if ( not name->is_byteArray() and not name->is_doubleByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ResourceMark resourceMark;

    int len = name->is_byteArray() ? ByteArrayOop( name )->length() : DoubleByteArrayOop( name )->length();
    char *str = new_resource_array<char>( len + 1 );
    name->is_byteArray() ? ByteArrayOop( name )->copy_null_terminated( str, len + 1 ) : DoubleByteArrayOop( name )->copy_null_terminated( str, len + 1 );
    return oopFactory::new_byteArray( InliningDatabase::mangle_name( str ) );
}


PRIM_DECL_1( SystemPrimitives::inlining_database_demangle, Oop name ) {
    PROLOGUE_1( "inlining_database_demangle", name );
    // Check type on argument
    if ( not name->is_byteArray() and not name->is_doubleByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ResourceMark resourceMark;

    int len = name->is_byteArray() ? ByteArrayOop( name )->length() : DoubleByteArrayOop( name )->length();
    char *str = new_resource_array<char>( len + 1 );
    name->is_byteArray() ? ByteArrayOop( name )->copy_null_terminated( str, len + 1 ) : DoubleByteArrayOop( name )->copy_null_terminated( str, len + 1 );
    return oopFactory::new_byteArray( InliningDatabase::unmangle_name( str ) );
}


PRIM_DECL_2( SystemPrimitives::inlining_database_add_entry, Oop receiver_class, Oop method_selector ) {
    PROLOGUE_2( "inlining_database_add_entry", receiver_class, method_selector );

    // Check type of argument
    if ( not receiver_class->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Check type of argument
    if ( not method_selector->is_symbol() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    LookupKey key( KlassOop( receiver_class ), method_selector );
    InliningDatabase::add_lookup_entry( &key );
    return receiver_class;
}


PRIM_DECL_0( SystemPrimitives::sliding_system_average ) {
    PROLOGUE_0( "system_sliding_average" );

    if ( not UseSlidingSystemAverage )
        return markSymbol( vmSymbols::not_active() );

//    std::uint32_t * array = SlidingSystemAverage::update();
    std::array<std::uint32_t, SlidingSystemAverage::number_of_cases> _array = SlidingSystemAverage::update();

    ObjectArrayOop result = oopFactory::new_objArray( SlidingSystemAverage::number_of_cases - 1 );

    for ( std::size_t i = 1; i < SlidingSystemAverage::number_of_cases; i++ ) {
        result->obj_at_put( i, smiOopFromValue( _array[ i ] ) );
    }

    return result;
}

// Enumeration primitives
// - it is important to exclude contextOops since they should be invisible to the Smalltalk level.

class InstancesOfClosure : public ObjectClosure {

public:
    InstancesOfClosure( KlassOop target, int limit ) {
        this->_result = new GrowableArray<Oop>( 100 );
        this->_target = target;
        this->_limit  = limit;
    }


    int      _limit;
    KlassOop _target;
    GrowableArray<Oop> *_result;


    void do_object( MemOop obj ) {
        if ( obj->klass() == _target ) {
            if ( _result->length() < _limit and not obj->is_context() ) {
                _result->append( obj );
            }
        }
    }
};


PRIM_DECL_2( SystemPrimitives::instances_of, Oop klass, Oop limit ) {
    PROLOGUE_2( "instances_of", klass, limit );

    // Check type of argument
    if ( not klass->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not limit->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;
    ResourceMark  rm;

    InstancesOfClosure blk( KlassOop( klass ), SMIOop( limit )->value() );
    Universe::object_iterate( &blk );

    int            length = blk._result->length();
    ObjectArrayOop result = oopFactory::new_objArray( length );

    for ( std::size_t i = 1; i <= length; i++ ) {
        result->obj_at_put( i, blk._result->at( i - 1 ) );
    }

    return result;
}


class ConvertClosure : public OopClosure {

private:
    void do_oop( Oop *o ) {
        Reflection::convert( o );
    }
};

class HasReferenceClosure : public OopClosure {

private:
    Oop _target;

public:
    HasReferenceClosure( Oop target ) {
        _target = target;
        _result = false;
    }


    void do_oop( Oop *o ) {
        if ( *o == _target )
            _result = true;
    }


    bool_t _result;
};

class ReferencesToClosure : public ObjectClosure {

public:
    ReferencesToClosure( Oop target, int limit ) {
        _result = new GrowableArray<Oop>( 100 );
        _target = target;
        _limit  = limit;
    }


    int _limit;
    Oop _target;
    GrowableArray<Oop> *_result;


    bool_t has_reference( MemOop obj ) {
        HasReferenceClosure blk( _target );
        obj->oop_iterate( &blk );
        return blk._result;
    }


    void do_object( MemOop obj ) {
        if ( has_reference( obj ) ) {
            if ( _result->length() < _limit and not obj->is_context() ) {
                _result->append( obj );
            }
        }
    }
};


PRIM_DECL_2( SystemPrimitives::references_to, Oop obj, Oop limit ) {
    PROLOGUE_2( "references_to", obj, limit );

    // Check type of argument
    if ( not limit->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;
    ResourceMark  rm;

    ReferencesToClosure blk( obj, SMIOop( limit )->value() );
    Universe::object_iterate( &blk );

    int            length = blk._result->length();
    ObjectArrayOop result = oopFactory::new_objArray( length );
    for ( int      index  = 1; index <= length; index++ ) {
        result->obj_at_put( index, blk._result->at( index - 1 ) );
    }
    return result;
}


class HasInstanceReferenceClosure : public OopClosure {

private:
    KlassOop _target;

public:
    HasInstanceReferenceClosure( KlassOop target ) {
        this->_target = target;
        this->_result = false;
    }


    void do_oop( Oop *o ) {
        if ( ( *o )->klass() == _target )
            _result = true;
    }


    bool_t _result;
};

class ReferencesToInstancesOfClosure : public ObjectClosure {

public:
    ReferencesToInstancesOfClosure( KlassOop target, int limit ) {
        this->_result = new GrowableArray<Oop>( 100 );
        this->_target = target;
        this->_limit  = limit;
    }


    int      _limit;
    KlassOop _target;
    GrowableArray<Oop> *_result;


    bool_t has_reference( MemOop obj ) {
        HasInstanceReferenceClosure blk( _target );
        obj->oop_iterate( &blk );
        return blk._result;
    }


    void do_object( MemOop obj ) {
        if ( has_reference( obj ) ) {
            if ( _result->length() < _limit and not obj->is_context() ) {
                _result->append( obj );
            }
        }
    }
};


PRIM_DECL_2( SystemPrimitives::references_to_instances_of, Oop klass, Oop limit ) {
    PROLOGUE_2( "references_to_instances_of", klass, limit );

    // Check type of argument
    if ( not klass->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( not limit->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;
    ResourceMark  rm;

    ReferencesToInstancesOfClosure blk( KlassOop( klass ), SMIOop( limit )->value() );
    Universe::object_iterate( &blk );

    int            length = blk._result->length();
    ObjectArrayOop result = oopFactory::new_objArray( length );

    for ( std::size_t index = 1; index <= length; index++ ) {
        result->obj_at_put( index, blk._result->at( index - 1 ) );
    }

    return result;
}


class AllObjectsClosure : public ObjectClosure {
public:
    AllObjectsClosure( int limit ) {
        this->_result = new GrowableArray<Oop>( 20000 );
        this->_limit  = limit;
    }


    int _limit;
    GrowableArray<Oop> *_result;


    void do_object( MemOop obj ) {
        if ( _result->length() < _limit and not obj->is_context() ) {
            _result->append( obj );
        }
    }
};


PRIM_DECL_1( SystemPrimitives::all_objects, Oop limit ) {
    PROLOGUE_1( "all_objects", limit );

    // Check type of argument
    if ( not limit->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;
    ResourceMark  rm;

    AllObjectsClosure blk( SMIOop( limit )->value() );
    Universe::object_iterate( &blk );

    int            length = blk._result->length();
    ObjectArrayOop result = oopFactory::new_objArray( length );
    for ( int      index  = 1; index <= length; index++ ) {
        result->obj_at_put( index, blk._result->at( index - 1 ) );
    }
    return result;
}


PRIM_DECL_0( SystemPrimitives::flush_code_cache ) {
    PROLOGUE_0( "flush_code_cache" );
    Universe::code->flush();
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::flush_dead_code ) {
    PROLOGUE_0( "flush_dead_code" );
    Universe::code->flushZombies();
    return trueObj;
}


PRIM_DECL_0( SystemPrimitives::command_line_args ) {
    PROLOGUE_0( "command_line_args" );

    int argc = os::argc();
    char **argv = os::argv();

    ObjectArrayOop result = oopFactory::new_objArray( argc );
    result->set_length( argc );
    for ( std::size_t i = 0; i < argc; i++ ) {
        ByteArrayOop arg = oopFactory::new_byteArray( argv[ i ] );
        result->obj_at_put( i + 1, arg );
    }
    return result;
}


PRIM_DECL_0( SystemPrimitives::current_thread_id ) {
    PROLOGUE_0( "current_thread_id" );
    return smiOopFromValue( os::current_thread_id() );
}


PRIM_DECL_0( SystemPrimitives::object_memory_size ) {
    PROLOGUE_0( "object_memory_size" );
    return oopFactory::new_double( double( Universe::old_gen.used() ) / Universe::old_gen.capacity() );
}


PRIM_DECL_0( SystemPrimitives::freeSpace ) {
    PROLOGUE_0( "freeSpace" );
    return smiOopFromValue( Universe::old_gen.free() );
}


PRIM_DECL_0( SystemPrimitives::nurseryFreeSpace ) {
    PROLOGUE_0( "nurseryFreeSpace" );
    return smiOopFromValue( Universe::new_gen.eden()->free() );
}


PRIM_DECL_1( SystemPrimitives::alienMalloc, Oop size ) {
    PROLOGUE_0( "alienMalloc" );

    if ( not size->is_smi() )
        return markSymbol( vmSymbols::argument_has_wrong_type() );

    int theSize = SMIOop( size )->value();
    if ( theSize <= 0 )
        return markSymbol( vmSymbols::argument_is_invalid() );

    return smiOopFromValue( (int) malloc( theSize ) );
}


PRIM_DECL_1( SystemPrimitives::alienCalloc, Oop size ) {
    PROLOGUE_0( "alienCalloc" );
    if ( not size->is_smi() )
        return markSymbol( vmSymbols::argument_has_wrong_type() );

    int theSize = SMIOop( size )->value();
    if ( theSize <= 0 )
        return markSymbol( vmSymbols::argument_is_invalid() );

    return smiOopFromValue( (int) calloc( SMIOop( size )->value(), 1 ) );
}


PRIM_DECL_1( SystemPrimitives::alienFree, Oop address ) {
    PROLOGUE_0( "alienFree" );

    if ( not address->is_smi() and not( address->is_byteArray() and address->klass() == Universe::find_global( "LargeInteger" ) ) )
        return markSymbol( vmSymbols::argument_has_wrong_type() );

    if ( address->is_smi() ) {
        if ( SMIOop( address )->value() == 0 )
            return markSymbol( vmSymbols::argument_is_invalid() );

        free( (void *) SMIOop( address )->value() );

    } else { // LargeInteger
        BlockScavenge bs;
        Integer *largeAddress = &ByteArrayOop( address )->number();
        bool_t ok;
        int    intAddress     = largeAddress->as_int( ok );
        if ( intAddress == 0 or not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
        free( (void *) intAddress );
    }

    return trueObj;
}
