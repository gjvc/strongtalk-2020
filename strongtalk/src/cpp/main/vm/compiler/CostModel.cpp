//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/compiler/CostModel.hpp"


std::int32_t CostModel::_cost[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];


void CostModel::set_default_costs() {
    // default cost for most codes
    set_cost_for_all( 1 );

    // specialize for individual code types
    set_cost_for_type( ByteCodes::CodeType::new_closure, 3 );
    set_cost_for_type( ByteCodes::CodeType::new_context, 3 );
    set_cost_for_type( ByteCodes::CodeType::message_send, 5 );
    set_cost_for_type( ByteCodes::CodeType::nonlocal_return, 3 );
    set_cost_for_type( ByteCodes::CodeType::primitive_call, 5 );
    set_cost_for_type( ByteCodes::CodeType::dll_call, 5 );

    // specialize for individual send types
    set_cost_for_send( ByteCodes::SendType::predicted_send, 2 );
    set_cost_for_send( ByteCodes::SendType::accessor_send, 2 );
}


void CostModel::set_cost_for_all( std::int32_t cost ) {
    for ( std::int32_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        _cost[ i ] = cost;
    }
}


void CostModel::set_cost_for_code( ByteCodes::Code code, std::int32_t cost ) {
    st_assert( ByteCodes::is_defined( code ), "undefined bytecode" );
    _cost[ static_cast<std::int32_t>(code) ] = cost;
}


void CostModel::set_cost_for_type( ByteCodes::CodeType type, std::int32_t cost ) {
    for ( std::int32_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        if ( ByteCodes::code_type( ByteCodes::Code( i ) ) == type ) {
            _cost[ i ] = cost;
        }
    }
}


void CostModel::set_cost_for_send( ByteCodes::SendType type, std::int32_t cost ) {
    for ( std::int32_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        if ( ByteCodes::send_type( ByteCodes::Code( i ) ) == type ) {
            _cost[ i ] = cost;
        }
    }
}


void CostModel::print() {

    _console->print_cr( "%40s: %s", "Bytecode", "Cost" );
    for ( std::int32_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES ); i++ ) {
        ByteCodes::Code code = ByteCodes::Code( i );
        if ( ByteCodes::is_defined( code ) ) {
            _console->print_cr( "%40s: %d", ByteCodes::name( code ), cost_for( code ) );
        }
    }
}


void costModel_init() {
    _console->print_cr( "%%system-init:  costModel_init" );

    CostModel::set_default_costs();
}
