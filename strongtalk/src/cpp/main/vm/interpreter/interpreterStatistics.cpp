
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/interpreter/interpreterStatistics.hpp"
#include "vm/interpreter/ByteCodes.hpp"

#include <array>

std::array <uint32_t, static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES)>InterpreterStatistics::_bytecode_counters;
std::array <int, static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES)>     InterpreterStatistics::_bytecode_generation_order;

bool_t       InterpreterStatistics::_is_initialized = false;


void InterpreterStatistics::reset_bytecode_counters() {
    for ( auto x : _bytecode_counters ) {
        x = 0;
    }
}


void InterpreterStatistics::reset_bytecode_generation_order() {
    for ( int i = 0; i < static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES); i++ )
        _bytecode_generation_order[ i ] = i;
}


ByteCodes::Code InterpreterStatistics::ith_bytecode_to_generate( int i ) {
    st_assert( 0 <= i and i < static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES), "illegal index" );
    return ByteCodes::Code( _bytecode_generation_order[ i ] );
}


void InterpreterStatistics::initialize() {
    if ( is_initialized() )
        return;
    reset_bytecode_counters();
    reset_bytecode_generation_order();
    _is_initialized = true;
}


bool_t InterpreterStatistics::is_initialized() {
    return _is_initialized;
}
