//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"



// The file defines the interface to the internal primitives.

// Primitives are divided into four categories:
//                                                        (canScavenge, NonLocalReturn,   canBeConstantFolded)
// 1) Allocation primitives.                              (true,        false, false)
// 2) Pure primitives.                                    (false,       false, true)
// 3) Function primitives                                 (false,       false, false)
// 4) Primitives who might scavenge, gc, or call delta    (true,        false, false)
//    .


// WARNING: do not change the element order of enum PrimitiveGroup
// without adjusting the Smalltalk DeltaPrimitiveGenerator code to match!

enum PrimitiveGroup {
    NormalPrimitive,            //

    IntComparisonPrimitive,     // Integer comparison primitive
    IntArithmeticPrimitive,     // Integer arithmetic

    FloatComparisonPrimitive,   // FP comparison primitive
    FloatArithmeticPrimitive,   // FP arithmetic

    ObjArrayPrimitive,          // access/size
    ByteArrayPrimitive,         // access/size
    DoubleByteArrayPrimitive,   // access/size

    BlockPrimitive              // block-related primitives (creation, invocation, contexts)
};

// WARNING: do not change the element order of enum PrimitiveGroup
// without adjusting the Smalltalk DeltaPrimitiveGenerator code to match!




class PrimitiveDescriptor;


class InterpretedPrimitiveCache : public ValueObject {

    private:
        uint8_t * hp() const {
            return ( uint8_t * ) this;
        }


    public:
        SymbolOop name() const;

        int number_of_parameters() const;

        PrimitiveDescriptor * pdesc() const;

        bool_t has_receiver() const;

        bool_t has_failure_code() const;
};


// The PrimitiveDescriptor structure exposes all properties of a primitive.
// Primitives are like procedures (no dispatching is necessary) and invoked by providing a number of parameters.

class PseudoRegister;
class Expression;
class Node;

// _flags
//  16    can_scavenge() can it trigger a scavenge/GC?
//  17    can_perform_NonLocalReturn() can it do an NonLocalReturn or process abort
//  18    can_fail()
//  19    can_be_constant_folded() is it side-effect free? (so it can be const-folded if args are const)
//  20    has_receiver()  does it require a receiver? ({{self prim...}})
//  21    is_internal()  true for VM-internal primitives
//  22    needs_delta_fp_code()  must caller set up lastDeltaFP?

class PrimitiveDescriptor {

    public:
        const char * _name;             // name of the primitive
        primitiveFunctionType _fn;      // primitive entry point
        int                   _flags;   // see unpacking below
        const char ** _types;           // the return type and parameter types [0] contains the return type, [1..number_of_parameters] contains types for the parameters
        const char ** _errors;          // a null terminated list of errors for the primitive excluding {FirstArgumenthasWrongType, SecondArgumenthasWrongType ...}

    public:
        const char * name() const {
            return _name;
        }


        // the primitive entry point.
        primitiveFunctionType fn() const {
            return _fn;
        }


        // flags
        int number_of_parameters() const {
            return get_unsigned_bitfield( _flags, 0, 8 );
        }


        // # of parameters (self or arguments), excluding failure block (if any);
        // i.e., # of args that are actually passed to C
        PrimitiveGroup group() const {
            return ( PrimitiveGroup ) get_unsigned_bitfield( _flags, 8, 8 );
        }


        bool_t is_special_prim() const {
            return group() not_eq NormalPrimitive;
        }


        bool_t can_scavenge() const {
            return isBitSet( _flags, 16 );
        }   // can it trigger a scavenge/GC?

        bool_t can_perform_NonLocalReturn() const {
            return isBitSet( _flags, 17 );
        }   // can it do an NonLocalReturn or process abort?

        bool_t can_fail() const {
            return isBitSet( _flags, 18 );
        }

        // can_fail: can primitive fail with arguments of correct type?  (NB: even if not can_fail(), primitive will fail if argument types are wrong)
        bool_t can_invoke_delta() const {
            return can_perform_NonLocalReturn();
        }   // can it call other Delta code?

        bool_t can_be_constant_folded() const {
            return isBitSet( _flags, 19 );
        }   // is it side-effect free? (so it can be const-folded if args are const)

        bool_t has_receiver() const {
            return isBitSet( _flags, 20 );
        }   // does it require a receiver? ({{self prim...}})

        bool_t is_internal() const {
            return isBitSet( _flags, 21 );
        }   // true for VM-internal primitives

        bool_t needs_delta_fp_code() const {
            return !isBitSet( _flags, 22 );
        }   // must caller set up lastDeltaFP?

        bool_t can_walk_stack() const; // can it trigger a stack print? (debug/interrupt point)

        // the name of the primitive as a symbol
        SymbolOop selector() const;

        // Evaluates the primitive with the given parameters
        Oop eval( Oop * parameters );


    protected:        // NB: can't use protected:, or else Microsoft Compiler (2.1) refuses to initialize prim array
        // Type information of the primitive
        const char * parameter_type( int index ) const; // 0 <= index < number_of_parameters()
        const char * return_type() const;

        // Type information for compiler
        // - returns nullptr if type is unknown or too complicated

    public:
        Expression * parameter_klass( int index, PseudoRegister * p, Node * n ) const; // 0 <= index < number_of_parameters()
        Expression * return_klass( PseudoRegister * p, Node * n ) const;

    protected:
        Expression * convertToKlass( const char * type, PseudoRegister * p, Node * n ) const;

    public:
        // Error information
        const char * error( int index ) const {
            return _errors[ index ];
        }

        // Comparison operation
        int compare( const char * str, int len );

        // Miscellaneous operations
        void print();

        void verify();

        void error( const char * msg );
};


class Primitives : AllStatic {

    public:
        static void print_table();


        static PrimitiveDescriptor * lookup( SymbolOop selector ) {
            return lookup( ( const char * ) selector->bytes(), selector->length() );
        }


        static PrimitiveDescriptor * lookup( primitiveFunctionType fn );

        static void lookup_and_patch(); // routine called by interpreter

        // For debugging / profiling
        static void clear_counters();

        static void print_counters();

        static void initialize();

        static void patch( const char * name, const char * entry_point );

    private:
        static PrimitiveDescriptor * lookup( const char * selector, int len );
        static PrimitiveDescriptor * lookup( const char * selector );


        static PrimitiveDescriptor * verified_lookup( const char * selector ); // Fails if the primitive could not be found

        // Primitives used by the compiler are looked up at startup
        static PrimitiveDescriptor * _new0;
        static PrimitiveDescriptor * _new1;
        static PrimitiveDescriptor * _new2;
        static PrimitiveDescriptor * _new3;
        static PrimitiveDescriptor * _new4;
        static PrimitiveDescriptor * _new5;
        static PrimitiveDescriptor * _new6;
        static PrimitiveDescriptor * _new7;
        static PrimitiveDescriptor * _new8;
        static PrimitiveDescriptor * _new9;

        static PrimitiveDescriptor * _equal;
        static PrimitiveDescriptor * _not_equal;

        static PrimitiveDescriptor * _block_allocate;
        static PrimitiveDescriptor * _block_allocate0;
        static PrimitiveDescriptor * _block_allocate1;
        static PrimitiveDescriptor * _block_allocate2;

        static PrimitiveDescriptor * _context_allocate;
        static PrimitiveDescriptor * _context_allocate0;
        static PrimitiveDescriptor * _context_allocate1;
        static PrimitiveDescriptor * _context_allocate2;

    public:
        static PrimitiveDescriptor * new0() {
            return _new0;
        }


        static PrimitiveDescriptor * new1() {
            return _new1;
        }


        static PrimitiveDescriptor * new2() {
            return _new2;
        }


        static PrimitiveDescriptor * new3() {
            return _new3;
        }


        static PrimitiveDescriptor * new4() {
            return _new4;
        }


        static PrimitiveDescriptor * new5() {
            return _new5;
        }


        static PrimitiveDescriptor * new6() {
            return _new6;
        }


        static PrimitiveDescriptor * new7() {
            return _new7;
        }


        static PrimitiveDescriptor * new8() {
            return _new8;
        }


        static PrimitiveDescriptor * new9() {
            return _new9;
        }


        static PrimitiveDescriptor * equal() {
            return _equal;
        }


        static PrimitiveDescriptor * not_equal() {
            return _not_equal;
        }


        static PrimitiveDescriptor * block_allocate() {
            return _block_allocate;
        }


        static PrimitiveDescriptor * block_allocate0() {
            return _block_allocate0;
        }


        static PrimitiveDescriptor * block_allocate1() {
            return _block_allocate1;
        }


        static PrimitiveDescriptor * block_allocate2() {
            return _block_allocate2;
        }


        static PrimitiveDescriptor * context_allocate() {
            return _context_allocate;
        }


        static PrimitiveDescriptor * context_allocate0() {
            return _context_allocate0;
        }


        static PrimitiveDescriptor * context_allocate1() {
            return _context_allocate1;
        }


        static PrimitiveDescriptor * context_allocate2() {
            return _context_allocate2;
        }
};
