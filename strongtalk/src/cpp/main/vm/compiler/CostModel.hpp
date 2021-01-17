//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/interpreter/ByteCodes.hpp"


// CostModel holds the 'costs' for all bytecodes.
// These costs are used to estimate the inline cost for a given methodOop.

class CostModel : AllStatic {
    private:
        static int _cost[static_cast<int>(ByteCodes::Code::NUMBER_OF_CODES)];

    public:
        static int cost_for( ByteCodes::Code code ) {
            return _cost[ ( int ) code ];
        }


        static void set_default_costs();                                        // sets default costs
        static void set_cost_for_all( int cost );                               // sets cost for all instructions
        static void set_cost_for_code( ByteCodes::Code code, int cost );        // sets cost for individual instruction
        static void set_cost_for_type( ByteCodes::CodeType type, int cost );    // sets cost for all instructions of a certain type
        static void set_cost_for_send( ByteCodes::SendType send, int cost );    // sets cost for all sends of a certain type

        static void print();
};
