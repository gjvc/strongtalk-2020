//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/MethodIterator.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/system/dll.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/system/sizes.hpp"




// MethodInterval

MethodInterval::MethodInterval( MethodOop method, MethodInterval * parent ) {
    initialize( method, parent, 1, method->end_byteCodeIndex(), false );
}


MethodInterval::MethodInterval( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int end_byteCodeIndex, bool_t failBlock ) {
    initialize( method, parent, begin_byteCodeIndex, end_byteCodeIndex, failBlock );
}


void MethodInterval::initialize( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int end_byteCodeIndex, bool_t failBlock ) {
    _method               = method;
    _parent               = parent;
    _begin_byteCodeIndex  = begin_byteCodeIndex;
    _end_byteCodeIndex    = end_byteCodeIndex;
    _in_primitive_failure = failBlock;
    _info                 = nullptr;
}


// InlineSendNode

InlineSendNode::InlineSendNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int end_byteCodeIndex ) :
    MethodInterval( method, parent, begin_byteCodeIndex, end_byteCodeIndex ) {
}


// CondNode

CondNode::CondNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int dest_offset ) :
    InlineSendNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex + dest_offset ) {
    _expr_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, end_byteCodeIndex() );
}


// AndNode

AndNode::AndNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int dest_offset ) :
    CondNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset ) {
}


SymbolOop AndNode::selector() const {
    return vmSymbols::and_();
}


// OrNode

OrNode::OrNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int dest_offset ) :
    CondNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset ) {
}


SymbolOop OrNode::selector() const {
    return vmSymbols::or_();
}


// WhileNode

WhileNode::WhileNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int cond_offset, int end_offset ) :
    InlineSendNode( method, parent, begin_byteCodeIndex ) {

    CodeIterator c( method, next_byteCodeIndex + cond_offset + end_offset );
    switch ( c.code() ) {
        case ByteCodes::Code::whileTrue_byte:
        case ByteCodes::Code::whileTrue_word:
            _cond = true;
            break;
        case ByteCodes::Code::whileFalse_byte:
        case ByteCodes::Code::whileFalse_word:
            _cond = false;
            break;
        default: st_fatal( "expecting while jump" );
    }
    int jump_end = c.next_byteCodeIndex();

    if ( cond_offset == 0 ) {
        _body_code = nullptr;
        _expr_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, jump_end );
    } else {
        int cond_dest = next_byteCodeIndex + cond_offset;
        _body_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, cond_dest );
        _expr_code = MethodIterator::factory->new_MethodInterval( method, this, cond_dest, jump_end );
    }
    set_end_byteCodeIndex( expr_code()->end_byteCodeIndex() );
}


SymbolOop WhileNode::selector() const {
    if ( is_whileTrue() )
        return body_code() ? vmSymbols::while_true_() : vmSymbols::while_true();
    else
        return body_code() ? vmSymbols::while_false_() : vmSymbols::while_false();
}


// IfNode

IfNode::IfNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, bool_t cond, int else_offset, uint8_t structure ) :
    InlineSendNode( method, parent, begin_byteCodeIndex ) {
    bool_t has_else_branch;
    int    else_jump_size;
    _cond                       = cond;
    _produces_result            = isBitSet( structure, 0 );
    has_else_branch             = isBitSet( structure, 1 );
    _ignore_else_while_printing = isBitSet( structure, 2 );
    else_jump_size              = structure >> 4;

    if ( has_else_branch ) {
        int else_jump = next_byteCodeIndex + else_offset - else_jump_size;
        _then_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, else_jump );
        CodeIterator c( method, else_jump );
        int          end_offset;
        switch ( c.code() ) {
            case ByteCodes::Code::jump_else_byte:
                end_offset = c.byte_at( 1 );
                break;
            case ByteCodes::Code::jump_else_word:
                end_offset = c.word_at( 1 );
                break;
            default: st_fatal( "expecting an else jump" );
        }
        _else_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex + else_offset, next_byteCodeIndex + else_offset + end_offset );
        set_end_byteCodeIndex( else_code()->end_byteCodeIndex() );
    } else {
        _then_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, next_byteCodeIndex + else_offset );
        _else_code = nullptr;
        set_end_byteCodeIndex( then_code()->end_byteCodeIndex() );
    }
}


SymbolOop IfNode::selector() const {
    if ( is_ifTrue() )
        return else_code() ? vmSymbols::if_true_false() : vmSymbols::if_true();
    else
        return else_code() ? vmSymbols::if_false_true() : vmSymbols::if_false();
}


// ExternalCallNode

ExternalCallNode::ExternalCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex ) :
    MethodInterval( method, parent, begin_byteCodeIndex, next_byteCodeIndex ) {
    _failure_code = nullptr;
}


ExternalCallNode::ExternalCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int end_offset ) :
    MethodInterval( method, parent, begin_byteCodeIndex, next_byteCodeIndex + end_offset ) {
    st_assert( end_offset > 0, "wrong offset" );
    _failure_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, end_byteCodeIndex(), true );
}


// PrimitiveCallNode

PrimitiveCallNode::PrimitiveCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor * pdesc ) :
    ExternalCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex ) {
    st_assert( ( name == nullptr ) not_eq ( pdesc == nullptr ), "we need one an only one kind" );
    _has_receiver = has_receiver;
    _name         = ( name == nullptr ) ? pdesc->selector() : name;
    _pdesc        = ( pdesc == nullptr ) ? Primitives::lookup( name ) : pdesc;
}

// DLLCallNode

PrimitiveCallNode::PrimitiveCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor * pdesc, int end_offset ) :
    ExternalCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, end_offset ) {
    st_assert( ( name == nullptr ) not_eq ( pdesc == nullptr ), "we need one an only one kind" );
    _has_receiver = has_receiver;
    _name         = ( name == nullptr ) ? pdesc->selector() : name;
    _pdesc        = ( pdesc == nullptr ) ? Primitives::lookup( name ) : pdesc;
}


int PrimitiveCallNode::number_of_parameters() const {
    int result = name()->number_of_arguments() + ( has_receiver() ? 1 : 0 ) - ( failure_code() ? 1 : 0 );
    st_assert( _pdesc == nullptr or pdesc()->number_of_parameters() == result, "checking result" );
    return result;
}


void DLLCallNode::initialize( Interpreted_DLLCache * cache ) {
    _dll_name      = cache->dll_name();
    _function_name = cache->funct_name();
    _nofArgs       = cache->number_of_arguments();
    _function      = cache->entry_point();
    _async         = cache->async();
}


DLLCallNode::DLLCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, Interpreted_DLLCache * cache ) :
    ExternalCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex ) {
    initialize( cache );
}


// MethodClosure

MethodClosure::MethodClosure() {
    _method               = nullptr;
    _byteCodeIndex        = _next_byteCodeIndex = PrologueByteCodeIndex;
    _aborting             = false;
    _in_primitive_failure = false;
    _float0_index         = 0;
}


void MethodClosure::set_method( MethodOop method ) {
    _method = method;
    st_assert( method->number_of_stack_temporaries() % 2 == 0 or not method->has_float_temporaries(), "inconsistency" );
    _float0_index = 256 - method->number_of_stack_temporaries() / 2;
}


int MethodClosure::float_at( int index ) {
    int fno = _float0_index - index;
    st_assert( 0 <= fno and fno < _method->total_number_of_floats(), "illegal float number" );
    return fno;
}

// CustomizedMethodClosure

void CustomizedMethodClosure::push_instVar_name( SymbolOop name ) {
    st_fatal( "instance variable not resolved" );
}


void CustomizedMethodClosure::store_instVar_name( SymbolOop name ) {
    st_fatal( "instance variable not resolved" );
}


void CustomizedMethodClosure::push_classVar_name( SymbolOop name ) {
    st_fatal( "class variable not resolved" );
}


void CustomizedMethodClosure::store_classVar_name( SymbolOop name ) {
    st_fatal( "class variable not resolved" );
}


void CustomizedMethodClosure::push_classVar( AssociationOop assoc ) {
    push_global( assoc );
}


void CustomizedMethodClosure::store_classVar( AssociationOop assoc ) {
    store_global( assoc );
}

// SpecializedMethodClosure

void SpecializedMethodClosure::if_node( IfNode * node ) {
    MethodIterator iter( node->then_code(), this );
    if ( node->else_code() not_eq nullptr ) {
        MethodIterator iter( node->else_code(), this );
    }
}


void SpecializedMethodClosure::cond_node( CondNode * node ) {
    MethodIterator iter( node->expr_code(), this );
}


void SpecializedMethodClosure::while_node( WhileNode * node ) {
    MethodIterator iter( node->expr_code(), this );
    if ( node->body_code() not_eq nullptr ) {
        MethodIterator iter( node->body_code(), this );
    }
}


void SpecializedMethodClosure::primitive_call_node( PrimitiveCallNode * node ) {
    if ( node->failure_code() not_eq nullptr ) {
        MethodIterator iter( node->failure_code(), this );
    }
}


void SpecializedMethodClosure::dll_call_node( DLLCallNode * node ) {
    if ( node->failure_code() not_eq nullptr ) {
        MethodIterator iter( node->failure_code(), this );
    }
}

// MethodIterator

void MethodIterator::unknown_code( uint8_t code ) {
    _console->print_cr( "Unknown code found 0x%x", code );
    st_fatal( "aborting" );
}


void MethodIterator::should_never_encounter( uint8_t code ) {
    _console->print_cr( "Should never iterate through code 0x%x", code );
    st_fatal( "aborting" );
}


static inline uint8_t map0to256( uint8_t ch ) {
    return ch ? ch : 256;
}


void MethodIterator::dispatch( MethodClosure * blk ) {

    bool_t oldFailState = blk->in_primitive_failure_block();
    blk->set_primitive_failure( _interval->in_primitive_failure_block() );
    CodeIterator iter( _interval->method(), _interval->begin_byteCodeIndex() );

    int lastArgNo = _interval->method()->number_of_arguments() - 1;
    blk->set_method( _interval->method() );
    int next_byteCodeIndex = _interval->begin_byteCodeIndex();

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
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_06) );
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
                blk->push_literal( smiOopFromValue( -( int ) iter.byte_at( 1 ) ) );
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
                blk->push_literal( nilObj );
                break;
            case ByteCodes::Code::push_true:
                blk->push_literal( trueObj );
                break;
            case ByteCodes::Code::push_false:
                blk->push_literal( falseObj );
                break;
            case ByteCodes::Code::unimplemented_20:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_20) );
                break;
            case ByteCodes::Code::unimplemented_21:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_21) );
                break;
            case ByteCodes::Code::unimplemented_22:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_22) );
                break;
            case ByteCodes::Code::unimplemented_23:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_23) );
                break;
            case ByteCodes::Code::unimplemented_24:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_24) );
                break;
            case ByteCodes::Code::unimplemented_25:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_25) );
                break;
            case ByteCodes::Code::unimplemented_26:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_26) );
                break;
            case ByteCodes::Code::unimplemented_27:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_27) );
                break;
            case ByteCodes::Code::return_instVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "must be symbol" );
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
                SMIOop offset = SMIOop( iter.oop_at( 1 ) );
                st_assert( offset->is_smi(), "must be smi_t" );
                blk->push_instVar( offset->value() );
                blk->method_return( 0 );
            }
                break;
            case ByteCodes::Code::push_instVar: {
                SMIOop offset = SMIOop( iter.oop_at( 1 ) );
                st_assert( offset->is_smi(), "must be smi_t" );
                blk->push_instVar( offset->value() );
            }
                break;
            case ByteCodes::Code::store_instVar_pop: {
                SMIOop offset = SMIOop( iter.oop_at( 1 ) );
                st_assert( offset->is_smi(), "must be smi_t" );
                blk->store_instVar( offset->value() );
                blk->pop();
            }
                break;
            case ByteCodes::Code::store_instVar: {
                SMIOop offset = SMIOop( iter.oop_at( 1 ) );
                st_assert( offset->is_smi(), "must be smi_t" );
                blk->store_instVar( offset->value() );
            }
                break;
            case ByteCodes::Code::float_allocate:
                blk->allocate_temporaries( 1 + iter.byte_at( 1 ) * 2 );
                blk->float_allocate( iter.byte_at( 2 ), iter.byte_at( 3 ) );
                break;
            case ByteCodes::Code::float_floatify_pop:
                blk->float_floatify( Floats::floatify, blk->float_at( iter.byte_at( 1 ) ) );
                break;
            case ByteCodes::Code::float_move:
                blk->float_move( blk->float_at( iter.byte_at( 1 ) ), blk->float_at( iter.byte_at( 2 ) ) );
                break;
            case ByteCodes::Code::float_set:
                blk->float_set( blk->float_at( iter.byte_at( 1 ) ), *( DoubleOop * ) iter.aligned_oop( 2 ) );
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
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_39) );
                break;
            case ByteCodes::Code::unimplemented_3a:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_3a) );
                break;
            case ByteCodes::Code::unimplemented_3b:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_3b) );
                break;
            case ByteCodes::Code::unimplemented_3c:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_3c) );
                break;
            case ByteCodes::Code::push_instVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "must be symbol" );
                blk->push_instVar_name( name );
            }
                break;
            case ByteCodes::Code::store_instVar_pop_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "must be symbol" );
                blk->store_instVar_name( name );
                blk->pop();
            }
                break;
            case ByteCodes::Code::store_instVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "must be symbol" );
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
                blk->allocate_closure( context_as_scope, 0, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_context_1:
                blk->allocate_closure( context_as_scope, 1, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_context_2:
                blk->allocate_closure( context_as_scope, 2, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_context_n:
                blk->allocate_closure( context_as_scope, iter.byte_at( 1 ), MethodOop( iter.oop_at( 2 ) ) );
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
                blk->allocate_closure( tos_as_scope, 0, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_tos_1:
                blk->allocate_closure( tos_as_scope, 1, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_tos_2:
                blk->allocate_closure( tos_as_scope, 2, MethodOop( iter.oop_at( 1 ) ) );
                break;
            case ByteCodes::Code::push_new_closure_tos_n:
                blk->allocate_closure( tos_as_scope, iter.byte_at( 1 ), MethodOop( iter.oop_at( 2 ) ) );
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
                int       len = map0to256( iter.byte_at( 1 ) );
                for ( int i   = 0; i < len; i++ )
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
                int       len = map0to256( iter.byte_at( 1 ) );
                for ( int i   = 0; i < len; i++ )
                    blk->copy_argument_into_context( lastArgNo - iter.byte_at( i + 2 ), i + 1 );
                break;
            }
            case ByteCodes::Code::ifTrue_byte: {
                IfNode * node = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), true, iter.byte_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::ifFalse_byte: {
                IfNode * node      = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), false, iter.byte_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::and_byte: {
                AndNode * node     = MethodIterator::factory->new_AndNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->cond_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::or_byte: {
                OrNode * node      = MethodIterator::factory->new_OrNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
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
                should_never_encounter( static_cast<uint8_t>(ByteCodes::Code::jump_else_byte) );
                break;
            case ByteCodes::Code::jump_loop_byte: {
                WhileNode * node = MethodIterator::factory->new_WhileNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.byte_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->while_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::ifTrue_word: {
                IfNode * node      = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), true, iter.word_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::ifFalse_word: {
                IfNode * node      = MethodIterator::factory->new_IfNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), false, iter.word_at( 2 ), iter.byte_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->if_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::and_word: {
                AndNode * node     = MethodIterator::factory->new_AndNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.word_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->cond_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::or_word: {
                OrNode * node      = MethodIterator::factory->new_OrNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.word_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
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
                should_never_encounter( static_cast<uint8_t>(ByteCodes::Code::jump_else_word) );
                break;
            case ByteCodes::Code::jump_loop_word: {
                WhileNode * node = MethodIterator::factory->new_WhileNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.word_at( 1 + oopSize ), iter.word_at( 1 ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->while_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::interpreted_send_0:        // fall through
            case ByteCodes::Code::interpreted_send_1:        // fall through
            case ByteCodes::Code::interpreted_send_2:        // fall through
            case ByteCodes::Code::interpreted_send_n:        // fall through
            case ByteCodes::Code::compiled_send_0:            // fall through
            case ByteCodes::Code::compiled_send_1:            // fall through
            case ByteCodes::Code::compiled_send_2:            // fall through
            case ByteCodes::Code::compiled_send_n:            // fall through
            case ByteCodes::Code::primitive_send_0:            // fall through
            case ByteCodes::Code::primitive_send_1:            // fall through
            case ByteCodes::Code::primitive_send_2:            // fall through
            case ByteCodes::Code::primitive_send_n:            // fall through
            case ByteCodes::Code::polymorphic_send_0:        // fall through
            case ByteCodes::Code::polymorphic_send_1:        // fall through
            case ByteCodes::Code::polymorphic_send_2:        // fall through
            case ByteCodes::Code::polymorphic_send_n:        // fall through
            case ByteCodes::Code::megamorphic_send_0:        // fall through
            case ByteCodes::Code::megamorphic_send_1:        // fall through
            case ByteCodes::Code::megamorphic_send_2:        // fall through
            case ByteCodes::Code::megamorphic_send_n:        // fall through
                blk->normal_send( iter.ic() );
                break;
            case ByteCodes::Code::interpreted_send_0_pop:        // fall through
            case ByteCodes::Code::interpreted_send_1_pop:        // fall through
            case ByteCodes::Code::interpreted_send_2_pop:        // fall through
            case ByteCodes::Code::interpreted_send_n_pop:        // fall through
            case ByteCodes::Code::compiled_send_0_pop:        // fall through
            case ByteCodes::Code::compiled_send_1_pop:        // fall through
            case ByteCodes::Code::compiled_send_2_pop:        // fall through
            case ByteCodes::Code::compiled_send_n_pop:        // fall through
            case ByteCodes::Code::primitive_send_0_pop:        // fall through
            case ByteCodes::Code::primitive_send_1_pop:        // fall through
            case ByteCodes::Code::primitive_send_2_pop:        // fall through
            case ByteCodes::Code::primitive_send_n_pop:        // fall through
            case ByteCodes::Code::polymorphic_send_0_pop:        // fall through
            case ByteCodes::Code::polymorphic_send_1_pop:        // fall through
            case ByteCodes::Code::polymorphic_send_2_pop:        // fall through
            case ByteCodes::Code::polymorphic_send_n_pop:        // fall through
            case ByteCodes::Code::megamorphic_send_0_pop:        // fall through
            case ByteCodes::Code::megamorphic_send_1_pop:        // fall through
            case ByteCodes::Code::megamorphic_send_2_pop:        // fall through
            case ByteCodes::Code::megamorphic_send_n_pop:        // fall through
                blk->normal_send( iter.ic() );
                blk->pop();
                break;
            case ByteCodes::Code::interpreted_send_self:        // fall through
            case ByteCodes::Code::compiled_send_self:        // fall through
            case ByteCodes::Code::primitive_send_self:        // fall through
            case ByteCodes::Code::polymorphic_send_self:        // fall through
            case ByteCodes::Code::megamorphic_send_self:        // fall through
                blk->self_send( iter.ic() );
                break;
            case ByteCodes::Code::interpreted_send_self_pop:    // fall through
            case ByteCodes::Code::compiled_send_self_pop:        // fall through
            case ByteCodes::Code::primitive_send_self_pop:        // fall through
            case ByteCodes::Code::polymorphic_send_self_pop:    // fall through
            case ByteCodes::Code::megamorphic_send_self_pop:    // fall through
                blk->self_send( iter.ic() );
                blk->pop();
                break;
            case ByteCodes::Code::interpreted_send_super:        // fall through
            case ByteCodes::Code::compiled_send_super:        // fall through
            case ByteCodes::Code::primitive_send_super:        // fall through
            case ByteCodes::Code::polymorphic_send_super:        // fall through
            case ByteCodes::Code::megamorphic_send_super:        // fall through
                blk->super_send( iter.ic() );
                break;
            case ByteCodes::Code::interpreted_send_super_pop:    // fall through
            case ByteCodes::Code::compiled_send_super_pop:        // fall through
            case ByteCodes::Code::primitive_send_super_pop:        // fall through
            case ByteCodes::Code::polymorphic_send_super_pop:    // fall through
            case ByteCodes::Code::megamorphic_send_super_pop:    // fall through
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
            case ByteCodes::Code::prim_call: // fall through
            case ByteCodes::Code::primitive_call_self: {
                PrimitiveDescriptor * pdesc = Primitives::lookup( ( primitiveFunctionType ) iter.word_at( 1 ) );
                PrimitiveCallNode   * node  = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), pdesc->has_receiver(), nullptr, pdesc );
                // %hack: this assertion fails
                // assert(pdesc->has_receiver() == (iter.code() == ByteCodes::Code::primitive_call_self), "just checking");
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call:
                blk->predict_primitive_call( Primitives::lookup( ( primitiveFunctionType ) iter.word_at( 1 ) ), -1 );
                break;
            case ByteCodes::Code::primitive_call_failure: // fall through
            case ByteCodes::Code::primitive_call_self_failure: {
                PrimitiveDescriptor * pdesc = Primitives::lookup( ( primitiveFunctionType ) iter.word_at( 1 ) );
                PrimitiveCallNode   * node  = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), pdesc->has_receiver(), nullptr, pdesc, iter.word_at( 1 + oopSize ) );
                st_assert( pdesc->has_receiver() == ( iter.code() == ByteCodes::Code::primitive_call_self_failure ), "just checking" );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call_failure:
                blk->predict_primitive_call( Primitives::lookup( ( primitiveFunctionType ) iter.word_at( 1 ) ), iter.next_byteCodeIndex() + iter.word_at( 1 + oopSize ) );
                break;
            case ByteCodes::Code::dll_call_sync: {
                DLLCallNode * node = MethodIterator::factory->new_DLLCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.dll_cache() );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->dll_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::access_send_self:
                blk->self_send( iter.ic() );
                break;
            case ByteCodes::Code::unimplemented_bc:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_bc) );
                break;
            case ByteCodes::Code::primitive_call_lookup: // fall through
            case ByteCodes::Code::primitive_call_self_lookup: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "name must be SymbolOop" );
                PrimitiveCallNode * node = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.code() == ByteCodes::Code::primitive_call_self_lookup, name, nullptr );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call_lookup:
                blk->predict_primitive_call( Primitives::lookup( SymbolOop( iter.word_at( 1 ) ) ), -1 );
                break;
            case ByteCodes::Code::primitive_call_failure_lookup: // fall through
            case ByteCodes::Code::primitive_call_self_failure_lookup: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "name must be SymbolOop" );
                PrimitiveCallNode * node = MethodIterator::factory->new_PrimitiveCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.code() == ByteCodes::Code::primitive_call_self_failure_lookup, name, nullptr, iter.word_at( 1 + oopSize ) );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->primitive_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::predict_primitive_call_failure_lookup:
                blk->predict_primitive_call( Primitives::lookup( SymbolOop( iter.word_at( 1 ) ) ), iter.byteCodeIndex() + iter.word_at( 1 + oopSize ) );
                break;
            case ByteCodes::Code::dll_call_async: {
                DLLCallNode * node = MethodIterator::factory->new_DLLCallNode( _interval->method(), _interval, iter.byteCodeIndex(), iter.next_byteCodeIndex(), iter.dll_cache() );
                st_assert( node->end_byteCodeIndex() <= _interval->end_byteCodeIndex(), "just checking" );
                blk->dll_call_node( node );
                next_byteCodeIndex = node->end_byteCodeIndex();
                break;
            }
            case ByteCodes::Code::unimplemented_c7:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_c7) );
                break;
            case ByteCodes::Code::access_send_0:
                blk->normal_send( iter.ic() );
                break;
            case ByteCodes::Code::unimplemented_cc:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_cc) );
                break;
            case ByteCodes::Code::unimplemented_dc:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_dc) );
                break;
            case ByteCodes::Code::special_primitive_send_1_hint:
                // ignore - only meaningfull for the interpreter
                break;
            case ByteCodes::Code::unimplemented_de:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_de) );
                break;
            case ByteCodes::Code::unimplemented_df:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_df) );
                break;
            case ByteCodes::Code::smi_add: // fall through
            case ByteCodes::Code::smi_sub: // fall through
            case ByteCodes::Code::smi_mult: // fall through
            case ByteCodes::Code::smi_div: // fall through
            case ByteCodes::Code::smi_mod: // fall through
            case ByteCodes::Code::smi_create_point: // fall through
            case ByteCodes::Code::smi_equal: // fall through
            case ByteCodes::Code::smi_not_equal: // fall through
            case ByteCodes::Code::smi_less: // fall through
            case ByteCodes::Code::smi_less_equal: // fall through
            case ByteCodes::Code::smi_greater: // fall through
            case ByteCodes::Code::smi_greater_equal: // fall through
            case ByteCodes::Code::objArray_at: // fall through
            case ByteCodes::Code::objArray_at_put: // fall through
            case ByteCodes::Code::smi_and: // fall through
            case ByteCodes::Code::smi_or: // fall through
            case ByteCodes::Code::smi_xor: // fall through
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
                st_assert( name->is_symbol(), "must be symbol" );
                blk->push_classVar_name( name );
                break;
            }
            case ByteCodes::Code::store_classVar_pop_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "must be symbol" );
                blk->store_classVar_name( name );
                blk->pop();
                break;
            }
            case ByteCodes::Code::store_classVar_name: {
                SymbolOop name = SymbolOop( iter.oop_at( 1 ) );
                st_assert( name->is_symbol(), "must be symbol" );
                blk->store_classVar_name( name );
                break;
            }
            case ByteCodes::Code::unimplemented_fa:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_fa) );
                break;
            case ByteCodes::Code::unimplemented_fb:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_fb) );
                break;
            case ByteCodes::Code::unimplemented_fc:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_fc) );
                break;
            case ByteCodes::Code::unimplemented_fd:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_fd) );
                break;
            case ByteCodes::Code::unimplemented_fe:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::unimplemented_fe) );
                break;
            case ByteCodes::Code::halt:
                unknown_code( static_cast<uint8_t>(ByteCodes::Code::halt) );
                break;
            default: ShouldNotReachHere();
        }
    }

    blk->set_primitive_failure( oldFailState );
}


MethodIterator::MethodIterator( MethodOop m, MethodClosure * blk, AbstractMethodIntervalFactory * f ) {
    factory   = f;
    _interval = factory->new_MethodInterval( m, nullptr );
    dispatch( blk );
}


MethodIterator::MethodIterator( MethodInterval * interval, MethodClosure * blk, AbstractMethodIntervalFactory * f ) {
    factory   = f;
    _interval = interval;
    dispatch( blk );
}


MethodIntervalFactory         MethodIterator::defaultFactory;
AbstractMethodIntervalFactory * MethodIterator::factory;


MethodInterval * MethodIntervalFactory::new_MethodInterval( MethodOop method, MethodInterval * parent ) {
    return new MethodInterval( method, parent );
}


MethodInterval * MethodIntervalFactory::new_MethodInterval( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int end_byteCodeIndex, bool_t failureBlock ) {
    return new MethodInterval( method, parent, begin_byteCodeIndex, end_byteCodeIndex, failureBlock );
}


AndNode * MethodIntervalFactory::new_AndNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int dest_offset ) {
    return new AndNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset );
}


OrNode * MethodIntervalFactory::new_OrNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int dest_offset ) {
    return new OrNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset );
}


WhileNode * MethodIntervalFactory::new_WhileNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, int cond_offset, int end_offset ) {
    return new WhileNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, cond_offset, end_offset );
}


IfNode * MethodIntervalFactory::new_IfNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, bool_t cond, int else_offset, uint8_t structure ) {
    return new IfNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, cond, else_offset, structure );
}


PrimitiveCallNode * MethodIntervalFactory::new_PrimitiveCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor * pdesc ) {
    return new PrimitiveCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, has_receiver, name, pdesc );
}


PrimitiveCallNode * MethodIntervalFactory::new_PrimitiveCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor * pdesc, int end_offset ) {
    return new PrimitiveCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, has_receiver, name, pdesc, end_offset );
}


DLLCallNode * MethodIntervalFactory::new_DLLCallNode( MethodOop method, MethodInterval * parent, int begin_byteCodeIndex, int next_byteCodeIndex, Interpreted_DLLCache * cache ) {
    return new DLLCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, cache );
}
