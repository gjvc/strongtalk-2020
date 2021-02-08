//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/memory/AgeTable.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/memory/PrintObjectClosure.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/memory/WaterMark.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"


bool         garbageCollectionInProgress = false;
bool         scavengeRequired            = false;
bool         bootstrappingInProgress     = true;
std::int32_t BlockScavenge::counter      = 0;
bool          Universe::_scavenge_blocked = false;
NewGeneration Universe::new_gen;
OldGeneration Universe::old_gen;
SymbolTable   *Universe::symbol_table;
RememberedSet *Universe::remembered_set;
AgeTable      *Universe::age_table;
Zone          *Universe::code;
SpaceSizes    Universe::current_sizes;
std::int32_t   Universe::tenuring_threshold;
std::int32_t   Universe::scavengeCount;

// Classes
KlassOop smiKlassObject     = KlassOop( badOop );
KlassOop contextKlassObject = KlassOop( badOop );
KlassOop doubleKlassObject  = KlassOop( badOop );
KlassOop Universe::_memOopKlassObject    = KlassOop( badOop );
KlassOop Universe::_objArrayKlassObject  = KlassOop( badOop );
KlassOop Universe::_byteArrayKlassObject = KlassOop( badOop );
KlassOop symbolKlassObject = KlassOop( badOop );
KlassOop Universe::_associationKlassObject = KlassOop( badOop );
KlassOop zeroArgumentBlockKlassObject  = KlassOop( badOop );
KlassOop oneArgumentBlockKlassObject   = KlassOop( badOop );
KlassOop twoArgumentBlockKlassObject   = KlassOop( badOop );
KlassOop threeArgumentBlockKlassObject = KlassOop( badOop );
KlassOop fourArgumentBlockKlassObject  = KlassOop( badOop );
KlassOop fiveArgumentBlockKlassObject  = KlassOop( badOop );
KlassOop sixArgumentBlockKlassObject   = KlassOop( badOop );
KlassOop sevenArgumentBlockKlassObject = KlassOop( badOop );
KlassOop eightArgumentBlockKlassObject = KlassOop( badOop );
KlassOop nineArgumentBlockKlassObject  = KlassOop( badOop );
KlassOop Universe::_methodKlassObject    = KlassOop( badOop );
KlassOop Universe::_characterKlassObject = KlassOop( badOop );
KlassOop doubleValueArrayKlassObject = KlassOop( badOop );
KlassOop Universe::_vframeKlassObject = KlassOop( badOop );

// Objects
Oop nilObject   = Oop( badOop );
Oop trueObject  = Oop( badOop );
Oop falseObject = Oop( badOop );
ObjectArrayOop Universe::_asciiCharacters        = ObjectArrayOop( badOop );
ObjectArrayOop Universe::_systemDictionaryObject = ObjectArrayOop( badOop );
ObjectArrayOop Universe::_objectIDTable          = ObjectArrayOop( badOop );
ObjectArrayOop Universe::_pic_free_list          = ObjectArrayOop( badOop );
Oop            Universe::_callBack_receiver      = Oop( badOop );
SymbolOop      Universe::_callBack_selector      = SymbolOop( badOop );
Oop            Universe::_dll_lookup_receiver    = Oop( badOop );
SymbolOop      Universe::_dll_lookup_selector    = SymbolOop( badOop );
MethodOop      Universe::_sweeper_method         = nullptr;


void Universe::genesis() {

    ResourceMark resourceMark;

    SPDLOG_INFO( "%vm-backend-implementation[{}]", UseNewBackend | TryNewBackend ? "new" : "old" );
    if ( UseNewBackend | TryNewBackend ) {
        SPDLOG_INFO( "%vm-backend-makeConformant[{}]", UseNewMakeConformant ? "yes" : "no" );
    }

    if ( not Interpreter::is_optimized() ) {
        Interpreter::print_code_status();
    }

    scavengeCount = 0;

    current_sizes.initialize();

    symbol_table = new SymbolTable;

    age_table = new AgeTable;

    // Reserve Space for object heap
    ReservedSpace rs( current_sizes._reserved_object_size );

    if ( not rs.is_reserved() ) {
        st_fatal( "could not reserve enough space for object heap" );
    }

    std::int32_t new_size = ReservedSpace::page_align_size( current_sizes._eden_size + 2 * current_sizes._surv_size );

    ReservedSpace new_rs = rs.first_part( new_size );
    ReservedSpace old_rs = rs.last_part( new_size );

    new_gen.initialize( new_rs, current_sizes._eden_size, current_sizes._surv_size );
    old_gen.initialize( old_rs, current_sizes._old_size );

    st_assert( new_gen._highBoundary <= old_gen._lowBoundary, "old Space allocated lower than new Space" );

    remembered_set = new RememberedSet; // uses _boundary's

    tenuring_threshold = AgeTable::table_size;    // don't tenure anything at first

    LookupCache::flush();

    code = new Zone( current_sizes._code_size );
}


void Universe::cleanup_after_bootstrap() {
    ObjectIDTable::cleanup_after_bootstrap();

    Universe::_callBack_receiver = nilObject();
    Universe::_callBack_selector = SymbolOop( nilObject() );

    Universe::_dll_lookup_receiver = nilObject();
    Universe::_dll_lookup_selector = SymbolOop( nilObject() );

    Universe::_pic_free_list = ObjectArrayKlass::allocate_tenured_pic( number_of_interpreterPolymorphicInlineCache_sizes );

    Universe::_characterKlassObject = KlassOop( find_global( "Character" ) );
    // Check if all roots are valid.
    Universe::roots_do( Universe::check_root );
}


void Universe::check_root( Oop *p ) {
    if ( *p == badOop ) st_fatal( "badOop found in roots" );
}


void Universe::switch_pointers( Oop from, Oop to ) {
    st_assert( from->is_mem() and to->is_mem(), "unexpected kind of pointer switching" );

    // FIX LATER
    // st_assert(not from->is_old() or to->is_old(), "shouldn't be switching an old Oop to a new Oop");
    // APPLY_TO_VM_OOPS( SWITCH_POINTERS_TEMPLATE );

    new_gen.switch_pointers( from, to );
    old_gen.switch_pointers( from, to );

    // FIX LATER
    // processes->switch_pointers(from, to);
    symbol_table->switch_pointers( from, to );
}


MemOop Universe::relocate( MemOop p ) {
    static_cast<void>(p); // unused

    //APPLY_TO_SPACES(SPACE_OOP_RELOCATE_TEMPLATE);
    ShouldNotReachHere(); // Oop not in any old Space
    return nullptr;
}


bool Universe::verify_oop( MemOop p ) {

    if ( new_gen.eden()->contains( p ) ) {
        return true;
    }

    if ( new_gen.from()->contains( p ) ) {
        return true;
    }

    if ( old_gen.contains( p ) ) {
        return true;
    }

    if ( new_gen.to()->contains( p ) ) {
        error( "MemOop 0x{0:x} is in new generation to-space", p );
    } else {
        error( "MemOop 0x{0:x} is not in new generation to-space", p );
    }

    return false;
}


void Universe::verify( bool postScavenge ) {
    ResourceMark resourceMark;
    SPDLOG_INFO( "status-verify:  " );

    new_gen.verify();
    SPDLOG_INFO( "newgen, " );

    old_gen.verify();
    SPDLOG_INFO( "oldgen, " );

    remembered_set->verify( postScavenge );
    SPDLOG_INFO( "remembered_set, " );

    symbol_table->verify();
    SPDLOG_INFO( "symbol_table, " );

    Processes::verify();
    SPDLOG_INFO( "processes " );

    SPDLOG_INFO( "ok" );
}


void Universe::print() {

    SPDLOG_INFO( "Memory:" );
    new_gen.print();
    old_gen.print();
    if ( WizardMode ) {
        SPDLOG_INFO( ", tenuring_threshold=[0x{08:x}]", tenuring_threshold );
    }

}


Oop *Universe::object_start( Oop *p ) {
    if ( new_gen.contains( p ) )
        return new_gen.object_start( p );
    return old_gen.object_start( p );
}


class PrintClosure : public ObjectClosure {

private:
    void do_object( MemOop obj ) {
        PrintObjectClosure blk( _console );
        blk.do_object( obj );
        obj->layout_iterate( &blk );
    }

};


void Universe::print_layout() {
    PrintClosure blk;
    object_iterate( &blk );
}


static void decode_method( MethodOop method, KlassOop klass ) {

    if ( WizardMode ) {
        ResourceMark resourceMark;
        method->print_codes();
    }

    {
        ResourceMark resourceMark;
        method->print();
        PrettyPrinter::print( method, klass );
    }

}


static void decode_klass( SymbolOop name, KlassOop klass ) {
    static_cast<void>(name); // unused

    ObjectArrayOop f{ nullptr };

    // Klass methods
    f = klass->klass_part()->methods();
    for ( std::int32_t index = 1; index <= f->length(); index++ ) {
        decode_method( MethodOop( f->obj_at( index ) ), klass );
    }

    // Mixin methods
    f = klass->klass_part()->mixin()->methods();
    for ( std::int32_t index = 1; index <= f->length(); index++ ) {
        decode_method( MethodOop( f->obj_at( index ) ), klass );
    }

}


void Universe::decode_methods() {

    for ( std::int32_t i = 1; i <= Universe::systemDictionaryObject()->length(); i++ ) {
        AssociationOop assoc = (AssociationOop) Universe::systemDictionaryObject()->obj_at( i );
        if ( assoc->value()->is_klass() )
            decode_klass( assoc->key(), KlassOop( assoc->value() ) );
    }
}


void Universe::object_iterate( ObjectClosure *blk ) {
    new_gen.object_iterate( blk );
    old_gen.object_iterate( blk );
}


static OopClosure *the_blk;


static void the_func( Oop *p ) {
    the_blk->do_oop( p );
}


void Universe::root_iterate( OopClosure *blk ) {
    the_blk = blk;
    Universe::oops_do( the_func );
}


// Traverses the system dictionary to find the association referring the class or meta class and then prints the key.
void Universe::print_klass_name( KlassOop k ) {

    std::int32_t l = systemDictionaryObject()->length();

    for ( std::int32_t i = 1; i <= l; i++ ) {
        AssociationOop assoc = (AssociationOop) systemDictionaryObject()->obj_at( i );
        if ( assoc->value() == k ) {
            assoc->key()->print_symbol_on();
            return;
        } else if ( assoc->value()->klass() == k ) {
            assoc->key()->print_symbol_on();
            SPDLOG_INFO( " class" );
            return;
        }
    }
}


const char *Universe::klass_name( KlassOop k ) {

    if ( k == nullptr )
        return "(nullptr)";

    std::int32_t l = systemDictionaryObject()->length();

    for ( std::int32_t i = 1; i <= l; i++ ) {
        AssociationOop assoc = (AssociationOop) systemDictionaryObject()->obj_at( i );
        if ( assoc->value() == k ) {
            return assoc->key()->as_string();
        } else if ( assoc->value()->klass() == k ) {
            SymbolOop name    = assoc->key();
            char      *result = new_resource_array<char>( name->length() + 7 );
            sprintf( result, "%s class", name->as_string() );
            return result;
        }
    }

    // it's an unknown class (mixin invocation)
    KlassOop super = k->klass_part()->superKlass();
    if ( super not_eq ::nilObject ) {
        const char *superName = klass_name( super );
        const char *txt       = "unnamed class inheriting from ";
        char       *result    = new_resource_array<char>( strlen( txt ) + strlen( superName ) + 2 );
        sprintf( result, "%s%s", txt, superName );
        return result;
    } else {
        return "<top>";
    }
}


KlassOop Universe::method_holder_of( MethodOop m ) {
    m = m->home();    // so block methods can be found, too
    std::int32_t l = systemDictionaryObject()->length();

    for ( std::int32_t i = 1; i <= l; i++ ) {
        AssociationOop assoc = (AssociationOop) systemDictionaryObject()->obj_at( i );
        if ( assoc->value()->is_klass() ) {
            KlassOop k = KlassOop( assoc->value() );
            KlassOop res;
            if ( ( res = k->klass_part()->lookup_method_holder_for( m ) ) not_eq nullptr ) {
                // note: do search this way because not all superclasses are in system dictionary
                return res;
            } else if ( ( res = k->klass()->klass_part()->lookup_method_holder_for( m ) ) not_eq nullptr ) {
                return res;    // in metaclass hierarchy
            }
        }
    }

    if ( WizardMode ) {
        spdlog::warn( "could not find methodHolder of method at 0x{0:x}", static_cast<const void *>(m) );
    }

    return nullptr;
}


SymbolOop Universe::find_global_key_for( Oop value, bool *meta ) {
    *meta = false;
    std::int32_t l = systemDictionaryObject()->length();

    for ( std::int32_t i = 1; i <= l; i++ ) {
        AssociationOop assoc = AssociationOop( systemDictionaryObject()->obj_at( i ) );
        if ( assoc->is_constant() and assoc->value()->is_klass() ) {
            if ( assoc->value() == value ) {
                return assoc->key();
            } else {
                KlassOop s = KlassOop( assoc->value() )->klass();
                if ( s == value ) {
                    *meta = true;
                    return assoc->key();
                }
            }
        }
    }

    return nullptr;
}


Oop Universe::find_global( const char *name, bool must_be_constant ) {

    if ( not must_be_constant ) {
        if ( strcmp( name, "true" ) == 0 )
            return trueObject();
        if ( strcmp( name, "false" ) == 0 )
            return falseObject();
        if ( strcmp( name, "nil" ) == 0 )
            return nilObject();
    }

    SymbolOop    sym = oopFactory::new_symbol( name );
    std::int32_t l   = systemDictionaryObject()->length();

    for ( std::int32_t i = 1; i <= l; i++ ) {
        AssociationOop assoc = AssociationOop( systemDictionaryObject()->obj_at( i ) );
        if ( assoc->key() == sym ) {
            if ( not must_be_constant or assoc->is_constant() ) {
                return assoc->value();
            }
        }
    }
    return nullptr;
}


AssociationOop Universe::find_global_association( const char *name ) {
    SymbolOop symbolOop = oopFactory::new_symbol( name );

    std::int32_t l = systemDictionaryObject()->length();

    for ( std::int32_t i = 1; i <= l; i++ ) {
        AssociationOop assoc = (AssociationOop) systemDictionaryObject()->obj_at( i );
        if ( assoc->key() == symbolOop )
            return assoc;
    }

    return nullptr;
}


void Universe::methods_in_array_do( ObjectArrayOop array, void f( MethodOop method ) ) {
    std::int32_t length = array->length();

    for ( std::int32_t i = 1; i <= length; i++ ) {
        MethodOop method = MethodOop( array->obj_at( i ) );
        st_assert( method->is_method(), "just checking" );
        f( method );
    }
}


void Universe::methods_for_do( KlassOop klass, void f( MethodOop method ) ) {
    Klass *k = klass->klass_part();
    methods_in_array_do( k->methods(), f );
    methods_in_array_do( klass->klass()->klass_part()->methods(), f );
    if ( k->is_named_class() ) {
        // Fix the mixin parts
        methods_in_array_do( k->mixin()->methods(), f );
        methods_in_array_do( klass->klass()->klass_part()->mixin()->methods(), f );
    }
}


class MethodsClosure : public klassOopClosure {

private:
    void (*_function)( MethodOop method );

public:
    MethodsClosure( void f( MethodOop method ) ) : _function{ f } {
        _function = f;
    }


    void do_klass( KlassOop klass ) {
        Universe::methods_for_do( klass, _function );
    }
};


void Universe::methods_do( void f( MethodOop method ) ) {
    MethodsClosure it( f );
    classes_do( &it );
}


void Universe::classes_for_do( KlassOop klass, klassOopClosure *iterator ) {
    // call f with the class
    iterator->do_klass( klass );
    // recurse if the super class is a anonymous class
    if ( not klass->klass_part()->has_superKlass() )
        return;
    if ( not klass->klass_part()->superKlass()->klass_part()->is_named_class() )
        return;
    classes_for_do( klass->klass_part()->superKlass(), iterator );
}


void Universe::classes_do( klassOopClosure *iterator ) {

    ObjectArrayOop array  = Universe::systemDictionaryObject();
    std::int32_t   length = array->length();

    for ( std::int32_t i = 1; i <= length; i++ ) {
        AssociationOop assoc = AssociationOop( array->obj_at( i ) );
        st_assert( assoc->is_association(), "just checking" );
        if ( assoc->is_constant() and assoc->value()->is_klass() ) {
            classes_for_do( KlassOop( assoc->value() ), iterator );
        }
    }
}


void Universe::flush_inline_caches_in_method( MethodOop method ) {
    method->clear_inline_caches();
}


class FlushClosure : public ObjectClosure {
private:
    void do_object( MemOop obj ) {
        if ( obj->is_method() )
            MethodOop( obj )->clear_inline_caches();
    }
};


void Universe::flush_inline_caches_in_methods() {
    FlushClosure blk;
    object_iterate( &blk );
}


class AllMethodsClosure : public ObjectClosure {

private:
    void (*_function)( MethodOop m );

public:
    AllMethodsClosure( void f( MethodOop m ) ) : _function{ f } {
    }


    void do_object( MemOop obj ) {
        if ( obj->is_method() ) {
            _function( MethodOop( obj ) );
        }
    }

};


void Universe::methodOops_do( void f( MethodOop m ) ) {
    AllMethodsClosure blk( f );
    object_iterate( &blk );
}


static void cleanup_method( MethodOop m ) {
    m->cleanup_inline_caches();
}


void Universe::cleanup_all_inline_caches() {
    DeltaCallCache::clearAll();
    methodOops_do( cleanup_method );
    code->cleanup_inline_caches();
}


void universe_init() {
    SPDLOG_INFO( "system-init:  universe_init" );

    Universe::genesis();
}


void Universe::roots_do( void f( Oop * ) ) {
    // External Objects
    f( (Oop *) &::nilObject );
    f( (Oop *) &::trueObject );
    f( (Oop *) &::falseObject );

    // External Classes
    f( (Oop *) &::smiKlassObject );
    f( (Oop *) &::contextKlassObject );
    f( (Oop *) &::doubleKlassObject );

    // Classes
    f( (Oop *) &_memOopKlassObject );
    f( (Oop *) &_objArrayKlassObject );
    f( (Oop *) &_byteArrayKlassObject );
    f( (Oop *) &::symbolKlassObject );
    f( (Oop *) &_associationKlassObject );
    f( (Oop *) &::zeroArgumentBlockKlassObject );
    f( (Oop *) &::oneArgumentBlockKlassObject );
    f( (Oop *) &::twoArgumentBlockKlassObject );
    f( (Oop *) &::threeArgumentBlockKlassObject );
    f( (Oop *) &::fourArgumentBlockKlassObject );
    f( (Oop *) &::fiveArgumentBlockKlassObject );
    f( (Oop *) &::sixArgumentBlockKlassObject );
    f( (Oop *) &::sevenArgumentBlockKlassObject );
    f( (Oop *) &::eightArgumentBlockKlassObject );
    f( (Oop *) &::nineArgumentBlockKlassObject );
    f( (Oop *) &_methodKlassObject );
    f( (Oop *) &_characterKlassObject );
    // fix f((Oop*)&::doubleValueArrayKlassObject);

    // Objects
    f( (Oop *) &_asciiCharacters );
    f( (Oop *) &_systemDictionaryObject );
    f( (Oop *) &_objectIDTable );
    f( (Oop *) &_callBack_receiver );
    f( (Oop *) &_callBack_selector );
    f( (Oop *) &_dll_lookup_receiver );
    f( (Oop *) &_dll_lookup_selector );
    f( (Oop *) &_pic_free_list );

    f( (Oop *) &_sweeper_method );

    f( (Oop *) &_vframeKlassObject );
}


void Universe::oops_do( void f( Oop * ) ) {
    // Iterate over the local roots
    roots_do( f );
    // Iterate over the zone
    code->oops_do( f );
    // Iterate over all handles
    Handles::oops_do( f );
    // Iterate over the oops in the inlining database
    InliningDatabase::oops_do( f );
}


void Universe::add_global( Oop value ) {
    _systemDictionaryObject = _systemDictionaryObject->copy_add( value );
}


void Universe::remove_global_at( std::int32_t index ) {
    _systemDictionaryObject = _systemDictionaryObject->copy_remove( index );
}


bool Universe::on_page_boundary( void *addr ) {
    return ( (std::int32_t) addr ) % page_size() == 0;
}


std::int32_t Universe::page_size() {
    return os::vm_page_size();
}


void Universe::store( Oop *p, Oop contents, bool cs ) {
    st_assert( is_heap( p ) or not cs, "Reference must be in object memory to mark card." );
    *p = contents;
    if ( cs and ( p >= (Oop *) Universe::new_gen.boundary() ) ) remembered_set->record_store( p );
    if ( cs )
        remembered_set->record_store( p );
}


Oop *Universe::allocate_in_survivor_space( MemOop p, std::int32_t size, bool &is_new ) {
    if ( p->mark()->age() < tenuring_threshold and new_gen.would_fit( size ) ) {
        is_new = true;
        return new_gen.allocate_in_survivor_space( size );
    } else {
        is_new = false;
        return old_gen.allocate( size );
    }
}


Oop Universe::tenure( Oop p ) {
    tenuring_threshold = 0;        // tenure everything
    scavenge( &p );
//#define checkIt( s ) st_assert(s->used() == 0, "new spaces should be empty");
//    APPLY_TO_YOUNG_SPACES( checkIt )
//#undef checkIt

    st_assert( new_gen.eden()->used() == 0, "new spaces should be empty" );
    st_assert( new_gen.from()->used() == 0, "new spaces should be empty" );
    st_assert( new_gen.to()->used() == 0, "new spaces should be empty" );

    return p;
}


bool Universe::can_scavenge() {
    // don't scavenge if we're in critical vm operation
    if ( processSemaphore )
        return false;

    if ( BlockScavenge::is_blocked() )
        return false;

    // don't scavenge if we're allocating in the VM process.
    if ( DeltaProcess::active()->in_vm_operation() )
        return false;

    return true;
}


extern "C" void scavenge_and_allocate( std::int32_t size ) {
    Universe::scavenge_and_allocate( size, nullptr );
}


Oop *Universe::scavenge_and_allocate( std::int32_t size, Oop *p ) {
    // Fix this:
    //  If it is a huge object we are allocating we should allocate it in old_space and return without doing a scavenge
    if ( not can_scavenge() ) {
        _scavenge_blocked = true;
        return allocate_tenured( size );
    }

    VM_Scavenge op( p );
    VMProcess::execute( &op );
//  The following assertions break the tests
//  assert(DeltaProcess::active()->last_Delta_fp() not_eq nullptr, "last Delta fp should be present");
//  assert(DeltaProcess::active()->last_Delta_sp() not_eq nullptr, "last Delta fp should be present");
    _scavenge_blocked = false;
    return allocate_without_scavenge( size );
}


void Universe::scavenge_oop( Oop *p ) {
    *p = ( *p )->scavenge();
}


bool Universe::needs_garbage_collection() {
    return old_gen.free() < new_gen.to()->capacity();
}


void os_dump_context();


void Universe::scavenge( Oop *p ) {
    // %note
    //   the symbol_table can be ignored during scavenge since all all symbols are tenured.
    FlagSetting fl( garbageCollectionInProgress, true );
    if ( DeltaProcess::stepping )
        breakpoint();

    ResourceMark resourceMark;
    scavengeCount++;
    st_assert( not processSemaphore, "processSemaphore shouldn't be set" );
    {
        EventMarker m( "scavenging" );
        TraceTime   t( "Scavenge", PrintScavenge );

        if ( PrintScavenge and WizardMode ) {
            SPDLOG_INFO( " {:d}", tenuring_threshold );
        }

        if ( VerifyBeforeScavenge ) {
            verify();
        }
        WeakArrayRegister::begin_scavenge();

        // Getting ready for scavenge
        age_table->clear();

        new_gen._toSpace->clear();

        // Save top of to_space and old_gen
        NewWaterMark to_mark  = new_gen._toSpace->top_mark();
        OldWaterMark old_mark = old_gen.top_mark();

        // Scavenge all roots
        if ( p ) {
            SCAVENGE_TEMPLATE( p );
        }

        Universe::oops_do( scavenge_oop );
        //Universe::roots_do(scavenge_oop);
        //Handles::oops_do(scavenge_oop);

        {
            FOR_EACH_OLD_SPACE( s ) {
                s->scavenge_recorded_stores();
            }
        }

        Processes::scavenge_contents();
        NotificationQueue::oops_do( &Universe::scavenge_oop );

        // Scavenge promoted contents in to_space and old_gen until done.

        while ( ( old_mark not_eq old_gen.top_mark() ) or ( to_mark not_eq new_gen._toSpace->top_mark() ) ) {
            old_gen.scavenge_contents_from( &old_mark );
            new_gen._toSpace->scavenge_contents_from( &to_mark );
        }

        WeakArrayRegister::check_and_scavenge_contents();

        new_gen.swap_spaces();

        // Set the desired survivor size to half the real survivor Space
        std::int32_t desired_survivor_size = new_gen.to()->capacity() / 2;
        tenuring_threshold = age_table->tenuring_threshold( desired_survivor_size / OOP_SIZE );

        if ( VerifyAfterScavenge ) {
            verify( true );
        }

        // do this at end so an overflow during a scavenge doesnt cause another one
        scavengeRequired = false;
    }
}


Space *Universe::spaceFor( void *p ) {

    if ( new_gen.from()->contains( p ) ) {
        return new_gen.from();
    }

    if ( new_gen.eden()->contains( p ) ) {
        return new_gen.eden();
    }

    {
        FOR_EACH_OLD_SPACE( s )if ( s->contains( p ) )
                return s;
    }

    ShouldNotReachHere(); // not in any space
    return nullptr;
}


KlassOop Universe::smiKlassObject() {
    return ::smiKlassObject;
}


KlassOop Universe::contextKlassObject() {
    return ::contextKlassObject;
}


KlassOop Universe::doubleKlassObject() {
    return ::doubleKlassObject;
}


KlassOop Universe::memOopKlassObject() {
    return _memOopKlassObject;
}


KlassOop Universe::objArrayKlassObject() {
    return _objArrayKlassObject;
}


KlassOop Universe::byteArrayKlassObject() {
    return _byteArrayKlassObject;
}


KlassOop Universe::symbolKlassObject() {
    return ::symbolKlassObject;
}


KlassOop Universe::associationKlassObject() {
    return _associationKlassObject;
}


KlassOop Universe::zeroArgumentBlockKlassObject() {
    return ::zeroArgumentBlockKlassObject;
}


KlassOop Universe::oneArgumentBlockKlassObject() {
    return ::oneArgumentBlockKlassObject;
}


KlassOop Universe::twoArgumentBlockKlassObject() {
    return ::twoArgumentBlockKlassObject;
}


KlassOop Universe::threeArgumentBlockKlassObject() {
    return ::threeArgumentBlockKlassObject;
}


KlassOop Universe::fourArgumentBlockKlassObject() {
    return ::fourArgumentBlockKlassObject;
}


KlassOop Universe::fiveArgumentBlockKlassObject() {
    return ::fiveArgumentBlockKlassObject;
}


KlassOop Universe::sixArgumentBlockKlassObject() {
    return ::sixArgumentBlockKlassObject;
}


KlassOop Universe::sevenArgumentBlockKlassObject() {
    return ::sevenArgumentBlockKlassObject;
}


KlassOop Universe::eightArgumentBlockKlassObject() {
    return ::eightArgumentBlockKlassObject;
}


KlassOop Universe::nineArgumentBlockKlassObject() {
    return ::nineArgumentBlockKlassObject;
}


KlassOop Universe::methodKlassObject() {
    return _methodKlassObject;
}


KlassOop Universe::characterKlassObject() {
    return _characterKlassObject;
}


KlassOop Universe::doubleValueArrayKlassObject() {
    return ::doubleValueArrayKlassObject;
}


KlassOop Universe::vframeKlassObject() {
    return _vframeKlassObject;
}


Oop Universe::nilObject() {
    return ::nilObject;
}


Oop Universe::trueObject() {
    return ::trueObject;
}


Oop Universe::falseObject() {
    return ::falseObject;
}


ObjectArrayOop Universe::asciiCharacters() {
    return _asciiCharacters;
}


ObjectArrayOop Universe::systemDictionaryObject() {
    return _systemDictionaryObject;
}


ObjectArrayOop Universe::pic_free_list() {
    return _pic_free_list;
}


Oop Universe::callBack_receiver() {
    return _callBack_receiver;
}


SymbolOop Universe::callBack_selector() {
    return _callBack_selector;
}


void Universe::set_callBack( Oop receiver, SymbolOop selector ) {
    _callBack_receiver = receiver;
    _callBack_selector = selector;
}


Oop Universe::dll_lookup_receiver() {
    return _dll_lookup_receiver;
}


SymbolOop Universe::dll_lookup_selector() {
    return _dll_lookup_selector;
}


void Universe::set_dll_lookup( Oop receiver, SymbolOop selector ) {
    _dll_lookup_receiver = receiver;
    _dll_lookup_selector = selector;
}


MethodOop Universe::sweeper_method() {
    return _sweeper_method;
}
