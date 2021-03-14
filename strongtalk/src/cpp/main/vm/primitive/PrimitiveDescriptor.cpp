
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitive/DoubleByteArray_primitives.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/primitive/PrimitiveDescriptor.hpp"
#include "vm/compiler/Expression.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/primitive/block_primitives.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/runtime/Process.hpp"


void PrimitiveDescriptor::print() {

    //
    SPDLOG_INFO( "%-72s  {:d}  {}{}{}{}{}{}{}{}{}",
                  name(),
                  number_of_parameters(),
                  has_receiver() ? 'R' : '_',
                  can_fail() ? 'F' : '_',
                  can_scavenge() ? 'S' : '_',
                  can_walk_stack() ? 'W' : '_',
                  can_perform_NonLocalReturn() ? 'N' : '_',
                  can_be_constant_folded() ? 'C' : '_',
                  can_invoke_delta() ? 'D' : '_',
                  is_internal() ? 'I' : '_',
                  needs_delta_fp_code() ? 'P' : '_' );

    //
    switch ( group() ) {
        case PrimitiveGroup::IntComparisonPrimitive:
            _console->print( "IntComparisonPrimitive / small_int_t" );
            break;
        case PrimitiveGroup::IntArithmeticPrimitive:
            _console->print( "IntArithmeticPrimitive / small_int_t" );
            break;
        case PrimitiveGroup::FloatComparisonPrimitive:
            _console->print( "FloatComparisonPrimitive" );
            break;
        case PrimitiveGroup::FloatArithmeticPrimitive:
            _console->print( "FloatArithmeticPrimitive" );
            break;
        case PrimitiveGroup::ByteArrayPrimitive:
            _console->print( "ByteArrayPrimitive" );
            break;
        case PrimitiveGroup::DoubleByteArrayPrimitive:
            _console->print( "DoubleByteArrayPrimitive" );
            break;
        case PrimitiveGroup::ObjectArrayPrimitive:
            _console->print( "ObjectArrayPrimitive" );
            break;
        case PrimitiveGroup::BlockPrimitive:
            _console->print( "BlockPrimitive" );
            break;
        case PrimitiveGroup::NormalPrimitive:
            _console->print( "NormalPrimitive" );
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
    const bool reverseArgs = true;    // change this when changing primitive calling convention
    Oop        res{};                 //

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
            default:
                res = nullptr;
                ShouldNotReachHere();
                ::exit( 111 );
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
            default:
                res = nullptr;
                ShouldNotReachHere();
                ::exit( 111 );
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
        SPDLOG_INFO( "ebx changed from ({0:x} to {0:x}) in:", ebx_on_stack, ebx_now );
        print();
    }

    return res;
}



bool PrimitiveDescriptor::can_walk_stack() const {
    return can_scavenge() or can_invoke_delta() or can_perform_NonLocalReturn();
}


SymbolOop PrimitiveDescriptor::selector() const {
    return OopFactory::new_symbol( name() );
}


const char *PrimitiveDescriptor::parameter_type( std::int32_t index ) const {
    st_assert( ( 0 <= index ) and ( index < number_of_parameters() ), "illegal parameter index" );
    return _types[ 1 + index ];
}


const char *PrimitiveDescriptor::return_type() const {
    return _types[ 0 ];
}


Expression *PrimitiveDescriptor::convertToKlass( const char *type, PseudoRegister *p, Node *n ) const {

    if ( 0 == strcmp( type, "SmallInteger" ) )
        return new KlassExpression( Universe::smiKlassObject(), p, n );

    if ( 0 == strcmp( type, "Double" ) )
        return new KlassExpression( Universe::doubleKlassObject(), p, n );

    if ( 0 == strcmp( type, "Float" ) )
        return new KlassExpression( Universe::doubleKlassObject(), p, n );

    if ( 0 == strcmp( type, "Symbol" ) )
        return new KlassExpression( Universe::symbolKlassObject(), p, n );

    if ( 0 == strcmp( type, "Boolean" ) ) {
        // NB: set expression node to nullptr, not n -- MergeExpression cannot be split
        Expression *t = new ConstantExpression( Universe::trueObject(), p, nullptr );
        Expression *f = new ConstantExpression( Universe::falseObject(), p, nullptr );
        return new MergeExpression( t, f, p, nullptr );
    }

    // should extend:
    // - looking up klassName in global class dictionary would cover many other cases
    // - for these, need to agree what prim info means: "exact match" or "any subclass of"
    // fix this later  -Urs 11/95
    return nullptr;
}


Expression *PrimitiveDescriptor::parameter_klass( std::int32_t index, PseudoRegister *p, Node *n ) const {
    return convertToKlass( parameter_type( index ), p, n );
}


Expression *PrimitiveDescriptor::return_klass( PseudoRegister *p, Node *n ) const {
    return convertToKlass( return_type(), p, n );
}


void PrimitiveDescriptor::error( const char *msg ) {
    print();
    ::error( msg );
}


void PrimitiveDescriptor::verify() {

    if ( can_invoke_delta() ) {
        if ( not can_scavenge() )
            error( "canInvokeDelta implies canScavenge" );
        if ( not can_walk_stack() )
            error( "canInvokeDelta implies can_walk_stack" );
        if ( can_be_constant_folded() )
            error( "canInvokeDelta implies not canbeConstantFolded" );
        if ( not can_perform_NonLocalReturn() )
            error( "canInvokeDelta implies canPerformNonLocalReturn" );
    }
    if ( can_be_constant_folded() ) {
        if ( can_perform_NonLocalReturn() )
            error( "canbeConstantFolded implies not canPerformNonLocalReturn" );
    }
    if ( group() == PrimitiveGroup::BlockPrimitive ) {
        if ( not can_walk_stack() )
            error( "blocks must have can_walk_stack" );
    }
}


std::int32_t PrimitiveDescriptor::compare( const char *str, std::int32_t len ) const {

    std::int32_t src_len = strlen( name() );
    std::int32_t sign    = strncmp( name(), str, min( src_len, len ) );

//    if ( sign not_eq 0 or src_len == len ) return sign;
    if ( sign not_eq 0 )
        return sign < 0 ? -1 : 1;
    if ( src_len == len )
        return 0;
    return src_len < len ? -1 : 1;
}
