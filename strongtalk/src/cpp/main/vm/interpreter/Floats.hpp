
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/assembler/MacroAssembler.hpp"

// Floats describes the floating point operations of the interpreter and implements stub routines used to execute this operations.
// Note: The Floats stub routines are *inside* the interpreter code.

class Floats : AllStatic {

public:
    static constexpr smi_t magic                   = 0x0badbabe;
    static constexpr smi_t max_number_of_functions = 25;

    // Function codes for float operations.
    // When changing these codes, make sure that the bytecode compiler is updated as well!
    // (use delta +GenerateSmalltalk to generate an updated file-in file for Smalltalk).

    enum class Function {
        // nullary functions
        zero,               // float(i) := 0.0
        one,                // float(i) := 1.0

        // unary functions
        abs,                // float(i) := |float(i)|
        negated,            // float(i) := -float(i)
        squared,            // float(i) := float(i) * float(i)
        sqrt,               // float(i) := sqrt(float(i))
        sin,                // float(i) := sin(float(i))
        cos,                // float(i) := cos(float(i))
        tan,                // float(i) := tan(float(i))
        exp,                // float(i) := exp(float(i))
        ln,                 // float(i) := ln(float(i))

        // binary functions
        add,                // float(i) := float(i) + float(i + 1)
        subtract,           // float(i) := float(i) - float(i + 1)
        multiply,           // float(i) := float(i) * float(i + 1)
        divide,             // float(i) := float(i) / float(i + 1)
        modulo,             // float(i) := float(i) \ float(i + 1)

        // unary functions to Oop
        is_zero,            // tos := float(i) =  0.0
        is_not_zero,        // tos := float(i) ~= 1.0
        oopify,             // tos := Oop(float(i))

        // binary functions to Oop
        is_equal,           // tos := float(i) =  float(i + 1)
        is_not_equal,       // tos := float(i) ~= float(i + 1)
        is_less,            // tos := float(i) <  float(i + 1)
        is_less_equal,      // tos := float(i) <= float(i + 1)
        is_greater,         // tos := float(i) >  float(i + 1)
        is_greater_equal,   // tos := float(i) >= float(i + 1)

        number_of_functions,

        // debugging & other purposes only (not actually generated in the bytecodes)
        undefined,          // error handling
        floatify            // used only to find selectors (MethodIterator)
    };

private:
    static bool_t                                            _is_initialized;    // true if Floats has been initialized
    static std::array<const char *, max_number_of_functions> _function_names;

    static void generate_tst( MacroAssembler *masm, Assembler::Condition cc );

    static void generate_cmp( MacroAssembler *masm, Assembler::Condition cc );

    static void generate( MacroAssembler *masm, Function f );

public:
    // Dispatch (interpreter)
    // Note: _function_table is bigger than number_of_functions to catch illegal (byte) indices // XXX
    static std::array<const char *, max_number_of_functions> _function_table;

    // Debugging/Printing
    static const char *function_name_for( Function f );

    // Tells if there is a selector for the float operation
    static bool_t has_selector_for( Function f );

    // Returns the selector for the float operation; nullptr is there is no selector
    static SymbolOop selector_for( Function f );


    static Oop magic_value() {
        return SMIOop( magic );
    }


    // Initialization/debugging
    static bool_t is_initialized() {
        return _is_initialized;
    }


    static void init( MacroAssembler *masm );

    static void print();
};
