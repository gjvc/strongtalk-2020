//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/compiler/CostModel.hpp"


std::int32_t CostModel::_cost[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];


void CostModel::set_default_costs() {
    // default cost for most codes
    set_cost_for_all( 1 );

    // specialize for individual code types
    set_cost_for_type( ByteCodes::CodeType::NEW_CLOSURE, 3 );
    set_cost_for_type( ByteCodes::CodeType::NEW_CONTEXT, 3 );
    set_cost_for_type( ByteCodes::CodeType::MESSAGE_SEND, 5 );
    set_cost_for_type( ByteCodes::CodeType::NONLOCAL_RETURN, 3 );
    set_cost_for_type( ByteCodes::CodeType::PRIMITIVE_CALL, 5 );
    set_cost_for_type( ByteCodes::CodeType::DLL_CALL, 5 );

    // specialize for individual send types
    set_cost_for_send( ByteCodes::SendType::PREDICTED_SEND, 2 );
    set_cost_for_send( ByteCodes::SendType::ACCESSOR_SEND, 2 );
}


void CostModel::set_cost_for_all( std::int32_t cost ) {
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        _cost[ i ] = cost;
    }
}


void CostModel::set_cost_for_code( ByteCodes::Code code, std::int32_t cost ) {
    st_assert( ByteCodes::is_defined( code ), "undefined bytecode" );
    _cost[ static_cast<std::int32_t>(code) ] = cost;
}


void CostModel::set_cost_for_type( ByteCodes::CodeType type, std::int32_t cost ) {
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        if ( ByteCodes::code_type( ByteCodes::Code( i ) ) == type ) {
            _cost[ i ] = cost;
        }
    }
}


void CostModel::set_cost_for_send( ByteCodes::SendType type, std::int32_t cost ) {
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        if ( ByteCodes::send_type( ByteCodes::Code( i ) ) == type ) {
            _cost[ i ] = cost;
        }
    }
}


void CostModel::print() {

    SPDLOG_INFO( "{40:s}: {}", "Bytecode", "Cost" );
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        ByteCodes::Code code = ByteCodes::Code( i );
        if ( ByteCodes::is_defined( code ) ) {
            SPDLOG_INFO( "%40s: {}", ByteCodes::name( code ), cost_for( code ) );
        }
    }
}


void costModel_init() {
    SPDLOG_INFO( "system-init:  costModel_init" );

    CostModel::set_default_costs();
}
