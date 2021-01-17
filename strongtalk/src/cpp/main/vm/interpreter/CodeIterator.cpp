//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/system/dll.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/system/sizes.hpp"


bool_t Interpreted_DLLCache::async() const {
    uint8_t * p = ( uint8_t * ) this;                // p point to first Oop in DLL call
    while ( ByteCodes::Code( *--p ) == ByteCodes::Code::halt );    // search back for DLL call bytecode
    ByteCodes::Code code = ByteCodes::Code( *p );
    st_assert( code == ByteCodes::Code::dll_call_sync or code == ByteCodes::Code::dll_call_async, "not a dll call" );
    return code == ByteCodes::Code::dll_call_async;
}


void CodeIterator::align() {
    _current = align( _current );
}


uint8_t * CodeIterator::align( uint8_t * p ) const {
    return ( uint8_t * ) ( ( ( int ) p + 3 ) & ( ~3 ) );
}


CodeIterator::CodeIterator( MethodOop method, int startByteCodeIndex ) {
    st_assert( PrologueByteCodeIndex <= startByteCodeIndex and startByteCodeIndex <= method->size_of_codes() * oopSize, "startByteCodeIndex out of range" );
    _methodOop = method;
    set_byteCodeIndex( startByteCodeIndex );
    _end = method->codes_end();
}


CodeIterator::CodeIterator( uint8_t * hp ) {
    _methodOop = MethodOopDescriptor::methodOop_from_hcode( hp );
    _current   = hp;
    _end       = _methodOop->codes_end();
}


void CodeIterator::set_byteCodeIndex( int byteCodeIndex ) {
    _current = _methodOop->codes( byteCodeIndex );
}


int CodeIterator::byteCodeIndex() const {
    return ( _current - _methodOop->codes() ) + 1;
}


int CodeIterator::next_byteCodeIndex() const {
    return ( next_hp() - _methodOop->codes() ) + 1;
}


uint8_t * CodeIterator::next_hp() const {

    if ( _current >= _end )
        return nullptr;

    switch ( format() ) {
        case ByteCodes::Format::B:
            return _current + 1;
        case ByteCodes::Format::BB:
            return _current + 2;
        case ByteCodes::Format::BBB:
            return _current + 3;
        case ByteCodes::Format::BBBB:
            return _current + 4;
        case ByteCodes::Format::BBO:   // fall through
        case ByteCodes::Format::BBL:
            return align( _current + 2 ) + oopSize;
        case ByteCodes::Format::BO:    // fall through
        case ByteCodes::Format::BL:
            return align( _current + 1 ) + oopSize;
        case ByteCodes::Format::BLB:
            return align( _current + 1 ) + oopSize + 1;
        case ByteCodes::Format::BOO:   // fall through
        case ByteCodes::Format::BLO:   // fall through
        case ByteCodes::Format::BOL:   // fall through
        case ByteCodes::Format::BLL:
            return align( _current + 1 ) + oopSize + oopSize;
        case ByteCodes::Format::BBOO:  // fall through
        case ByteCodes::Format::BBLO:
            return align( _current + 2 ) + oopSize + oopSize;
        case ByteCodes::Format::BOOLB:
            return align( _current + 1 ) + oopSize + oopSize + oopSize + 1;
        case ByteCodes::Format::BBS:
            return _current + 2 + ( _current[ 1 ] == 0 ? 256 : _current[ 1 ] );
    }
    ShouldNotReachHere();
    return nullptr;
}


InterpretedInlineCache * CodeIterator::ic() {

    switch ( format() ) {

        case ByteCodes::Format::BOO:   // fall through
        case ByteCodes::Format::BLO:
            return reinterpret_cast<InterpretedInlineCache *>( align( _current + 1 ) );

        case ByteCodes::Format::BBOO:  // fall through
        case ByteCodes::Format::BBLO:
            return reinterpret_cast<InterpretedInlineCache *>( align( _current + 2 ) );

        default:
            return nullptr;

    }

}


Interpreted_DLLCache * CodeIterator::dll_cache() {
    return reinterpret_cast<Interpreted_DLLCache *>( align( _current + 1 ) );
}


InterpretedPrimitiveCache * CodeIterator::primitive_cache() {
    return reinterpret_cast<InterpretedPrimitiveCache *>(_current);
}


const char * CodeIterator::interpreter_return_point( bool_t restore_value ) const {
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


Oop * CodeIterator::block_method_addr() {
    switch ( code() ) {
        case ByteCodes::Code::push_new_closure_tos_0:      // fall through
        case ByteCodes::Code::push_new_closure_tos_1:      // fall through
        case ByteCodes::Code::push_new_closure_tos_2:      // fall through
        case ByteCodes::Code::push_new_closure_context_0:  // fall through
        case ByteCodes::Code::push_new_closure_context_1:  // fall through
        case ByteCodes::Code::push_new_closure_context_2:
            return aligned_oop( 1 );
        case ByteCodes::Code::push_new_closure_tos_n:      // fall through
        case ByteCodes::Code::push_new_closure_context_n:
            return aligned_oop( 2 );
    }
    return nullptr;
}


MethodOop CodeIterator::block_method() {
    switch ( code() ) {
        case ByteCodes::Code::push_new_closure_tos_0:      // fall through
        case ByteCodes::Code::push_new_closure_tos_1:      // fall through
        case ByteCodes::Code::push_new_closure_tos_2:      // fall through
        case ByteCodes::Code::push_new_closure_context_0:  // fall through
        case ByteCodes::Code::push_new_closure_context_1:  // fall through
        case ByteCodes::Code::push_new_closure_context_2:
            return MethodOop( oop_at( 1 ) );
        case ByteCodes::Code::push_new_closure_tos_n:      // fall through
        case ByteCodes::Code::push_new_closure_context_n:
            return MethodOop( oop_at( 2 ) );
    }
    return nullptr;
}


void CodeIterator::customize_class_var_code( KlassOop to_klass ) {
    st_assert( code() == ByteCodes::Code::push_classVar_name or code() == ByteCodes::Code::store_classVar_pop_name or code() == ByteCodes::Code::store_classVar_name, "must be class variable byte code" );

    Oop * p = aligned_oop( 1 );
    SymbolOop name = SymbolOop( *p );
    st_assert( name->is_symbol(), "name must be symbol" );
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
    st_assert( code() == ByteCodes::Code::push_classVar or code() == ByteCodes::Code::store_classVar_pop or code() == ByteCodes::Code::store_classVar, "must be class variable byte code" );

    Oop * p = aligned_oop( 1 );
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
    st_assert( code() == ByteCodes::Code::push_classVar or code() == ByteCodes::Code::store_classVar_pop or code() == ByteCodes::Code::store_classVar, "must be class variable byte code" );

    Oop * p = aligned_oop( 1 );
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

    Oop * p = aligned_oop( 1 );
    SymbolOop name = SymbolOop( *p );
    st_assert( name->is_symbol(), "name must be symbol" );
    int offset = to_klass->klass_part()->lookup_inst_var( name );
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

    Oop * p = aligned_oop( 1 );
    st_assert( ( *p )->is_smi(), "must be smi_t" );
    int       old_offset = SMIOop( *p )->value();
    SymbolOop name       = from_klass->klass_part()->inst_var_name_at( old_offset );
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

    Oop * p = aligned_oop( 1 );
    st_assert( ( *p )->is_smi(), "must be smi_t" );
    int       old_offset = SMIOop( *p )->value();
    SymbolOop name       = from_klass->klass_part()->inst_var_name_at( old_offset );
    if ( not name ) {
        st_fatal( "instance variable not found" );
    }
    int new_offset = to_klass->klass_part()->lookup_inst_var( name );
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
