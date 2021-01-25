
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/CodeIterator.hpp"
#include "vm/primitives/dByteArray_primitives.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/primitives/primitives_table.cpp"
#include "PrimitiveDescriptor.hpp"


void PrimitiveDescriptor::print() {

    //
    _console->print( "%-72s  %d  %s%s%s%s%s%s%s%s%s",
                     name(),
                     number_of_parameters(),
                     has_receiver() ? "R" : "_",
                     can_fail() ? "F" : "_",
                     can_scavenge() ? "S" : "_",
                     can_walk_stack() ? "W" : "_",
                     can_perform_NonLocalReturn() ? "N" : "_",
                     can_be_constant_folded() ? "C" : "_",
                     can_invoke_delta() ? "D" : "_",
                     is_internal() ? "I" : "_",
                     needs_delta_fp_code() ? "P" : "_" );

    //
    switch ( group() ) {
        case PrimitiveGroup::IntComparisonPrimitive:
            _console->print( "IntComparisonPrimitive / smi_t");
            break;
        case PrimitiveGroup::IntArithmeticPrimitive:
            _console->print( "IntArithmeticPrimitive / smi_t");
            break;
        case PrimitiveGroup::FloatComparisonPrimitive:
            _console->print( "FloatComparisonPrimitive");
            break;
        case PrimitiveGroup::FloatArithmeticPrimitive:
            _console->print( "FloatArithmeticPrimitive");
            break;
        case PrimitiveGroup::ByteArrayPrimitive:
            _console->print( "ByteArrayPrimitive");
            break;
        case PrimitiveGroup::DoubleByteArrayPrimitive:
            _console->print( "DoubleByteArrayPrimitive");
            break;
        case PrimitiveGroup::ObjArrayPrimitive:
            _console->print( "ObjArrayPrimitive");
            break;
        case PrimitiveGroup::BlockPrimitive:
            _console->print( "BlockPrimitive");
            break;
        case PrimitiveGroup::NormalPrimitive:
            _console->print( "NormalPrimitive");
            break;
        default: st_fatal( "Unknown primitive group" );
    }

    //
    _console->cr();
}


Oop PrimitiveDescriptor::eval( Oop *a ) {

    std::int32_t ebx_on_stack;

// %hack: see below
#ifdef __GNUC__
    __asm__(
    "pushl %%eax;"
    "movl %%ebx, %%eax;"
    "movl %%eax, %0;"
    "popl %%eax;"
    : "=a"(ebx_on_stack)
    );
#else
    __asm mov ebx_on_stack, ebx
#endif

    //
    const bool_t reverseArgs = true;    // change this when changing primitive calling convention
    Oop          res;                   //

    //
    if ( reverseArgs ) {
        switch ( number_of_parameters() ) {
            case 0:
                res = ( reinterpret_cast<prim_fntype0>( _fn ) )();
                break;
            case 1:
                res = ( reinterpret_cast<prim_fntype1>( _fn ) )( a[ 0 ] );
                break;
            case 2:
                res = ( reinterpret_cast<prim_fntype2>( _fn ) )( a[ 1 ], a[ 0 ] );
                break;
            case 3:
                res = ( reinterpret_cast<prim_fntype3>( _fn ) )( a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 4:
                res = ( reinterpret_cast<prim_fntype4>( _fn ) )( a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 5:
                res = ( reinterpret_cast<prim_fntype5>( _fn ) )( a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 6:
                res = ( reinterpret_cast<prim_fntype6>( _fn ) )( a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 7:
                res = ( reinterpret_cast<prim_fntype7>( _fn ) )( a[ 6 ], a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 8:
                res = ( reinterpret_cast<prim_fntype8>( _fn ) )( a[ 7 ], a[ 6 ], a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 9:
                res = ( reinterpret_cast<prim_fntype9>( _fn ) )( a[ 8 ], a[ 7 ], a[ 6 ], a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            default: ShouldNotReachHere();
        }

    } else {
        switch ( number_of_parameters() ) {
            case 0:
                res = ( reinterpret_cast<prim_fntype0>( _fn ) )();
                break;
            case 1:
                res = ( reinterpret_cast<prim_fntype1>( _fn ) )( a[ 0 ] );
                break;
            case 2:
                res = ( reinterpret_cast<prim_fntype2>( _fn ) )( a[ 0 ], a[ 1 ] );
                break;
            case 3:
                res = ( reinterpret_cast<prim_fntype3>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ] );
                break;
            case 4:
                res = ( reinterpret_cast<prim_fntype4>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ] );
                break;
            case 5:
                res = ( reinterpret_cast<prim_fntype5>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ] );
                break;
            case 6:
                res = ( reinterpret_cast<prim_fntype6>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ] );
                break;
            case 7:
                res = ( reinterpret_cast<prim_fntype7>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ], a[ 6 ] );
                break;
            case 8:
                res = ( reinterpret_cast<prim_fntype8>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ], a[ 6 ], a[ 7 ] );
                break;
            case 9:
                res = ( reinterpret_cast<prim_fntype9>( _fn ) )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ], a[ 6 ], a[ 7 ], a[ 8 ] );
                break;
            default: ShouldNotReachHere();
        }
    }

    // %hack: some primitives alter EBX and crash the compiler's constant propagation
    std::int32_t ebx_now;
#ifdef __GNUC__
    __asm__("pushl %%eax;"
            "movl %%ebx, %%eax;"
            "movl %%eax, %0;"
            "movl %1, %%eax;"
            "movl %%eax, %%ebx;"
            "popl %%eax;" : "=a"(ebx_now) : "a"(ebx_on_stack));
#endif

#ifdef _MSVC
    __asm mov ebx_now, ebx
    __asm mov ebx, ebx_on_stack
#endif

    if ( ebx_now not_eq ebx_on_stack ) {
        _console->print_cr( "ebx changed (%X -> %X) in :", ebx_on_stack, ebx_now );
        print();
    }

    return res;
}
