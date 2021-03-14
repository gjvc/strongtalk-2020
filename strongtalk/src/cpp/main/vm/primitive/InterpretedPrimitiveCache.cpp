
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/interpreter/CodeIterator.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/primitive/block_primitives.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/primitive/DoubleByteArray_primitives.hpp"
#include "vm/primitive/PrimitiveDescriptor.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/ResourceMark.hpp"


PrimitiveDescriptor *InterpretedPrimitiveCache::pdesc() const {

    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_self:
        case ByteCodes::Code::primitive_call_self_failure:
            return Primitives::lookup( (primitiveFunctionType) c.word_at( 1 ) );

        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_failure_lookup:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return Primitives::lookup( SymbolOop( c.oop_at( 1 ) ) );

        default: st_fatal( "Wrong bytecode" );
    }
    return nullptr;
}


bool InterpretedPrimitiveCache::has_receiver() const {
    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::primitive_call_self:
        case ByteCodes::Code::primitive_call_self_failure:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return true;

        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_failure_lookup:
            return false;

        default: st_fatal( "Wrong bytecode" );
    }
    return false;
}


SymbolOop InterpretedPrimitiveCache::name() const {
    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_self:
        case ByteCodes::Code::primitive_call_self_failure:
            return Primitives::lookup( (primitiveFunctionType) c.word_at( 1 ) )->selector();

        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_failure_lookup:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return SymbolOop( c.oop_at( 1 ) );

        default: st_fatal( "Wrong bytecode" );
    }
    return nullptr;
}


std::int32_t InterpretedPrimitiveCache::number_of_parameters() const {
    std::int32_t result = name()->number_of_arguments() + ( has_receiver() ? 1 : 0 ) - ( has_failure_code() ? 1 : 0 );
    st_assert( pdesc() == nullptr or pdesc()->number_of_parameters() == result, "checking result" );
    return result;
}


bool InterpretedPrimitiveCache::has_failure_code() const {
    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_failure_lookup:
        case ByteCodes::Code::primitive_call_self_failure:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return true;

        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self:
            return false;

        default: st_fatal( "Wrong bytecode" );
    }
    return false;
}
