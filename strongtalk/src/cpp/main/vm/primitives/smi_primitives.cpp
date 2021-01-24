//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitives/smi_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"


TRACE_FUNC( TraceSmiPrims, "smi_t" )


std::size_t smiOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_smi(), "receiver must be smi_t")

#define SMI_RELATIONAL_OP( op )                                     \
  if (not argument->is_smi())                                       \
    return markSymbol(vmSymbols::first_argument_has_wrong_type());  \
  int a = (int) receiver;                                           \
  int b = (int) argument;                                           \
  return a op b ? trueObject : falseObject


PRIM_DECL_2( smiOopPrimitives::lessThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThan", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( < );
}


PRIM_DECL_2( smiOopPrimitives::greaterThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThan", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( > );
}


PRIM_DECL_2( smiOopPrimitives::lessThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThanOrEqual", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( <= );
}


PRIM_DECL_2( smiOopPrimitives::greaterThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThanOrEqual", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( >= );
}


PRIM_DECL_2( smiOopPrimitives::equal, Oop receiver, Oop argument ) {
    PROLOGUE_2( "equal", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( == );
}


PRIM_DECL_2( smiOopPrimitives::notEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "notEqual", receiver, argument );
    ASSERT_RECEIVER;
    SMI_RELATIONAL_OP( not_eq );
}


PRIM_DECL_2( smiOopPrimitives::bitAnd, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitAnd", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return SMIOop( int( receiver ) & int( argument ) );
}


PRIM_DECL_2( smiOopPrimitives::bitOr, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitOr", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return SMIOop( int( receiver ) | int( argument ) );
}


PRIM_DECL_2( smiOopPrimitives::bitXor, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitXor", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return SMIOop( int( receiver ) ^ int( argument ) );
}


PRIM_DECL_2( smiOopPrimitives::bitShift, Oop receiver, Oop argument ) {
    PROLOGUE_2( "bitShift", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    st_assert( INTEGER_TAG == 0, "check this code" );
    constexpr int bitsPerWord = oopSize * 8;
    constexpr int maxShiftCnt = bitsPerWord - TAG_SIZE - 1;
    int           n           = SMIOop( argument )->value();
    if ( n > 0 ) {
        // arithmetic shift left
        if ( n < maxShiftCnt ) {
            // 0 < n < maxShiftCnt < bitsPerWord	// |<- n ->|<- 1 ->|<- 32-(n+1) ->|
            int mask1 = 1 << ( bitsPerWord - ( n + 1 ) );    // |00...00|   1   |00..........00|
            int mask2 = -1 << ( bitsPerWord - n );    // |11...11|   0   |00..........00|
            if ( ( ( int( receiver ) + mask1 ) & mask2 ) == 0 ) {
                // i.e., the bit at position (32-(n+1)) is the same as the upper n bits, thus
                // after shifting out the upper n bits the sign hasn't changed -> no overflow
                return SMIOop( int( receiver ) << n );
            }
        }
        return markSymbol( vmSymbols::smi_overflow() );
    } else {
        // arithmetic shift right
        if ( n < -maxShiftCnt )
            n = -maxShiftCnt;
        return SMIOop( ( int( receiver ) >> -n ) & ( -1 << TAG_SIZE ) );
    }
}


PRIM_DECL_2( smiOopPrimitives::rawBitShift, Oop receiver, Oop argument ) {
    PROLOGUE_2( "rawBitShift", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    st_assert( INTEGER_TAG == 0, "check this code" );
    const int bitsPerWord = oopSize * 8;
    int       n           = SMIOop( argument )->value();
    if ( n >= 0 ) {
        // logical shift right
        return SMIOop( (std::uint32_t) receiver << ( n % bitsPerWord ) );
    } else {
        // logical shift left
        return SMIOop( ( (std::uint32_t) receiver >> ( ( -n ) % bitsPerWord ) ) & ( -1 << TAG_SIZE ) );
    }
}


PRIM_DECL_1( smiOopPrimitives::asObject, Oop receiver ) {
    PROLOGUE_1( "asObject", receiver );
    ASSERT_RECEIVER;
    int id = SMIOop( receiver )->value();
    if ( objectIDTable::is_index_ok( id ) )
        return objectIDTable::at( id );
    return markSymbol( vmSymbols::index_not_valid() );
}


PRIM_DECL_1( smiOopPrimitives::printCharacter, Oop receiver ) {
    PROLOGUE_1( "printCharacter", receiver );
    ASSERT_RECEIVER;
    lprintf( "%c", SMIOop( receiver )->value() );
    return receiver;
}


static void trap() {
    st_assert( false, "This primitive should be patched" );
}


extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_add( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_subtract( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_multiply( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_mod( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_div( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_quo( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_remainder( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
