//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/interpreter/interpreterStatistics.hpp"
#include "vm/interpreter/ByteCodes.hpp"


bool_t       InterpreterStatistics::_is_initialized = false;
uint32_t     InterpreterStatistics::_bytecode_counters[static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES)];
int          InterpreterStatistics::_bytecode_generation_order[static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES)];


void InterpreterStatistics::reset_bytecode_counters() {
    for ( int i = 0; i < static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES); i++ )
        _bytecode_counters[ i ] = 0;
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
