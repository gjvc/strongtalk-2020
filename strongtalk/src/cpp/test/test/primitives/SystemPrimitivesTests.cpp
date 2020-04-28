
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/primitives/system_primitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"

#include "test/utilities/testUtils.hpp"
#include "vm/system/sizes.hpp"

#include <gtest/gtest.h>


TEST( SystemPrimitivesTests, expansionShouldExpandOldGenerationCapacity ) {
    char msg[100];
    int  oldSize      = Universe::old_gen.capacity();
    EXPECT_EQ( trueObj, SystemPrimitives::expandMemory( smiOopFromValue( 1000 * 1024 ) ) ) << "wrong size";
    int  expectedSize = oldSize + ReservedSpace::align_size( 1000 * 1024, ObjectHeapExpandSize * 1024 );
    int  actualSize   = Universe::old_gen.capacity();
    sprintf( msg, "Generation has wrong capacity. Expected: %d, but was: %d", expectedSize, actualSize );
    EXPECT_EQ( expectedSize, actualSize ) << msg;
}


TEST( SystemPrimitivesTests, expansionShouldReturnMarkedResultWhenNotSMI ) {
    Oop result = SystemPrimitives::expandMemory( Oop( vmSymbols::aborted() ) );
    EXPECT_TRUE( result->is_mark() ) << "Result should be marked";
    EXPECT_EQ( vmSymbols::argument_has_wrong_type(), unmarkSymbol( result ) ) << "Wrong symbol";
}


TEST( SystemPrimitivesTests, expansionShouldReturnMarkedResultWhenNegative ) {
    Oop result = SystemPrimitives::expandMemory( smiOopFromValue( -1 ) );
    EXPECT_TRUE( result->is_mark() ) << "Result should be marked";
    EXPECT_EQ( vmSymbols::argument_is_invalid(), unmarkSymbol( result ) ) << "Wrong symbol";
}


TEST( SystemPrimitivesTests, oopSize ) {
    EXPECT_EQ( smiOopFromValue( oopSize ), SystemPrimitives::oopSize() ) << "oopSize is wrong";
}


extern "C" int expansion_count;

TEST( SystemPrimitivesTests, expansionsShouldReturnExpansionCountAsSMI ) {
    int expansions = expansion_count;
    SystemPrimitives::expandMemory( smiOopFromValue( ObjectHeapExpandSize * 1024 ) );
    EXPECT_EQ( smiOopFromValue( expansions + 1 ), SystemPrimitives::expansions() ) << "Wrong expansion count";
}


TEST( SystemPrimitivesTests, nurseryFreeSpaceShouldReturnEdenFreeSpaceAsSMI ) {
    ASSERT_EQ( Universe::new_gen.eden()->free(), SMIOop( SystemPrimitives::nurseryFreeSpace() )->value() );
}


TEST( SystemPrimitivesTests, shrinkMemoryShouldReduceOldSpaceCapacity ) {
    int freeSpace = SMIOop( SystemPrimitives::freeSpace() )->value();
    SystemPrimitives::expandMemory( smiOopFromValue( ObjectHeapExpandSize * 1024 ) );
    SystemPrimitives::shrinkMemory( smiOopFromValue( ObjectHeapExpandSize * 1024 ) );
    ASSERT_EQ( freeSpace, SMIOop( SystemPrimitives::freeSpace() )->value() );
}


TEST( SystemPrimitivesTests, shrinkMemoryShouldReturnValueOutOfRangeWhenInsufficientFreeSpace ) {
    int freeSpace = SMIOop( SystemPrimitives::freeSpace() )->value();
    ASSERT_EQ( ( int ) markSymbol( vmSymbols::value_out_of_range() ), ( int ) SystemPrimitives::shrinkMemory( smiOopFromValue( freeSpace + 1 ) ) );
    ASSERT_EQ( freeSpace, SMIOop( SystemPrimitives::freeSpace() )->value() );
}


TEST( SystemPrimitivesTests, shrinkMemoryShouldReturnValueOutOfRangeWhenNegative ) {
    int freeSpace = SMIOop( SystemPrimitives::freeSpace() )->value();
    ASSERT_EQ( ( int ) markSymbol( vmSymbols::value_out_of_range() ), ( int ) SystemPrimitives::shrinkMemory( smiOopFromValue( -1 ) ) );
    ASSERT_EQ( freeSpace, SMIOop( SystemPrimitives::freeSpace() )->value() );
}


TEST( SystemPrimitivesTests, shrinkMemoryShouldReturnArgumentIsOfWrongType ) {
    int freeSpace = SMIOop( SystemPrimitives::freeSpace() )->value();
    ASSERT_EQ( ( int ) markSymbol( vmSymbols::first_argument_has_wrong_type() ), ( int ) SystemPrimitives::shrinkMemory( vmSymbols::and1() ) );
    ASSERT_EQ( freeSpace, SMIOop( SystemPrimitives::freeSpace() )->value() );
}


TEST( SystemPrimitivesTests, alienMallocShouldReturnAlignedValue ) {
    Oop pointer = SystemPrimitives::alienMalloc( smiOopFromValue( 4 ) );
    EXPECT_TRUE( pointer->is_smi() ) << "result should be SmallInteger";
    int address = SMIOop( pointer )->value();
    EXPECT_TRUE( address % 4 == 0 ) << "not aligned";
    EXPECT_TRUE( address != 0 ) << "not allocated";
    free( ( void * ) address );
}


TEST( SystemPrimitivesTests, alienMallocShouldReturnMarkedSymbolWhenNotSmi ) {
    Oop pointer = SystemPrimitives::alienMalloc( vmSymbols::abs() );
    EXPECT_TRUE( pointer->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_has_wrong_type() ), pointer ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienMallocShouldReturnMarkedSymbolWhenSizeNegative ) {
    Oop pointer = SystemPrimitives::alienMalloc( smiOopFromValue( -1 ) );
    EXPECT_TRUE( pointer->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), pointer ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienMallocShouldReturnMarkedSymbolWhenSizeZero ) {
    Oop pointer = SystemPrimitives::alienMalloc( smiOopFromValue( 0 ) );
    EXPECT_TRUE( pointer->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), pointer ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienCallocShouldReturnAlignedValue ) {
    Oop pointer = SystemPrimitives::alienCalloc( smiOopFromValue( 4 ) );
    EXPECT_TRUE( pointer->is_smi() ) << "result should be SmallInteger";
    int address = SMIOop( pointer )->value();
    EXPECT_TRUE( address % 4 == 0 ) << "not aligned";
    EXPECT_TRUE( address != 0 ) << "not allocated";
    free( ( void * ) address );
}


TEST( SystemPrimitivesTests, alienCallocContentsShouldBeZero ) {
    char       message[100];
    Oop        pointer   = SystemPrimitives::alienCalloc( smiOopFromValue( 4 ) );
    const char * address = ( const char * ) SMIOop( pointer )->value();

    for ( int index = 0; index < 4; index++ ) {
        sprintf( message, "char %d should be zero", index );
        EXPECT_EQ( ( int ) address[ index ], 0 ) << message;
    }
    free( ( void * ) address );
}


TEST( SystemPrimitivesTests, alienCallocShouldReturnMarkedSymbolWhenNotSmi ) {
    Oop pointer = SystemPrimitives::alienCalloc( vmSymbols::abs() );
    EXPECT_TRUE( pointer->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_has_wrong_type() ), pointer ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienCallocShouldReturnMarkedSymbolWhenSizeNegative ) {
    Oop pointer = SystemPrimitives::alienCalloc( smiOopFromValue( -1 ) );
    EXPECT_TRUE( pointer->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), pointer ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienFreeShouldReturnMarkedSymbolWhenNotSmi ) {
    Oop result = SystemPrimitives::alienFree( vmSymbols::abs() );
    EXPECT_TRUE( result->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_has_wrong_type() ), result ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienCallocShouldReturnMarkedSymbolWhenAddressZero ) {
    Oop result = SystemPrimitives::alienCalloc( smiOopFromValue( 0 ) );

    EXPECT_TRUE( result->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), result ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienFreeShouldReturnMarkedSymbolWhenAddressZero ) {
    Oop result = SystemPrimitives::alienFree( smiOopFromValue( 0 ) );

    EXPECT_TRUE( result->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), result ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienFreeShouldReturnMarkedSymbolWhenAddressLargeIntegerZero ) {
    Oop result = SystemPrimitives::alienFree( as_large_integer( 0 ) );

    EXPECT_TRUE( result->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), result ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienFreeShouldReturnMarkedSymbolWhenLargeIntegerAddressTooBig ) {
    Oop largeInteger = as_large_integer( 256 * 256 * 256 );
    Oop tooBig       = byteArrayPrimitives::largeIntegerMultiply( largeInteger, largeInteger );
    Oop result       = SystemPrimitives::alienFree( tooBig );

    EXPECT_TRUE( result->is_mark() ) << "return should be marked";
    EXPECT_EQ( markSymbol( vmSymbols::argument_is_invalid() ), result ) << "wrong symbol returned";
}


TEST( SystemPrimitivesTests, alienFreeShouldFreeLargeIntegerAddress ) {
    int address = ( int ) malloc( 4 );
    Oop result  = SystemPrimitives::alienFree( as_large_integer( address ) );

    EXPECT_TRUE( !result->is_mark() ) << "should not be marked";
    EXPECT_TRUE( result == trueObj ) << "result should be true";
}
