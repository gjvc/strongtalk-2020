
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/vmSymbols.hpp"
#include "vm/interpreter/MethodNode.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/system/dll.hpp"


// InlineSendNode

InlineSendNode::InlineSendNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex ) :
    MethodInterval( method, parent, begin_byteCodeIndex, end_byteCodeIndex ) {
}


// CondNode

CondNode::CondNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) :
    InlineSendNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex + dest_offset ),
    _expr_code{ nullptr } {
    _expr_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, end_byteCodeIndex() );
}


// AndNode

AndNode::AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) :
    CondNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset ) {
}


SymbolOop AndNode::selector() const {
    return vmSymbols::and_();
}


// OrNode

OrNode::OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) :
    CondNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset ) {
}


SymbolOop OrNode::selector() const {
    return vmSymbols::or_();
}


// WhileNode

WhileNode::WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset ) :
    InlineSendNode( method, parent, begin_byteCodeIndex ),
    _cond{ false },
    _expr_code{ nullptr },
    _body_code{ nullptr } {

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
        default:
            st_fatal( "expecting while jump" );
    }
    std::int32_t jump_end = c.next_byteCodeIndex();

    if ( cond_offset == 0 ) {
        _body_code = nullptr;
        _expr_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, jump_end );
    } else {
        std::int32_t cond_dest = next_byteCodeIndex + cond_offset;
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

IfNode::IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool cond, std::int32_t else_offset, std::uint8_t structure ) :
    InlineSendNode( method, parent, begin_byteCodeIndex ),
    _cond{},
    _produces_result{},
    _ignore_else_while_printing{},
    _then_code{},
    _else_code{} {
    bool         has_else_branch;
    std::int32_t else_jump_size;
    _cond                       = cond;
    _produces_result            = isBitSet( structure, 0 );
    has_else_branch             = isBitSet( structure, 1 );
    _ignore_else_while_printing = isBitSet( structure, 2 );
    else_jump_size              = structure >> 4;

    if ( has_else_branch ) {
        std::int32_t else_jump = next_byteCodeIndex + else_offset - else_jump_size;
        _then_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, else_jump );
        CodeIterator c( method, else_jump );
        std::int32_t end_offset{ 0 };
        switch ( c.code() ) {
            case ByteCodes::Code::jump_else_byte:
                end_offset = c.byte_at( 1 );
                break;
            case ByteCodes::Code::jump_else_word:
                end_offset = c.word_at( 1 );
                break;
            default:
                st_fatal( "expecting an else jump" );
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

ExternalCallNode::ExternalCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex ) :
    MethodInterval( method, parent, begin_byteCodeIndex, next_byteCodeIndex ), _failure_code{ nullptr } {
}


ExternalCallNode::ExternalCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t end_offset ) :
    MethodInterval( method, parent, begin_byteCodeIndex, next_byteCodeIndex + end_offset ), _failure_code{ nullptr } {
    st_assert( end_offset > 0, "wrong offset" );
    _failure_code = MethodIterator::factory->new_MethodInterval( method, this, next_byteCodeIndex, end_byteCodeIndex(), true );
}


// PrimitiveCallNode

PrimitiveCallNode::PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc ) :
    ExternalCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex ),
    _name{ nullptr },
    _has_receiver{ has_receiver },
    _pdesc{} {

    st_assert( ( name == nullptr ) not_eq ( pdesc == nullptr ), "we need one an only one kind" );
    _name  = ( name == nullptr ) ? pdesc->selector() : name;
    _pdesc = ( pdesc == nullptr ) ? Primitives::lookup( name ) : pdesc;
}

// DLLCallNode

PrimitiveCallNode::PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset ) :
    ExternalCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, end_offset ),
    _name{ nullptr },
    _has_receiver{ has_receiver },
    _pdesc{} {

    st_assert( ( name == nullptr ) not_eq ( pdesc == nullptr ), "we need one an only one kind" );
    _name  = ( name == nullptr ) ? pdesc->selector() : name;
    _pdesc = ( pdesc == nullptr ) ? Primitives::lookup( name ) : pdesc;
}


std::int32_t PrimitiveCallNode::number_of_parameters() const {
    std::int32_t result = name()->number_of_arguments() + ( has_receiver() ? 1 : 0 ) - ( failure_code() ? 1 : 0 );
    st_assert( _pdesc == nullptr or pdesc()->number_of_parameters() == result, "checking result" );
    return result;
}


void DLLCallNode::initialize( Interpreted_DLLCache *cache ) {
    _dll_name      = cache->dll_name();
    _function_name = cache->funct_name();
    _nofArgs       = cache->number_of_arguments();
    _function      = cache->entry_point();
    _async         = cache->async();
}


DLLCallNode::DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache ) :
    ExternalCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex ),
    _dll_name{},
    _function_name{},
    _nofArgs{ 0 },
    _function{},
    _async{ false } {
    initialize( cache );
}
