//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/system/dll.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/oop/AssociationOopDescriptor.hpp"


bool Interpreted_DLLCache::async() const {
    std::uint8_t *p = (std::uint8_t *) this;                // p point to first Oop in DLL call
    while ( ByteCodes::Code( *--p ) == ByteCodes::Code::halt );    // search back for DLL call bytecode
    ByteCodes::Code code = ByteCodes::Code( *p );
    st_assert( code == ByteCodes::Code::dll_call_sync or code == ByteCodes::Code::dll_call_async, "not a dll call" );
    return code == ByteCodes::Code::dll_call_async;
}


void CodeIterator::align() {
    _current = align( _current );
}


std::uint8_t *CodeIterator::align( std::uint8_t *p ) const {
    return (std::uint8_t *) ( ( (std::int32_t) p + 3 ) & ( ~3 ) );
}


CodeIterator::CodeIterator( MethodOop method, std::int32_t startByteCodeIndex ) :
    _methodOop{ method },
    _current{},
    _end{} {

    st_assert( ( PrologueByteCodeIndex <= startByteCodeIndex ) and ( startByteCodeIndex <= method->size_of_codes() * OOP_SIZE ), "startByteCodeIndex out of range" );
    set_byteCodeIndex( startByteCodeIndex );
    _end = method->codes_end();
}


CodeIterator::CodeIterator( std::uint8_t *hp ) :
    _methodOop{ MethodOopDescriptor::methodOop_from_hcode( hp ) },
    _current{ hp },
    _end{ _methodOop->codes_end() } {
}


void CodeIterator::set_byteCodeIndex( std::int32_t byteCodeIndex ) {
    _current = _methodOop->codes( byteCodeIndex );
}


std::int32_t CodeIterator::byteCodeIndex() const {
    return ( _current - _methodOop->codes() ) + 1;
}


std::int32_t CodeIterator::next_byteCodeIndex() const {
    return ( next_hp() - _methodOop->codes() ) + 1;
}


std::uint8_t *CodeIterator::next_hp() const {

    if ( _current >= _end ) {
        return nullptr;
    }

    switch ( format() ) {
        case ByteCodes::Format::B:
            return _current + 1;
        case ByteCodes::Format::BB:
            return _current + 2;
        case ByteCodes::Format::BBB:
            return _current + 3;
        case ByteCodes::Format::BBBB:
            return _current + 4;
        case ByteCodes::Format::BBO:
            [[fallthrough]];
        case ByteCodes::Format::BBL:
            return align( _current + 2 ) + OOP_SIZE;
        case ByteCodes::Format::BO:
            [[fallthrough]];
        case ByteCodes::Format::BL:
            return align( _current + 1 ) + OOP_SIZE;
        case ByteCodes::Format::BLB:
            return align( _current + 1 ) + OOP_SIZE + 1;
        case ByteCodes::Format::BOO:
            [[fallthrough]];
        case ByteCodes::Format::BLO:
            [[fallthrough]];
        case ByteCodes::Format::BOL:
            [[fallthrough]];
        case ByteCodes::Format::BLL:
            return align( _current + 1 ) + OOP_SIZE + OOP_SIZE;
        case ByteCodes::Format::BBOO:
            [[fallthrough]];
        case ByteCodes::Format::BBLO:
            return align( _current + 2 ) + OOP_SIZE + OOP_SIZE;
        case ByteCodes::Format::BOOLB:
            return align( _current + 1 ) + OOP_SIZE + OOP_SIZE + OOP_SIZE + 1;
        case ByteCodes::Format::BBS:
            return _current + 2 + ( _current[ 1 ] == 0 ? 256 : _current[ 1 ] );
        default:
            return nullptr;
    }

    ShouldNotReachHere();
}


InterpretedInlineCache *CodeIterator::ic() {

    switch ( format() ) {

        case ByteCodes::Format::BOO:
            [[fallthrough]];
        case ByteCodes::Format::BLO:
            return reinterpret_cast<InterpretedInlineCache *>( align( _current + 1 ) );

        case ByteCodes::Format::BBOO:
            [[fallthrough]];
        case ByteCodes::Format::BBLO:
            return reinterpret_cast<InterpretedInlineCache *>( align( _current + 2 ) );

        default:
            return nullptr;

    }

}


Interpreted_DLLCache *CodeIterator::dll_cache() {
    return reinterpret_cast<Interpreted_DLLCache *>( align( _current + 1 ) );
}


InterpretedPrimitiveCache *CodeIterator::primitive_cache() {
    return reinterpret_cast<InterpretedPrimitiveCache *>(_current);
}


const char *CodeIterator::interpreter_return_point( bool restore_value ) const {
    // The return is only valid if we are in a send/primtive call/dll call.

    if ( is_message_send() ) {
        switch ( argumentsType() ) {
            case ByteCodes::ArgumentSpec::recv_0_args:
            case ByteCodes::ArgumentSpec::recv_1_args:
            case ByteCodes::ArgumentSpec::recv_2_args:
            case ByteCodes::ArgumentSpec::recv_n_args:
                return pop_result() ? ( restore_value ? Interpreter::deoptimized_return_from_send_with_receiver_pop_restore() : Interpreter::deoptimized_return_from_send_with_receiver_pop() ) : ( restore_value ? Interpreter::deoptimized_return_from_send_with_receiver_restore() : Interpreter::deoptimized_return_from_send_with_receiver() );
            case ByteCodes::ArgumentSpec::args_only:
                return pop_result() ? ( restore_value ? Interpreter::deoptimized_return_from_send_without_receiver_pop_restore() : Interpreter::deoptimized_return_from_send_without_receiver_pop() ) : ( restore_value ? Interpreter::deoptimized_return_from_send_without_receiver_restore() : Interpreter::deoptimized_return_from_send_without_receiver() );
            default: ShouldNotReachHere();
        }
    }

    if ( is_primitive_call() ) {
        switch ( code() ) {
            case ByteCodes::Code::prim_call:
            case ByteCodes::Code::primitive_call_lookup:
            case ByteCodes::Code::primitive_call_self:
            case ByteCodes::Code::primitive_call_self_lookup:
                return restore_value ? Interpreter::deoptimized_return_from_primitive_call_without_failure_block_restore() : Interpreter::deoptimized_return_from_primitive_call_without_failure_block();
            case ByteCodes::Code::primitive_call_failure:
            case ByteCodes::Code::primitive_call_failure_lookup:
            case ByteCodes::Code::primitive_call_self_failure:
            case ByteCodes::Code::primitive_call_self_failure_lookup:
                return restore_value ? Interpreter::deoptimized_return_from_primitive_call_with_failure_block_restore() : Interpreter::deoptimized_return_from_primitive_call_with_failure_block();
            default: ShouldNotReachHere();
        }
    }

    if ( is_dll_call() ) {
        return restore_value ? Interpreter::deoptimized_return_from_dll_call_restore() : Interpreter::deoptimized_return_from_dll_call();
    }

    ShouldNotReachHere();
    return nullptr;
}


Oop *CodeIterator::block_method_addr() {
    switch ( code() ) {
        case ByteCodes::Code::push_new_closure_tos_0:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_tos_1:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_tos_2:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_0:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_1:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_2:
            return aligned_oop( 1 );
        case ByteCodes::Code::push_new_closure_tos_n:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_n:
            return aligned_oop( 2 );
        default:
            return nullptr;
    }
    return nullptr;
}


MethodOop CodeIterator::block_method() {
    switch ( code() ) {
        case ByteCodes::Code::push_new_closure_tos_0:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_tos_1:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_tos_2:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_0:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_1:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_2:
            return MethodOop( oop_at( 1 ) );
        case ByteCodes::Code::push_new_closure_tos_n:
            [[fallthrough]];
        case ByteCodes::Code::push_new_closure_context_n:
            return MethodOop( oop_at( 2 ) );
        default:
            return nullptr;
    }
    return nullptr;
}


void CodeIterator::customize_class_var_code( KlassOop to_klass ) {
    st_assert( code() == ByteCodes::Code::push_classVar_name or code() == ByteCodes::Code::store_classVar_pop_name or code() == ByteCodes::Code::store_classVar_name, "must be class variable byte code" );

    Oop       *p   = aligned_oop( 1 );
    SymbolOop name = SymbolOop( *p );
    st_assert( name->isSymbol(), "name must be symbol" );
    AssociationOop assoc = to_klass->klass_part()->lookup_class_var( name );
    if ( !assoc ) return;
    st_assert( assoc->is_old(), "must be tenured" );
    if ( code() == ByteCodes::Code::push_classVar_name )
        set_code( ByteCodes::Code::push_classVar );
    else if ( code() == ByteCodes::Code::store_classVar_pop_name )
        set_code( ByteCodes::Code::store_classVar_pop );
    else
        set_code( ByteCodes::Code::store_classVar );
    Universe::store( p, assoc, false );
}


void CodeIterator::uncustomize_class_var_code( KlassOop from_klass ) {
    static_cast<void>(from_klass); // unused

    st_assert( code() == ByteCodes::Code::push_classVar or code() == ByteCodes::Code::store_classVar_pop or code() == ByteCodes::Code::store_classVar, "must be class variable byte code" );

    Oop            *p        = aligned_oop( 1 );
    AssociationOop old_assoc = AssociationOop( *p );
    st_assert( old_assoc->is_association(), "must be association" );
    if ( code() == ByteCodes::Code::push_classVar )
        set_code( ByteCodes::Code::push_classVar_name );
    else if ( code() == ByteCodes::Code::store_classVar_pop )
        set_code( ByteCodes::Code::store_classVar_pop_name );
    else
        set_code( ByteCodes::Code::store_classVar_name );
    Universe::store( p, old_assoc->key(), false );
}


void CodeIterator::recustomize_class_var_code( KlassOop from_klass, KlassOop to_klass ) {
    static_cast<void>(from_klass); // unused
    st_assert( code() == ByteCodes::Code::push_classVar or code() == ByteCodes::Code::store_classVar_pop or code() == ByteCodes::Code::store_classVar, "must be class variable byte code" );

    Oop            *p        = aligned_oop( 1 );
    AssociationOop old_assoc = AssociationOop( *p );
    st_assert( old_assoc->is_association(), "must be association" );
    AssociationOop new_assoc = to_klass->klass_part()->lookup_class_var( old_assoc->key() );
    if ( new_assoc ) {
        Universe::store( p, new_assoc, false );
    } else {
        if ( code() == ByteCodes::Code::push_classVar )
            set_code( ByteCodes::Code::push_classVar_name );
        else if ( code() == ByteCodes::Code::store_classVar_pop )
            set_code( ByteCodes::Code::store_classVar_pop_name );
        else
            set_code( ByteCodes::Code::store_classVar_name );
        Universe::store( p, old_assoc->key(), false );
    }
}


void CodeIterator::customize_inst_var_code( KlassOop to_klass ) {
    st_assert( code() == ByteCodes::Code::push_instVar_name or code() == ByteCodes::Code::store_instVar_pop_name or code() == ByteCodes::Code::store_instVar_name or code() == ByteCodes::Code::return_instVar_name, "must be instance variable byte code" );

    Oop       *p   = aligned_oop( 1 );
    SymbolOop name = SymbolOop( *p );
    st_assert( name->isSymbol(), "name must be symbol" );
    std::int32_t offset = to_klass->klass_part()->lookup_inst_var( name );
    if ( offset < 0 ) return;

    if ( code() == ByteCodes::Code::push_instVar_name )
        set_code( ByteCodes::Code::push_instVar );
    else if ( code() == ByteCodes::Code::store_instVar_pop_name )
        set_code( ByteCodes::Code::store_instVar_pop );
    else if ( code() == ByteCodes::Code::store_instVar_name )
        set_code( ByteCodes::Code::store_instVar );
    else
        set_code( ByteCodes::Code::return_instVar );
    Universe::store( p, smiOopFromValue( offset ) );
}


void CodeIterator::uncustomize_inst_var_code( KlassOop from_klass ) {
    st_assert( code() == ByteCodes::Code::push_instVar or code() == ByteCodes::Code::store_instVar_pop or code() == ByteCodes::Code::store_instVar or code() == ByteCodes::Code::return_instVar, "must be instance variable byte code" );

    Oop *p = aligned_oop( 1 );
    st_assert( ( *p )->isSmallIntegerOop(), "must be small_int_t" );
    std::int32_t old_offset = SmallIntegerOop( *p )->value();
    SymbolOop    name       = from_klass->klass_part()->inst_var_name_at( old_offset );
    if ( not name ) {
        st_fatal( "instance variable not found" );
    }
    if ( code() == ByteCodes::Code::push_instVar )
        set_code( ByteCodes::Code::push_instVar_name );
    else if ( code() == ByteCodes::Code::store_instVar_pop )
        set_code( ByteCodes::Code::store_instVar_pop_name );
    else if ( code() == ByteCodes::Code::store_instVar )
        set_code( ByteCodes::Code::store_instVar_name );
    else
        set_code( ByteCodes::Code::return_instVar_name );
    Universe::store( p, name, false );
}


void CodeIterator::recustomize_inst_var_code( KlassOop from_klass, KlassOop to_klass ) {
    st_assert( code() == ByteCodes::Code::push_instVar or code() == ByteCodes::Code::store_instVar_pop or code() == ByteCodes::Code::store_instVar or code() == ByteCodes::Code::return_instVar, "must be instance variable byte code" );

    Oop *p = aligned_oop( 1 );
    st_assert( ( *p )->isSmallIntegerOop(), "must be small_int_t" );
    std::int32_t old_offset = SmallIntegerOop( *p )->value();
    SymbolOop    name       = from_klass->klass_part()->inst_var_name_at( old_offset );
    if ( not name ) {
        st_fatal( "instance variable not found" );
    }
    std::int32_t new_offset = to_klass->klass_part()->lookup_inst_var( name );
    if ( new_offset >= 0 ) {
        Universe::store( p, smiOopFromValue( new_offset ) );
    } else {
        if ( code() == ByteCodes::Code::push_instVar )
            set_code( ByteCodes::Code::push_instVar_name );
        else if ( code() == ByteCodes::Code::store_instVar_pop )
            set_code( ByteCodes::Code::store_instVar_pop_name );
        else if ( code() == ByteCodes::Code::store_instVar )
            set_code( ByteCodes::Code::store_instVar_name );
        else
            set_code( ByteCodes::Code::return_instVar_name );
        Universe::store( p, name, false );
    }
}
