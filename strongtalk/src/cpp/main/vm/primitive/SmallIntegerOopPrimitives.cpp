
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/utility/ObjectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitive/SmallIntegerOopPrimitives.hpp"
#include "vm/runtime/VMSymbol.hpp"


TRACE_FUNC( TraceSmiPrims, "small_int_t" )


std::int32_t SmallIntegerOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->isSmallIntegerOop(), "receiver must be small_int_t")

#define SMI_RELATIONAL_OP( op ) \
  if (not argument->isSmallIntegerOop()) \
    return markSymbol(vmSymbols::first_argument_has_wrong_type()); \
  std::int32_t a = (std::int32_t) receiver; \
  std::int32_t b = (std::int32_t) argument; \
  return a op b ? trueObject : falseObject


PRIM_DECL_2( SmallIntegerOopPrimitives::lessThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThan", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( < );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::greaterThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThan", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( > );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::lessThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThanOrEqual", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( <= );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::greaterThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThanOrEqual", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( >= );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::equal, Oop receiver, Oop argument ) {
    PROLOGUE_2( "equal", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( == );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::notEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "notEqual", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( not_eq );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::bitAnd, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitAnd", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return SmallIntegerOop( std::int32_t( receiver ) & std::int32_t( argument ) );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::bitOr, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitOr", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return SmallIntegerOop( std::int32_t( receiver ) | std::int32_t( argument ) );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::bitXor, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitXor", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return SmallIntegerOop( std::int32_t( receiver ) ^ std::int32_t( argument ) );
}


PRIM_DECL_2( SmallIntegerOopPrimitives::bitShift, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitShift", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    st_assert( INTEGER_TAG == 0, "check this code" );
    constexpr std::int32_t bitsPerWord = OOP_SIZE * 8;
    constexpr std::int32_t maxShiftCnt = bitsPerWord - TAG_SIZE - 1;
    std::int32_t           n           = SmallIntegerOop( argument )->value();
    if ( n > 0 ) {
        // arithmetic shift left
        if ( n < maxShiftCnt ) {
            // 0 < n < maxShiftCnt < bitsPerWord	// |<- n ->|<- 1 ->|<- 32-(n+1) ->|
            std::int32_t mask1 = 1 << ( bitsPerWord - ( n + 1 ) );    // |00...00|   1   |00..........00|
            std::int32_t mask2 = -1 << ( bitsPerWord - n );    // |11...11|   0   |00..........00|
            if ( ( ( std::int32_t( receiver ) + mask1 ) & mask2 ) == 0 ) {
                // i.e., the bit at position (32-(n+1)) is the same as the upper n bits, thus
                // after shifting out the upper n bits the sign hasn't changed -> no overflow
                return SmallIntegerOop( std::int32_t( receiver ) << n );
            }
        }
        return markSymbol( vmSymbols::smi_overflow() );
    } else {
        // arithmetic shift right
        if ( n < -maxShiftCnt )
            n = -maxShiftCnt;
        return SmallIntegerOop( ( std::int32_t( receiver ) >> -n ) & ( -1 << TAG_SIZE ) );
    }
}


PRIM_DECL_2( SmallIntegerOopPrimitives::rawBitShift, Oop receiver, Oop argument ) {
    PROLOGUE_2( "rawBitShift", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    st_assert( INTEGER_TAG == 0, "check this code" );
    const std::int32_t bitsPerWord = OOP_SIZE * 8;
    std::int32_t       n           = SmallIntegerOop( argument )->value();
    if ( n >= 0 ) {
        // logical shift right
        return SmallIntegerOop( (std::uint32_t) receiver << ( n % bitsPerWord ) );
    } else {
        // logical shift left
        return SmallIntegerOop( ( (std::uint32_t) receiver >> ( ( -n ) % bitsPerWord ) ) & ( -1 << TAG_SIZE ) );
    }
}


PRIM_DECL_1( SmallIntegerOopPrimitives::asObject, Oop receiver ) {
    PROLOGUE_1( "asObject", receiver );
    ASSERT_RECEIVER;
    std::int32_t id = SmallIntegerOop( receiver )->value();
    if ( ObjectIDTable::is_index_ok( id ) )
        return ObjectIDTable::at( id );
    return markSymbol( vmSymbols::index_not_valid() );
}


PRIM_DECL_1( SmallIntegerOopPrimitives::printCharacter, Oop receiver ) {
    PROLOGUE_1( "printCharacter", receiver );
    ASSERT_RECEIVER;
    SPDLOG_INFO( "%c", SmallIntegerOop( receiver )->value() );
    return receiver;
}


static void trap() {
    st_assert( false, "This primitive should be patched" );
}


extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_add( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_subtract( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_multiply( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_mod( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_div( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_quo( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_remainder( Oop receiver, Oop argument ) {
    st_unused( receiver ); // unused
    st_unused( argument ); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
