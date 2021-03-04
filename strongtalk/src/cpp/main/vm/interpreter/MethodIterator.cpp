
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/MethodIterator.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/primitive/primitives.hpp"
#include "vm/interpreter/MethodClosure.hpp"
#include "vm/oop/AssociationOopDescriptor.hpp"


MethodIntervalFactory         MethodIterator::defaultFactory;
AbstractMethodIntervalFactory *MethodIterator::factory;


MethodIterator::MethodIterator( MethodOop m, MethodClosure *blk, AbstractMethodIntervalFactory *f ) :
    _interval{ nullptr } {
    factory   = f;
    _interval = factory->new_MethodInterval( m, nullptr );
    dispatch( blk );
}


MethodIterator::MethodIterator( MethodInterval *interval, MethodClosure *blk, AbstractMethodIntervalFactory *f ) :
    _interval{ nullptr } {
    factory   = f;
    _interval = interval;
    dispatch( blk );
}


void MethodIterator::unknown_code( std::uint8_t code ) {
    SPDLOG_INFO( "Unknown code found 0x{0:x}", code );
    st_fatal( "aborting" );
}


void MethodIterator::should_never_encounter( std::uint8_t code ) {
    SPDLOG_INFO( "Should never iterate through code 0x{0:x}", code );
    st_fatal( "aborting" );
}


static inline std::uint8_t map0to256( std::uint8_t ch ) {
    return ch ? ch : 256;
}


void MethodIterator::dispatch( MethodClosure *blk ) {

    bool oldFailState = blk->in_primitive_failure_block();
    blk->set_primitive_failure( _interval->in_primitive_failure_block() );
    CodeIterator iter( _interval->method(), _interval->begin_byteCodeIndex() );

    std::int32_t lastArgNo = _interval->method()->number_of_arguments() - 1;
    blk->set_method( _interval->method() );
    std::int32_t next_byteCodeIndex = _interval->begin_byteCodeIndex();

    while ( next_byteCodeIndex < _interval->end_byteCodeIndex() and not blk->aborting() ) {
        iter.set_byteCodeIndex( next_byteCodeIndex );
        blk->set_byteCodeIndex( iter.byteCodeIndex() );

        next_byteCodeIndex = iter.next_byteCodeIndex();
        blk->set_next_byteCodeIndex( next_byteCodeIndex );

        switch ( iter.code() ) {
            case ByteCodes::Code::push_temp_0:
                blk->push_temporary( 0 );
                break;
            case ByteCodes::Code::push_temp_1:
                blk->push_temporary( 1 );
                break;
            case ByteCodes::Code::push_temp_2:
                blk->push_temporary( 2 );
                break;
            case ByteCodes::Code::push_temp_3:
                blk->push_temporary( 3 );
                break;
            case ByteCodes::Code::push_temp_4:
                blk->push_temporary( 4 );
                break;
            case ByteCodes::Code::push_temp_5:
                blk->push_temporary( 5 );
                break;
            case ByteCodes::Code::unimplemented_06:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_06) );
                break;
            case ByteCodes::Code::push_temp_n:
                blk->push_temporary( 255 - iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::push_arg_1:
                blk->push_argument( lastArgNo );
                break;
            case ByteCodes::Code::push_arg_2:
                blk->push_argument( lastArgNo - 1 );
                break;
            case ByteCodes::Code::push_arg_3:
                blk->push_argument( lastArgNo - 2 );
                break;
            case ByteCodes::Code::push_arg_n:
                blk->push_argument( lastArgNo - iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::allocate_temp_1:
                blk->allocate_temporaries( 1 );
                break;
            case ByteCodes::Code::allocate_temp_2:
                blk->allocate_temporaries( 2 );
                break;
            case ByteCodes::Code::allocate_temp_3:
                blk->allocate_temporaries( 3 );
                break;
            case ByteCodes::Code::allocate_temp_n:
                blk->allocate_temporaries( map0to256( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::store_temp_0_pop:
                blk->store_temporary( 0 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_1_pop:
                blk->store_temporary( 1 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_2_pop:
                blk->store_temporary( 2 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_3_pop:
                blk->store_temporary( 3 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_4_pop:
                blk->store_temporary( 4 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_5_pop:
                blk->store_temporary( 5 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_n:
                blk->store_temporary( 255 - iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::store_temp_n_pop:
                blk->store_temporary( 255 - iter.byte_at( 1 ) );
                blk->pop();
                break;
            case ByteCodes::Code::push_neg_n:
                blk->push_literal( smiOopFromValue( -(std::int32_t) iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_succ_n:
                blk->push_literal( smiOopFromValue( iter.byte_at( 1 ) + 1 ) );
                break;
            case ByteCodes::Code::push_literal:
                blk->push_literal( iter.oop_at( 1 ) );
                break;
            case ByteCodes::Code::push_tos:
                blk->push_tos();
                break;
            case ByteCodes::Code::push_self:
                blk->push_self();
                break;
            case ByteCodes::Code::push_nil:
                blk->push_literal( nilObject );
                break;
            case ByteCodes::Code::push_true:
                blk->push_literal( trueObject );
                break;
            case ByteCodes::Code::push_false:
                blk->push_literal( falseObject );
                break;
            case ByteCodes::Code::unimplemented_20:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_20) );
                break;
            case ByteCodes::Code::unimplemented_21:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_21) );
                break;
            case ByteCodes::Code::unimplemented_22:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_22) );
                break;
            case ByteCodes::Code::unimplemented_23:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_23) );
                break;
            case ByteCodes::Code::unimplemented_24:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_24) );
                break;
            case ByteCodes::Code::unimplemented_25:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_25) );
                break;
            case ByteCodes::Code::unimplemented_26:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_26) );
                break;
            case ByteCodes::Code::unimplemented_27:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_27) );
                break;
            case ByteCodes::Code::return_instVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->push_instVar_name( name );
                blk->method_return( 0 );
            }
                break;
            case ByteCodes::Code::push_classVar: {
                AssociationOop assoc = AssociationOop( iter.oop_at( 1 ) );
                st_assert( assoc->is_association(), "must be association" );
                blk->push_classVar( assoc );
            }
                break;
            case ByteCodes::Code::store_classVar_pop: {
                AssociationOop assoc = AssociationOop( iter.oop_at( 1 ) );
                st_assert( assoc->is_association(), "must be association" );
                blk->store_classVar( assoc );
                blk->pop();
            }
                break;
            case ByteCodes::Code::store_classVar: {
                AssociationOop assoc = AssociationOop( iter.oop_at( 1 ) );
                st_assert( assoc->is_association(), "must be association" );
                blk->store_classVar( assoc );
            }
                break;
            case ByteCodes::Code::return_instVar: {
                SmallIntegerOop offset = SmallIntegerOop( iter.oop_at( 1 ) );
                st_assert( offset->isSmallIntegerOop(), "must be small_int_t" );
                blk->push_instVar( offset->value() );
                blk->method_return( 0 );
            }
                break;
            case ByteCodes::Code::push_instVar: {
                SmallIntegerOop offset = SmallIntegerOop( iter.oop_at( 1 ) );
                st_assert( offset->isSmallIntegerOop(), "must be small_int_t" );
                blk->push_instVar( offset->value() );
            }
                break;
            case ByteCodes::Code::store_instVar_pop: {
                SmallIntegerOop offset = SmallIntegerOop( iter.oop_at( 1 ) );
                st_assert( offset->isSmallIntegerOop(), "must be small_int_t" );
                blk->store_instVar( offset->value() );
                blk->pop();
            }
                break;
            case ByteCodes::Code::store_instVar: {
                SmallIntegerOop offset = SmallIntegerOop( iter.oop_at( 1 ) );
                st_assert( offset->isSmallIntegerOop(), "must be small_int_t" );
                blk->store_instVar( offset->value() );
            }
                break;
            case ByteCodes::Code::float_allocate:
                blk->allocate_temporaries( 1 + iter.byte_at( 1 ) * 2 );
                blk->float_allocate( iter.byte_at( 2 ), iter.byte_at( 3 ) );
                break;
            case ByteCodes::Code::float_floatify_pop:
                blk->float_floatify( Floats::Function::floatify, blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::float_move:
                blk->float_move( blk->float_at( iter.byte_at( 1 ) ), blk->float_at( iter.byte_at( 2 ) ) );
                break;
            case ByteCodes::Code::float_set:
                blk->float_set( blk->float_at( iter.byte_at( 1 ) ), *(DoubleOop *) iter.aligned_oop( 2 ) );
                break;
            case ByteCodes::Code::float_nullary_op:
                blk->float_nullary( Floats::Function( iter.byte_at( 2 ) ), blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::float_unary_op:
                blk->float_unary( Floats::Function( iter.byte_at( 2 ) ), blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::float_binary_op:
                blk->float_binary( Floats::Function( iter.byte_at( 2 ) ), blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::float_unary_op_to_oop:
                blk->float_unaryToOop( Floats::Function( iter.byte_at( 2 ) ), blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::float_binary_op_to_oop:
                blk->float_binaryToOop( Floats::Function( iter.byte_at( 2 ) ), blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::unimplemented_39:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_39) );
                break;
            case ByteCodes::Code::unimplemented_3a:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_3a) );
                break;
            case ByteCodes::Code::unimplemented_3b:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_3b) );
                break;
            case ByteCodes::Code::unimplemented_3c:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_3c) );
                break;
            case ByteCodes::Code::push_instVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->push_instVar_name( name );
            }
                break;
            case ByteCodes::Code::store_instVar_pop_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->store_instVar_name( name );
                blk->pop();
            }
                break;
            case ByteCodes::Code::store_instVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->store_instVar_name( name );
            }
                break;
            case ByteCodes::Code::push_temp_0_context_0:
                blk->push_temporary( 0, 0 );
                break;
            case ByteCodes::Code::push_temp_1_context_0:
                blk->push_temporary( 1, 0 );
                break;
            case ByteCodes::Code::push_temp_2_context_0:
                blk->push_temporary( 2, 0 );
                break;
            case ByteCodes::Code::push_temp_n_context_0:
                blk->push_temporary( iter.byte_at( 1 ), 0 );
                break;
            case ByteCodes::Code::store_temp_0_context_0_pop:
                blk->store_temporary( 0, 0 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_1_context_0_pop:
                blk->store_temporary( 1, 0 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_2_context_0_pop:
                blk->store_temporary( 2, 0 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_n_context_0_pop:
                blk->store_temporary( iter.byte_at( 1 ), 0 );
                blk->pop();
                break;
            case ByteCodes::Code::push_new_closure_context_0:
                blk->allocate_closure( AllocationType::context_as_scope, 0, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_context_1:
                blk->allocate_closure( AllocationType::context_as_scope, 1, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_context_2:
                blk->allocate_closure( AllocationType::context_as_scope, 2, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_context_n:
                blk->allocate_closure( AllocationType::context_as_scope, iter.byte_at( 1 ), MethodOop( iter.oop_at( 2 ) ) );
                break;
            case ByteCodes::Code::install_new_context_method_0:
                blk->allocate_context( 0, true );
                break;
            case ByteCodes::Code::install_new_context_method_1:
                blk->allocate_context( 1, true );
                break;
            case ByteCodes::Code::install_new_context_method_2:
                blk->allocate_context( 2, true );
                break;
            case ByteCodes::Code::install_new_context_method_n:
                blk->allocate_context( iter.byte_at( 1 ), true );
                break;
            case ByteCodes::Code::push_temp_0_context_1:
                blk->push_temporary( 0, 1 );
                break;
            case ByteCodes::Code::push_temp_1_context_1:
                blk->push_temporary( 1, 1 );
                break;
            case ByteCodes::Code::push_temp_2_context_1:
                blk->push_temporary( 2, 1 );
                break;
            case ByteCodes::Code::push_temp_n_context_1:
                blk->push_temporary( iter.byte_at( 1 ), 1 );
                break;
            case ByteCodes::Code::store_temp_0_context_1_pop:
                blk->store_temporary( 0, 1 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_1_context_1_pop:
                blk->store_temporary( 1, 1 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_2_context_1_pop:
                blk->store_temporary( 2, 1 );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_n_context_1_pop:
                blk->store_temporary( iter.byte_at( 1 ), 1 );
                blk->pop();
                break;
            case ByteCodes::Code::push_new_closure_tos_0:
                blk->allocate_closure( AllocationType::tos_as_scope, 0, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_tos_1:
                blk->allocate_closure( AllocationType::tos_as_scope, 1, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_tos_2:
                blk->allocate_closure( AllocationType::tos_as_scope, 2, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_tos_n:
                blk->allocate_closure( AllocationType::tos_as_scope, iter.byte_at( 1 ), MethodOop( iter.oop_at( 2 ) ) );
                break;
            case ByteCodes::Code::only_pop:
                blk->pop();
                break;
            case ByteCodes::Code::install_new_context_block_1:
                blk->allocate_context( 1, false );
                break;
            case ByteCodes::Code::install_new_context_block_2:
                blk->allocate_context( 2, false );
                break;
            case ByteCodes::Code::install_new_context_block_n:
                blk->allocate_context( iter.byte_at( 1 ), false );
                break;
            case ByteCodes::Code::push_temp_0_context_n:
                blk->push_temporary( 0, iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::push_temp_1_context_n:
                blk->push_temporary( 1, iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::push_temp_2_context_n:
                blk->push_temporary( 2, iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::push_temp_n_context_n:
                blk->push_temporary( iter.byte_at( 1 ), map0to256( iter.byte_at( 2 ) ) );
                break;
            case ByteCodes::Code::store_temp_0_context_n_pop:
                blk->store_temporary( 0, iter.byte_at( 1 ) );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_1_context_n_pop:
                blk->store_temporary( 1, iter.byte_at( 1 ) );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_2_context_n_pop:
                blk->store_temporary( 2, iter.byte_at( 1 ) );
                blk->pop();
                break;
            case ByteCodes::Code::store_temp_n_context_n_pop:
                blk->store_temporary( iter.byte_at( 1 ), map0to256( iter.byte_at( 2 ) ) );
                blk->pop();
                break;
            case ByteCodes::Code::set_self_via_context:
                blk->set_self_via_context();
                break;
            case ByteCodes::Code::copy_1_into_context:
                blk->copy_argument_into_context( lastArgNo - iter.byte_at( 1 ), 0 );
                break;
            case ByteCodes::Code::copy_2_into_context:
                blk->copy_argument_into_context( lastArgNo - iter.byte_at( 1 ), 0 );
                blk->copy_argument_into_context( lastArgNo - iter.byte_at( 2 ), 1 );
                break;
            case ByteCodes::Code::copy_n_into_context: {
                std::int32_t       len = map0to256( iter.byte_at( 1 ) );
                for ( std::int32_t i   = 0; i < len; i++ )
                    blk->copy_argument_into_context( lastArgNo - iter.byte_at( i + 2 ), i );
                break;
            }
            case ByteCodes::Code::copy_self_into_context:
                blk->copy_self_into_context();
                break;
            case ByteCodes::Code::copy_self_1_into_context:
                blk->copy_self_into_context();
                blk->copy_argument_into_context( lastArgNo - iter.byte_at( 1 ), 1 );
                break;
            case ByteCodes::Code::copy_self_2_into_context:
                blk->copy_self_into_context();
                blk->copy_argument_into_context( lastArgNo - iter.byte_at( 1 ), 1 );
                blk->copy_argument_into_context( lastArgNo - iter.byte_at( 2 ), 2 );
                break;
            case ByteCodes::Code::copy_self_n_into_context: {
                blk->copy_self_into_context();
                std::int32_t       len = map0to256( iter.byte_at( 1 ) );
                for ( std::int32_t i   = 0; i < len; i++ )
                    blk->copy_argument_into_context( lastArgNo - iter.byte_at( i + 2 ), i + 1 );
                break;
            }
            case ByteCodes::Code::ifTrue_byte: {
                IfNode *node = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), true, iter.byte_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::ifTrue_byte: just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::ifFalse_byte: {
                IfNode *node       = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), false, iter.byte_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::ifFalse_byte: just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::and_byte: {
                AndNode *node      = MethodIterator::factory->new_AndNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::and_byte: just checking" );
                blk->cond_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::or_byte: {
                OrNode *node       = MethodIterator::factory->new_OrNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::or_byte: just checking" );
                blk->cond_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::whileTrue_byte:
                // ignore since they are inside WhileNode expression body
                break;
            case ByteCodes::Code::whileFalse_byte:
                // ignore since they are inside WhileNode expression body
                break;
            case ByteCodes::Code::jump_else_byte:
                should_never_encounter( static_cast<std::uint8_t>(ByteCodes::Code::jump_else_byte) );
                break;
            case ByteCodes::Code::jump_loop_byte: {
                WhileNode *node = MethodIterator::factory->new_WhileNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.byte_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::jump_loop_byte: just checking" );
                blk->while_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::ifTrue_word: {
                IfNode *node       = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), true, iter.word_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::ifTrue_word: just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::ifFalse_word: {
                IfNode *node       = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), false, iter.word_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::ifFalse_word: just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::and_word: {
                AndNode *node      = MethodIterator::factory->new_AndNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.word_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::and_word: just checking" );
                blk->cond_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::or_word: {
                OrNode *node       = MethodIterator::factory->new_OrNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.word_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::or_word: just checking" );
                blk->cond_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::whileTrue_word:
                // Ignore since they are inside WhileNode expression body
                break;
            case ByteCodes::Code::whileFalse_word:
                // Ignore since they are inside WhileNode expression body
                break;
            case ByteCodes::Code::jump_else_word:
                should_never_encounter( static_cast<std::uint8_t>(ByteCodes::Code::jump_else_word) );
                break;
            case ByteCodes::Code::jump_loop_word: {
                WhileNode *node = MethodIterator::factory->new_WhileNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.word_at( 1 + OOP_SIZE ), iter.word_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::jump_loop_word: just checking" );
                blk->while_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::interpreted_send_0:
                [[fallthrough]];
            case ByteCodes::Code::interpreted_send_1:
                [[fallthrough]];
            case ByteCodes::Code::interpreted_send_2:
                [[fallthrough]];
            case ByteCodes::Code::interpreted_send_n:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_0:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_1:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_2:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_n:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_0:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_1:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_2:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_n:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_0:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_1:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_2:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_n:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_0:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_1:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_2:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_n:
//                [[fallthrough]];
                blk->normal_send( iter.ic() );
                break;
            case ByteCodes::Code::interpreted_send_0_pop:
                [[fallthrough]];
            case ByteCodes::Code::interpreted_send_1_pop:
                [[fallthrough]];
            case ByteCodes::Code::interpreted_send_2_pop:
                [[fallthrough]];
            case ByteCodes::Code::interpreted_send_n_pop:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_0_pop:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_1_pop:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_2_pop:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_n_pop:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_0_pop:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_1_pop:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_2_pop:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_n_pop:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_0_pop:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_1_pop:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_2_pop:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_n_pop:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_0_pop:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_1_pop:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_2_pop:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_n_pop:
                blk->normal_send( iter.ic() );
                blk->pop();
                break;
            case ByteCodes::Code::interpreted_send_self:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_self:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_self:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_self:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_self:
                blk->self_send( iter.ic() );
                break;
            case ByteCodes::Code::interpreted_send_self_pop:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_self_pop:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_self_pop:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_self_pop:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_self_pop:
                blk->self_send( iter.ic() );
                blk->pop();
                break;
            case ByteCodes::Code::interpreted_send_super:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_super:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_super:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_super:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_super:
                blk->super_send( iter.ic() );
                break;
            case ByteCodes::Code::interpreted_send_super_pop:
                [[fallthrough]];
            case ByteCodes::Code::compiled_send_super_pop:
                [[fallthrough]];
            case ByteCodes::Code::primitive_send_super_pop:
                [[fallthrough]];
            case ByteCodes::Code::polymorphic_send_super_pop:
                [[fallthrough]];
            case ByteCodes::Code::megamorphic_send_super_pop:
                blk->super_send( iter.ic() );
                blk->pop();
                break;
            case ByteCodes::Code::return_tos_pop_0:
                blk->method_return( 0 );
                break;
            case ByteCodes::Code::return_tos_pop_1:
                blk->method_return( 1 );
                break;
            case ByteCodes::Code::return_tos_pop_2:
                blk->method_return( 2 );
                break;
            case ByteCodes::Code::return_tos_pop_n:
                blk->method_return( iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::return_self_pop_0:
                blk->push_self();
                blk->method_return( 0 );
                break;
            case ByteCodes::Code::return_self_pop_1:
                blk->push_self();
                blk->method_return( 1 );
                break;
            case ByteCodes::Code::return_self_pop_2:
                blk->push_self();
                blk->method_return( 2 );
                break;
            case ByteCodes::Code::return_self_pop_n:
                blk->push_self();
                blk->method_return( iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::return_tos_zap_pop_n:
                blk->zap_scope();
                blk->method_return( iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::return_self_zap_pop_n:
                blk->zap_scope();
                blk->push_self();
                blk->method_return( iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::non_local_return_tos_pop_n:
                blk->nonlocal_return( iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::non_local_return_self_pop_n:
                blk->push_self();
                blk->nonlocal_return( iter.byte_at( 1 ) );
                break;
            case ByteCodes::Code::prim_call:
                [[fallthrough]];
            case ByteCodes::Code::primitive_call_self: {
                PrimitiveDescriptor *pdesc = Primitives::lookup( (primitiveFunctionType) iter.word_at( 1 ) );
                PrimitiveCallNode   *node  = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), pdesc->has_receiver(), nullptr, pdesc );
                // %hack: this assertion fails
                // assert(pdesc->has_receiver() == (iter.code() == ByteCodes::Code::primitive_call_self), "just checking");
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::primitive_call_self: just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call:
                blk->predict_primitive_call( Primitives::lookup( (primitiveFunctionType) iter.word_at( 1 ) ), -1 );
                break;
            case ByteCodes::Code::primitive_call_failure:
                [[fallthrough]];
            case ByteCodes::Code::primitive_call_self_failure: {
                PrimitiveDescriptor *pdesc = Primitives::lookup( (primitiveFunctionType) iter.word_at( 1 ) );
                PrimitiveCallNode   *node  = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), pdesc->has_receiver(), nullptr, pdesc, iter.word_at( 1 + OOP_SIZE ) );
                st_assert( pdesc->has_receiver() == ( iter.code() == ByteCodes::Code::primitive_call_self_failure ), "just checking" );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::primitive_call_self_failure: just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call_failure:
                blk->predict_primitive_call( Primitives::lookup( (primitiveFunctionType) iter.word_at( 1 ) ), iter.next_byteCodeIndex() + iter.word_at( 1 + OOP_SIZE ) );
                break;
            case ByteCodes::Code::dll_call_sync: {
                DLLCallNode *node = MethodIterator::factory->new_DLLCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.dll_cache() );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::dll_call_sync: just checking" );
                blk->dll_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::access_send_self:
                blk->self_send( iter.ic() );
                break;
            case ByteCodes::Code::unimplemented_bc:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_bc) );
                break;
            case ByteCodes::Code::primitive_call_lookup:
                [[fallthrough]];
            case ByteCodes::Code::primitive_call_self_lookup: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "name must be SymbolOop" );
                PrimitiveCallNode *node = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.code() == ByteCodes::Code::primitive_call_self_lookup, name, nullptr );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::primitive_call_self_lookup: just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call_lookup:
                blk->predict_primitive_call( Primitives::lookup( SymbolOop( iter.word_at( 1 ) ) ), -1 );
                break;
            case ByteCodes::Code::primitive_call_failure_lookup:
                [[fallthrough]];
            case ByteCodes::Code::primitive_call_self_failure_lookup: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "name must be SymbolOop" );
                PrimitiveCallNode *node = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.code() == ByteCodes::Code::primitive_call_self_failure_lookup, name, nullptr, iter.word_at( 1 + OOP_SIZE ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::primitive_call_self_failure_lookup just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call_failure_lookup:
                blk->predict_primitive_call( Primitives::lookup( SymbolOop( iter.word_at( 1 ) ) ), iter.byteCodeIndex() + iter.word_at( 1 + OOP_SIZE ) );
                break;
            case ByteCodes::Code::dll_call_async: {
                DLLCallNode *node = MethodIterator::factory->new_DLLCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.dll_cache() );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "ByteCodes::Code::dll_call_async: just checking" );
                blk->dll_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::unimplemented_c7:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_c7) );
                break;
            case ByteCodes::Code::access_send_0:
                blk->normal_send( iter.ic() );
                break;
            case ByteCodes::Code::unimplemented_cc:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_cc) );
                break;
            case ByteCodes::Code::unimplemented_dc:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_dc) );
                break;
            case ByteCodes::Code::special_primitive_send_1_hint:
                // ignore - only meaningfull for the interpreter
                break;
            case ByteCodes::Code::unimplemented_de:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_de) );
                break;
            case ByteCodes::Code::unimplemented_df:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_df) );
                break;
            case ByteCodes::Code::smi_add:
                [[fallthrough]];
            case ByteCodes::Code::smi_sub:
                [[fallthrough]];
            case ByteCodes::Code::smi_mult:
                [[fallthrough]];
            case ByteCodes::Code::smi_div:
                [[fallthrough]];
            case ByteCodes::Code::smi_mod:
                [[fallthrough]];
            case ByteCodes::Code::smi_create_point:
                [[fallthrough]];
            case ByteCodes::Code::smi_equal:
                [[fallthrough]];
            case ByteCodes::Code::smi_not_equal:
                [[fallthrough]];
            case ByteCodes::Code::smi_less:
                [[fallthrough]];
            case ByteCodes::Code::smi_less_equal:
                [[fallthrough]];
            case ByteCodes::Code::smi_greater:
                [[fallthrough]];
            case ByteCodes::Code::smi_greater_equal:
                [[fallthrough]];
            case ByteCodes::Code::objectArray_at:
                [[fallthrough]];
            case ByteCodes::Code::objectArray_at_put:
                [[fallthrough]];
            case ByteCodes::Code::smi_and:
                [[fallthrough]];
            case ByteCodes::Code::smi_or:
                [[fallthrough]];
            case ByteCodes::Code::smi_xor:
                [[fallthrough]];
            case ByteCodes::Code::smi_shift:
                blk->normal_send( iter.ic() );
                break;
            case ByteCodes::Code::double_equal:
                blk->double_equal();
                break;
            case ByteCodes::Code::double_tilde:
                blk->double_not_equal();
                break;
            case ByteCodes::Code::push_global:
                st_assert( iter.oop_at( 1 )->is_association(), "must be an association" );
                blk->push_global( AssociationOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::store_global_pop:
                st_assert( iter.oop_at( 1 )->is_association(), "must be an association" );
                blk->store_global( AssociationOop( iter.oop_at( 1 ) ) );
                blk->pop();
                break;
            case ByteCodes::Code::store_global:
                st_assert( iter.oop_at( 1 )->is_association(), "must be an association" );
                blk->store_global( AssociationOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_classVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->push_classVar_name( name );
                break;
            }
            case ByteCodes::Code::store_classVar_pop_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->store_classVar_name( name );
                blk->pop();
                break;
            }
            case ByteCodes::Code::store_classVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->isSymbol(), "must be symbol" );
                blk->store_classVar_name( name );
                break;
            }
            case ByteCodes::Code::unimplemented_fa:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_fa) );
                break;
            case ByteCodes::Code::unimplemented_fb:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_fb) );
                break;
            case ByteCodes::Code::unimplemented_fc:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_fc) );
                break;
            case ByteCodes::Code::unimplemented_fd:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_fd) );
                break;
            case ByteCodes::Code::unimplemented_fe:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::unimplemented_fe) );
                break;
            case ByteCodes::Code::halt:
                unknown_code( static_cast<std::uint8_t>(ByteCodes::Code::halt) );
                break;
            default: ShouldNotReachHere();
        }
    }

    blk->set_primitive_failure( oldFailState );
}
