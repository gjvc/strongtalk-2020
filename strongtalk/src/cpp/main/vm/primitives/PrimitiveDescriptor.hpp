
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/primitives/PrimitiveGroup.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Expression.hpp"


//
// the typedefs below are necessary to ensure that args are passed correctly when calling a primitive through a function pointer
// NB: there's no general n-argument primitive because some calling conventions can't handle vararg functions
//

typedef Oop (__CALLING_CONVENTION *prim_fntype0)();

typedef Oop (__CALLING_CONVENTION *prim_fntype1)( Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype2)( Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype3)( Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype4)( Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype5)( Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype6)( Oop, Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype7)( Oop, Oop, Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype8)( Oop, Oop, Oop, Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype9)( Oop, Oop, Oop, Oop, Oop, Oop, Oop, Oop, Oop );


//
// The PrimitiveDescriptor structure exposes all properties of a primitive.
// Primitives are like procedures (no dispatching is necessary) and invoked by providing a number of parameters.
//
// _flags
//  16    can_scavenge() can it trigger a scavenge/GC?
//  17    can_perform_NonLocalReturn() can it do an NonLocalReturn or process abort
//  18    can_fail()
//  19    can_be_constant_folded() is it side-effect free? (so it can be const-folded if args are const)
//  20    has_receiver()  does it require a receiver? ({{self prim...}})
//  21    is_internal()  true for VM-internal primitives
//  22    needs_delta_fp_code()  must caller set up lastDeltaFP?
//

class PrimitiveDescriptor {

public:
    const char            *_name;       // name of the primitive
    primitiveFunctionType _fn;          // primitive entry point
    std::int32_t                   _flags;       // see unpacking below
    const char            **_types;     // the return type and parameter types [0] contains the return type, [1..number_of_parameters] contains types for the parameters
    const char            **_errors;    // a null terminated list of errors for the primitive excluding {FirstArgumenthasWrongType, SecondArgumenthasWrongType ...}

public:
    const char *name() const {
        return _name;
    }


    // the primitive entry point.
    primitiveFunctionType fn() const {
        return _fn;
    }


    // flags
    std::int32_t number_of_parameters() const {
        return get_unsigned_bitfield( _flags, 0, 8 );
    }


    // # of parameters (self or arguments), excluding failure block (if any);
    // i.e., # of args that are actually passed to C
    PrimitiveGroup group() const {
        return (PrimitiveGroup) get_unsigned_bitfield( _flags, 8, 8 );
    }


    bool is_special_prim() const {
        return group() not_eq PrimitiveGroup::NormalPrimitive;
    }


    bool can_scavenge() const {
        return isBitSet( _flags, 16 );
    }   // can it trigger a scavenge/GC?

    bool can_perform_NonLocalReturn() const {
        return isBitSet( _flags, 17 );
    }   // can it do an NonLocalReturn or process abort?

    bool can_fail() const {
        return isBitSet( _flags, 18 );
    }


    // can_fail: can primitive fail with arguments of correct type?  (NB: even if not can_fail(), primitive will fail if argument types are wrong)
    bool can_invoke_delta() const {
        return can_perform_NonLocalReturn();
    }   // can it call other Delta code?

    bool can_be_constant_folded() const {
        return isBitSet( _flags, 19 );
    }   // is it side-effect free? (so it can be const-folded if args are const)

    bool has_receiver() const {
        return isBitSet( _flags, 20 );
    }   // does it require a receiver? ({{self prim...}})

    bool is_internal() const {
        return isBitSet( _flags, 21 );
    }   // true for VM-internal primitives

    bool needs_delta_fp_code() const {
        return !isBitSet( _flags, 22 );
    }   // must caller set up lastDeltaFP?

    bool can_walk_stack() const; // can it trigger a stack print? (debug/interrupt point)

    // the name of the primitive as a symbol
    SymbolOop selector() const;

    // Evaluates the primitive with the given parameters
    Oop eval( Oop *parameters );


protected:        // NB: can't use protected:, or else Microsoft Compiler (2.1) refuses to initialize prim array
    // Type information of the primitive
    const char *parameter_type( std::int32_t index ) const; // 0 <= index < number_of_parameters()
    const char *return_type() const;

    // Type information for compiler
    // - returns nullptr if type is unknown or too complicated

public:
    Expression *parameter_klass( std::int32_t index, PseudoRegister *p, Node *n ) const; // 0 <= index < number_of_parameters()
    Expression *return_klass( PseudoRegister *p, Node *n ) const;

protected:
    Expression *convertToKlass( const char *type, PseudoRegister *p, Node *n ) const;

public:
    // Error information
    const char *error( std::int32_t index ) const {
        return _errors[ index ];
    }

    // Comparison operation (must have signed result)
    std::int32_t compare( const char *str, std::int32_t len ) const;

    // Miscellaneous operations
    void print();

    void verify();

    void error( const char *msg );
};
