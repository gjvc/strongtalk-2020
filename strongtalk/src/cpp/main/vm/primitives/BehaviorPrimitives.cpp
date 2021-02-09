
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/system/platform.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/primitives/BehaviorPrimitives.hpp"


TRACE_FUNC( TraceBehaviorPrims, "behavior" )


std::int32_t BehaviorPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_klass(), "receiver must be klass object")


PRIM_DECL_2( BehaviorPrimitives::allocate3, Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    PROLOGUE_2( "allocate3", receiver, tenured )
    ASSERT_RECEIVER;
    if ( tenured not_eq Universe::trueObject() and tenured not_eq Universe::falseObject() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    Oop result = receiver->primitive_allocate( false, tenured == Universe::trueObject() );
    if ( nullptr == result )
        return markSymbol( vmSymbols::failed_allocation() );
    return result;
}


PRIM_DECL_1( BehaviorPrimitives::allocate2, Oop receiver ) {
    PROLOGUE_1( "allocate2", receiver )
    ASSERT_RECEIVER;
    return receiver->primitive_allocate();
}


PRIM_DECL_1( BehaviorPrimitives::allocate, Oop receiver ) {
    PROLOGUE_1( "allocate", receiver )
    ASSERT_RECEIVER;
    return receiver->primitive_allocate();
}


PRIM_DECL_1( BehaviorPrimitives::superclass, Oop receiver ) {
    PROLOGUE_1( "superclass", receiver );
    ASSERT_RECEIVER;
    return KlassOop( receiver )->klass_part()->superKlass();
}


PRIM_DECL_1( BehaviorPrimitives::superclass_of, Oop klass ) {
    PROLOGUE_1( "superclass_of", klass );
    if ( not klass->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return KlassOop( klass )->klass_part()->superKlass();
}


PRIM_DECL_2( BehaviorPrimitives::setSuperclass, Oop receiver, Oop newSuper ) {
    PROLOGUE_2( "setSuperclass", receiver, newSuper );
    if ( not receiver->is_klass() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( not( newSuper->is_klass() or newSuper == nilObject ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    Klass    *receiverClass = KlassOop( receiver )->klass_part();
    KlassOop newSuperclass;
    if ( receiverClass->superKlass() == newSuper )
        return receiver; // no change
    if ( receiverClass->superKlass() == nilObject ) {
        newSuperclass = KlassOop( newSuper );
        if ( newSuperclass->klass_part()->number_of_instance_variables() > 0 )
            return markSymbol( vmSymbols::argument_is_invalid() );
    } else {
        Klass *oldSuperclass = receiverClass->superKlass()->klass_part();
        if ( newSuper == nilObject ) {
            newSuperclass = KlassOop( nilObject );
            if ( oldSuperclass->number_of_instance_variables() > 0 )
                return markSymbol( vmSymbols::argument_is_invalid() );
        } else {
            newSuperclass = KlassOop( newSuper );

            if ( not oldSuperclass->has_same_inst_vars_as( newSuperclass ) )
                return markSymbol( vmSymbols::invalid_klass() );
        }
    }
    receiverClass->set_superKlass( newSuperclass );

    Universe::flush_inline_caches_in_methods();
    Universe::code->clear_inline_caches();

    LookupCache::flush();
    DeltaCallCache::clearAll();

    return receiver;
}


PRIM_DECL_1( BehaviorPrimitives::mixinOf, Oop behavior ) {
    PROLOGUE_1( "mixinOf", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return KlassOop( behavior )->klass_part()->mixin();
}


PRIM_DECL_1( BehaviorPrimitives::headerSize, Oop behavior ) {
    PROLOGUE_1( "headerSize", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return smiOopFromValue( KlassOop( behavior )->klass_part()->oop_header_size() );
}


PRIM_DECL_1( BehaviorPrimitives::nonIndexableSize, Oop behavior ) {
    PROLOGUE_1( "nonIndexableSize", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return smiOopFromValue( KlassOop( behavior )->klass_part()->non_indexable_size() );
}


PRIM_DECL_1( BehaviorPrimitives::is_specialized_class, Oop behavior ) {
    PROLOGUE_1( "is_specialized_class", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return KlassOop( behavior )->klass_part()->is_specialized_class() ? trueObject : falseObject;
}


PRIM_DECL_1( BehaviorPrimitives::can_be_subclassed, Oop behavior ) {
    PROLOGUE_1( "can_be_subclassed", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return KlassOop( behavior )->klass_part()->can_be_subclassed() ? trueObject : falseObject;
}


PRIM_DECL_1( BehaviorPrimitives::can_have_instance_variables, Oop behavior ) {
    PROLOGUE_1( "can_have_instance_variables", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return KlassOop( behavior )->klass_part()->can_have_instance_variables() ? trueObject : falseObject;
}

// OPERATIONS FOR CLASS VARIABLES

PRIM_DECL_2( BehaviorPrimitives::classVariableAt, Oop behavior, Oop index ) {
    PROLOGUE_2( "classVariableAt", behavior, index );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    std::int32_t i = SmallIntegerOop( index )->value();
    if ( i > 0 and i <= KlassOop( behavior )->klass_part()->number_of_classVars() )
        return KlassOop( behavior )->klass_part()->classVar_at( i );
    return markSymbol( vmSymbols::out_of_bounds() );
}


PRIM_DECL_1( BehaviorPrimitives::classVariables, Oop behavior ) {
    PROLOGUE_1( "classVariables", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return KlassOop( behavior )->klass_part()->classVars();
}

// OPERATIONS FOR METHODS

PRIM_DECL_2( BehaviorPrimitives::printMethod, Oop receiver, Oop name ) {
    PROLOGUE_2( "printMethod", receiver, name );
    ASSERT_RECEIVER;
    if ( not name->isByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    MethodOop m = KlassOop( receiver )->klass_part()->lookup( SymbolOop( name ) );
    if ( not m )
        return markSymbol( vmSymbols::not_found() );
    m->print_codes();
    return receiver;
}


/*


PRIM_DECL_1(BehaviorPrimitives::new0, Oop receiver){
  PROLOGUE_1("new0", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size(), &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  return obj;
}


PRIM_DECL_1(BehaviorPrimitives::new1, Oop receiver){
  PROLOGUE_1("new1", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 1, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 1 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new2, Oop receiver){
  PROLOGUE_1("new2", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 2, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 2 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new3, Oop receiver){
  PROLOGUE_1("new3", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 3, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 3 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new4, Oop receiver){
  PROLOGUE_1("new4", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 4, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 4 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 3), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new5, Oop receiver){
  PROLOGUE_1("new5", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 5, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 5 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 3), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 4), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new6, Oop receiver){
  PROLOGUE_1("new6", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 6, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 6 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 3), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 4), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 5), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new7, Oop receiver){
  PROLOGUE_1("new7", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 7, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 7 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 3), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 4), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 5), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 6), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new8, Oop receiver){
  PROLOGUE_1("new8", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 8, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 8 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 3), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 4), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 5), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 6), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 7), value, false);
  return obj;
}

PRIM_DECL_1(BehaviorPrimitives::new9, Oop receiver){
  PROLOGUE_1("new9", receiver);
  MemOop klass = MemOop(receiver);
  // allocate
  MemOop obj = as_memOop(Universe::allocate(memOopDescriptor::header_size() + 9, &klass));
  // header
  obj->initialize_header(false, klassOop(klass));
  // initialize 9 instance variable
  Oop value = nilObject;
  Universe::store(obj->oops(memOopDescriptor::header_size() + 0), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 1), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 2), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 3), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 4), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 5), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 6), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 7), value, false);
  Universe::store(obj->oops(memOopDescriptor::header_size() + 8), value, false);
   return obj;
}

*/

PRIM_DECL_2( BehaviorPrimitives::methodFor, Oop receiver, Oop selector ) {
    PROLOGUE_2( "methodFor", receiver, selector );
    ASSERT_RECEIVER;

    if ( not selector->isSymbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    MethodOop m = KlassOop( receiver )->klass_part()->lookup( SymbolOop( selector ) );
    if ( m )
        return m;
    return markSymbol( vmSymbols::not_found() );
}


PRIM_DECL_1( BehaviorPrimitives::format, Oop behavior ) {
    PROLOGUE_1( "format", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    const char *format_name = Klass::name_from_format( KlassOop( behavior )->klass_part()->format() );
    return OopFactory::new_symbol( format_name );
}


PRIM_DECL_1( BehaviorPrimitives::vm_type, Oop behavior ) {
    PROLOGUE_1( "format", behavior );
    if ( not behavior->is_klass() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    Klass::Format f = KlassOop( behavior )->klass_part()->format();

    switch ( f ) {
        case Klass::Format::mem_klass:
            return vmSymbols::mem_klass();
        case Klass::Format::association_klass:
            return vmSymbols::association_klass();
        case Klass::Format::block_closure_klass:
            return vmSymbols::blockClosure_klass();
        case Klass::Format::byte_array_klass:
            return vmSymbols::byteArray_klass();
        case Klass::Format::symbol_klass:
            return vmSymbols::symbol_klass();
        case Klass::Format::context_klass:
            return vmSymbols::context_klass();
        case Klass::Format::double_byte_array_klass:
            return vmSymbols::doubleByteArray_klass();
        case Klass::Format::double_value_array_klass:
            return vmSymbols::doubleValueArray_klass();
        case Klass::Format::double_klass:
            return vmSymbols::double_klass();
        case Klass::Format::klass_klass:
            return vmSymbols::klass_klass();
        case Klass::Format::method_klass:
            return vmSymbols::method_klass();
        case Klass::Format::mixin_klass:
            return vmSymbols::mixin_klass();
        case Klass::Format::object_array_klass:
            return vmSymbols::objectArray_klass();
        case Klass::Format::weak_array_klass:
            return vmSymbols::weakArray_klass();
        case Klass::Format::process_klass:
            return vmSymbols::process_klass();
        case Klass::Format::virtual_frame_klass:
            return vmSymbols::vframe_klass();
        case Klass::Format::proxy_klass:
            return vmSymbols::proxy_klass();
        case Klass::Format::smi_klass:
            return vmSymbols::smi_klass();
        default: st_fatal( "wrong format for klass" );
    }
    return markSymbol( vmSymbols::first_argument_has_wrong_type() );
}


PRIM_DECL_2( BehaviorPrimitives::is_class_of, Oop receiver, Oop obj ) {
    PROLOGUE_2( "is_class_of", receiver, obj );
    ASSERT_RECEIVER;
    return obj->klass() == receiver ? trueObject : falseObject;
}


// empty functions, we'll patch them later
static void trap() {
    st_assert( false, "This primitive should be patched" );
}


extern "C" Oop primitiveInlineAllocations( Oop receiver, Oop count ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(count); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew0( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew1( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew2( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew3( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew4( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew5( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew6( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew7( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew8( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveNew9( Oop receiver, Oop tenured ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(tenured); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
