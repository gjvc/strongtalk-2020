//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/assembler/MacroAssembler.hpp"

//
// GeneratedPrimitives contains the assembly primitives.
// Instead of relying on an assembler, they are generated during system initialization.
//
// Steps to add a new stub primitive:
//
// 1. add a new entry point (class variable)
// 2. add the corresponding entry point accessor (class method)
// 3. implement the primitive code generator
// 4. call the generator in init()
//

class PrimitivesGenerator : StackAllocatedObject {

private:

    MacroAssembler *masm;


    Address nil_addr() {
        return Address( int( &nilObj ), RelocationInformation::RelocationType::external_word_type );
    }


    Address true_addr() {
        return Address( int( &trueObj ), RelocationInformation::RelocationType::external_word_type );
    }


    Address false_addr() {
        return Address( int( &falseObj ), RelocationInformation::RelocationType::external_word_type );
    }


    Address smiKlass_addr() {
        return Address( int( &smiKlassObj ), RelocationInformation::RelocationType::external_word_type );
    }


    Address doubleKlass_addr() {
        return Address( int( &doubleKlassObj ), RelocationInformation::RelocationType::external_word_type );
    }


    Address contextKlass_addr() {
        return Address( int( &contextKlassObj ), RelocationInformation::RelocationType::external_word_type );
    }


    Label error_receiver_has_wrong_type;
    Label error_first_argument_has_wrong_type;
    Label error_division_by_zero;
    Label error_overflow;
    Label allocation_failure;

    void scavenge( int size );

    void test_for_scavenge( Register dst, int size, Label &need_scavenge );

protected:
    PrimitivesGenerator( MacroAssembler *_masm ) {
        masm = _masm;
    }


    enum arith_op {
        op_add, op_sub, op_mul, op_div
    };

    // add generators here
    void error_jumps();

    const char *smiOopPrimitives_add();

    const char *smiOopPrimitives_subtract();

    const char *smiOopPrimitives_multiply();

    const char *smiOopPrimitives_mod();

    const char *smiOopPrimitives_div();

    const char *smiOopPrimitives_quo();

    const char *smiOopPrimitives_remainder();

    const char *double_op( arith_op op );

    const char *double_from_smi();

    const char *primitiveNew( int n );

    const char *allocateBlock( int n );

    const char *allocateContext_var();

    const char *allocateContext( int n );

// slr perf testing
    const char *inline_allocation();

//
    friend class GeneratedPrimitives;
};


class GeneratedPrimitives : AllStatic {

private:
    static constexpr int _code_size = 1024 * 1024; // simply increase if too small (assembler will crash if too small)

    static bool_t _is_initialized;            // true if GeneratedPrimitives has been initialized
    //  static char _code[_code_size];		// the code buffer for the primitives
    static const char *_code;        // the code buffer for the primitives

    // add entry points here
    static const char *_allocateContext_var;

    static const char *_smiOopPrimitives_add;
    static const char *_smiOopPrimitives_subtract;
    static const char *_smiOopPrimitives_multiply;
    static const char *_smiOopPrimitives_mod;
    static const char *_smiOopPrimitives_div;
    static const char *_smiOopPrimitives_quo;
    static const char *_smiOopPrimitives_remainder;

    static const char *_double_add;
    static const char *_double_subtract;
    static const char *_double_multiply;
    static const char *_double_divide;
    static const char *_double_from_smi;

    static const char *_primitiveInlineAllocations;

    static std::array<const char *, 10> _primitiveValue;
    static std::array<const char *, 10> _primitiveNew;
    static std::array<const char *, 10> _allocateBlock;
    static std::array<const char *, 3>  _allocateContext;

    // helpers for generation and patch
    static const char *patch( const char *name, const char *entry_point );

    static const char *patch( const char *name, const char *entry_point, int argument );

    static Oop invoke( const char *op, Oop receiver, Oop argument );

    friend class PrimitivesGenerator;

public:

    static void set_primitiveValue( int n, const char *entry_point );

    // add entry point accessors here
    static const char *primitiveValue( int n );

    static const char *primitiveNew( int n );

    static const char *allocateBlock( int n );

    static const char *allocateContext( int n );    // -1 for variable size

    // Support for profiling
    static bool_t contains( const char *pc ) {
        return ( _code <= pc ) and ( pc < &_code[ _code_size ] );
    }


    // Support for compiler constant folding
    static Oop smiOopPrimitives_add( Oop receiver, Oop argument );

    static Oop smiOopPrimitives_subtract( Oop receiver, Oop argument );

    static Oop smiOopPrimitives_multiply( Oop receiver, Oop argument );

    static Oop smiOopPrimitives_mod( Oop receiver, Oop argument );

    static Oop smiOopPrimitives_div( Oop receiver, Oop argument );

    static Oop smiOopPrimitives_quo( Oop receiver, Oop argument );

    static Oop smiOopPrimitives_remainder( Oop receiver, Oop argument );

    static void patch_primitiveValue();

    static void init();                // must be called in system initialization phase
};
