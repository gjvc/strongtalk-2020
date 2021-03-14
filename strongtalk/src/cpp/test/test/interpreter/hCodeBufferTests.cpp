//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/interpreter/HeapCodeBuffer.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"

#include <gtest/gtest.h>


class HCodeBufferTests : public ::testing::Test {

protected:

    void SetUp() override {
        rm   = new HeapResourceMark();
        code = new HeapCodeBuffer();
    }


    void TearDown() override {
        delete rm;
        rm   = nullptr;
        code = nullptr;
    }


    HeapCodeBuffer   *code;
    HeapResourceMark *rm;
    char             msg[200];


    void checkByteLength( std::int32_t expected, const char *message ) {
        sprintf( msg, "Wrong byte length for %s, expected: %d, but was: %d", message, expected, code->byteLength() );
        EXPECT_EQ( expected, code->byteLength() ) << msg;
    }


    void checkOopLength( std::int32_t expected, const char *message ) {
        sprintf( msg, "Wrong Oop length for %s, expected: %d, but was: %d", message, expected, code->oopLength() );
        EXPECT_EQ( expected, code->oopLength() ) << msg;
    }


    void checkByte( std::int32_t expected, std::int32_t actual ) {
        sprintf( msg, "Expected: %d, but was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << msg;
    }


    void checkOop( std::int32_t expected, std::int32_t actual ) {
        sprintf( msg, "Expected: %d, but was: %d", expected, actual );
        EXPECT_EQ( std::int32_t( expected ), std::int32_t( actual ) ) << msg;
    }

};


TEST_F( HCodeBufferTests, pushingByteShouldAddByteToBytesArray
) {
    BlockScavenge  bs;
    KlassOop       messageClass  = KlassOop( Universe::find_global( "Message" ) );
    SymbolOop      errorSelector = SymbolOop( OopFactory::new_symbol( "value" ) );
    SymbolOop      selector      = SymbolOop( OopFactory::new_symbol( "receiver:selector:arguments:" ) );
    SymbolOop      dnuSelector   = SymbolOop( OopFactory::new_symbol( "doesNotUnderstand:" ) );
    ObjectArrayOop args          = ObjectArrayOop( OopFactory::new_objectArray( std::int32_t{ 0 } ) );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::push_literal)
    );
    code->pushOop( messageClass );
    checkByteLength( 8, "Message" );
    checkOopLength( 2, "Message" );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::push_self)
    );
    checkByteLength( 9, "self" );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::push_literal)
    );
    code->
        pushOop( errorSelector );
    checkByteLength( 16, "selector" );
    checkOopLength( 4, "selector" );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::push_literal)
    );
    code->
        pushOop( args );
    checkByteLength( 24, "args" );
    checkOopLength( 6, "args" );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::interpreted_send_n)
    );
    code->pushByte( 2 );
    code->
        pushOop( selector );
    code->
        pushOop( smiOopFromValue( 0 )
    );
    checkByteLength( 36, "constructor" );
    checkOopLength( 9, "constructor" );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::interpreted_send_self)
    );
    code->
        pushOop( dnuSelector );
    code->
        pushOop( smiOopFromValue( 0 )
    );
    checkByteLength( 48, "DNU" );
    checkOopLength( 12, "DNU" );
    code->pushByte( static_cast
                        <std::uint8_t>(ByteCodes::Code::return_tos_pop_0)
    );
    checkByteLength( 49, "return" );
}


TEST_F( HCodeBufferTests, pushingBytesShouldAddCorrectBytesToByteArray
) {
    code->pushByte( 1 );
    code->pushByte( 2 );
    code->pushByte( 3 );
    code->pushByte( 4 );
    checkByte( 1, code->bytes()->byte_at( 1 ) );
    checkByte( 2, code->bytes()->byte_at( 2 ) );
    checkByte( 3, code->bytes()->byte_at( 3 ) );
    checkByte( 4, code->bytes()->byte_at( 4 ) );
}


TEST_F( HCodeBufferTests, pushingOopShouldPadByteArray
) {
    code->pushByte( 1 );
    code->
        pushOop( smiOopFromValue( 2 )
    );
    checkByte( 1, code->bytes()->byte_at( 1 ) );
    checkByte( 0xFF, code->bytes()->byte_at( 2 ) );
    checkByte( 0xFF, code->bytes()->byte_at( 3 ) );
    checkByte( 0xFF, code->bytes()->byte_at( 4 ) );
    checkByte( 0, code->bytes()->byte_at( 5 ) );
    checkByte( 0, code->bytes()->byte_at( 6 ) );
    checkByte( 0, code->bytes()->byte_at( 7 ) );
    checkByte( 0, code->bytes()->byte_at( 8 ) );
}


TEST_F( HCodeBufferTests, pushingOopShouldAddToOopArray
) {
    BlockScavenge bs;
    code->
        pushOop( smiOopFromValue( 2 )
    );
//    checkOop( smiOopFromValue( 2 ), code->oops()->obj_at( 1 ) ); // XXX
}


TEST_F( HCodeBufferTests, pushingByteShouldAddZeroToOopArray
) {
    BlockScavenge bs;
    code->pushByte( 1 );
//    checkOop( smiOopFromValue( 0 ), code->oops()->obj_at( 1 ) ); // XXX
}
