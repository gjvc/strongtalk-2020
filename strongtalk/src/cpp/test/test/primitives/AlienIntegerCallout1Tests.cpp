//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/utility/Integer.hpp"
#include "vm/utility/IntegerOps.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/memory/OopFactory.hpp"

#include <gtest/gtest.h>


typedef struct _size5 {
    std::int32_t ignore;
    char         byte;
} size5_t;

extern "C" std::int32_t __CALLING_CONVENTION callLabs( std::int32_t *ptr ) {
    return labs( *ptr );
}

extern "C" std::int32_t __CALLING_CONVENTION size5( size5_t arg ) {
    return arg.byte == -1 ? 0 : -1;
}

extern "C" Oop __CALLING_CONVENTION forceScavenge1( std::int32_t ignore ) {
    st_unused( ignore );
    Universe::scavenge();
    return vmSymbols::completed();
}

extern "C" std::int32_t __CALLING_CONVENTION argAlignment( std::int32_t a ) {
    return ( (std::int32_t) &a ) & 0xF;
}

extern "C" const char *__CALLING_CONVENTION argUnsafe1( const char *a ) {
    return a;
}


class AlienIntegerCallout1Tests : public ::testing::Test {
public:
    AlienIntegerCallout1Tests() :
        ::testing::Test(),
        rm{ nullptr },
        handles{ nullptr },
        resultAlien{ nullptr },
        addressAlien{ nullptr },
        pointerAlien{ nullptr },
        functionAlien{ nullptr },
        directAlien{ nullptr },
        invalidFunctionAlien{ nullptr },
        unsafeAlien{ nullptr },
        unsafeContents{ nullptr },
        smi0{},
        smi1{},
        address{} {}


protected:
    void SetUp() override {
        rm      = new HeapResourceMark();
        smi0    = smiOopFromValue( 0 );
        smi1    = smiOopFromValue( 1 );
        handles = new( true ) GrowableArray<PersistentHandle **>( 5 );

        allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&labs) );
        allocateAlien( resultAlien, 12, 8 );
        allocateAlien( directAlien, 12, 4 );
        allocateAlien( addressAlien, 8, -4, &address );
        allocateAlien( pointerAlien, 8, 0, &address );
        allocateAlien( invalidFunctionAlien, 8, 0 );
        allocateUnsafe( unsafeAlien, unsafeContents );

        memset( address, 0, 8 );
    }


    void TearDown() override {
        while ( !handles->isEmpty() )
            release( handles->pop() );
        free( handles );
        handles = nullptr;
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark                   *rm;
    GrowableArray<PersistentHandle **> *handles;
    PersistentHandle                   *resultAlien, *addressAlien, *pointerAlien, *functionAlien;
    PersistentHandle                   *directAlien, *invalidFunctionAlien, *unsafeAlien, *unsafeContents;
    SmallIntegerOop                    smi0, smi1;
    char                               address[16];


    void allocateAlien( PersistentHandle *&alienHandle, std::int32_t arraySize, std::int32_t alienSize, void *ptr = nullptr ) {
        ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( arraySize ) );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
        if ( ptr )
            ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) ptr ), alien );
        alienHandle = new PersistentHandle( alien );
        handles->append( &alienHandle );
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkIntResult( const char *message, std::int32_t expected, PersistentHandle *alien ) {
        st_unused( message );
        char         text[200];
        bool         ok;
        std::int32_t actual = asInt( ok, ByteArrayPrimitives::alienSignedLongAt( smi1, alien->as_oop() ) );
        EXPECT_TRUE( ok ) << "not an integer result";
        sprintf( text, "Should be: %d, was: %d", expected, actual );
        EXPECT_TRUE( actual == expected ) << text;
    }


    std::int32_t asInt( bool &ok, Oop intOop ) {
        ok = true;
        if ( intOop->isSmallIntegerOop() )
            return SmallIntegerOop( intOop )->value();
        if ( !intOop->isByteArray() ) {
            ok = false;
            return 0;
        }
        return ByteArrayOop( intOop )->number().as_int32_t( ok );
    }


    Oop asOop( std::int32_t value ) {
        std::int32_t size     = IntegerOps::int_to_Integer_result_size_in_bytes( value );
        ByteArrayOop valueOop = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( size ) );
        IntegerOps::int_to_Integer( value, valueOop->number() );
        bool ok;
        Oop  result           = valueOop->number().as_SmallIntegerOop( ok );
        return ok ? result : valueOop;
    }


    void release( PersistentHandle **handle ) {
        delete *handle;
        *handle = nullptr;
    }


    void allocateUnsafe( PersistentHandle *&handle, PersistentHandle *&contents ) {
        st_unused( handle );
        KlassOop unsafeKlass = KlassOop( Universe::find_global( "UnsafeAlien" ) );
        unsafeAlien = new PersistentHandle( unsafeKlass->primitive_allocate() );
        std::int32_t offset = unsafeKlass->klass_part()->lookup_inst_var( OopFactory::new_symbol( "nonPointerObject" ) );

        contents = new PersistentHandle( Universe::byteArrayKlassObject()->primitive_allocate_size( 12 ) );
        MemOop( unsafeAlien->as_oop() )->instVarAtPut( offset, contents->as_oop() );
    }


    void setAddress( void *p, PersistentHandle *alien ) {
        ByteArrayPrimitives::alienSetAddress( asOop( (std::int32_t) p ), alien->as_oop() );
    }


};

TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldCallFunction ) {
    ByteArrayPrimitives::alienCallResult1( smiOopFromValue( -1 ), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", labs( -1 ), resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1WithUnsafeAlienShouldCallFunction ) {
    setAddress( reinterpret_cast <void *>(&argUnsafe1), functionAlien );
    ByteArrayPrimitives::alienCallResult1( unsafeAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", (std::int32_t) ByteArrayOop( unsafeContents->as_oop() )->bytes(), resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldCallFunctionAndIgnoreResultWhenResultAlienNil ) {
    Oop result = ByteArrayPrimitives::alienCallResult1( smiOopFromValue( -1 ), nilObject, functionAlien->as_oop() );
    EXPECT_TRUE( !result->isMarkOop() ) << "shoult not be marked";
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1WithIndirectArgumentShouldCallFunction ) {
    ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( -1 ), smi1, addressAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( addressAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", labs( -1 ), resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1WithDirectArgumentShouldCallFunction ) {
    ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( -1 ), smi1, directAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( directAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", labs( -1 ), resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1WithPointerArgumentShouldCallFunction ) {
    Oop address = asOop( (std::int32_t) &callLabs );
    ByteArrayPrimitives::alienSetAddress( address, functionAlien->as_oop() );
    ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( -1 ), smi1, pointerAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( pointerAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", labs( -1 ), resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1Should16ByteAlignArgs ) {
    Oop fnAddress = asOop( (std::int32_t) &argAlignment );
    ByteArrayPrimitives::alienSetAddress( fnAddress, functionAlien->as_oop() );
    Oop result = ByteArrayPrimitives::alienCallResult1( addressAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    EXPECT_TRUE( !result->isMarkOop() ) << "Should not be marked";
    checkIntResult( "wrong result", 0, resultAlien );
    ByteArrayPrimitives::alienSetSize( smiOopFromValue( -8 ), addressAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( addressAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", 0, resultAlien );
    ByteArrayPrimitives::alienSetSize( smiOopFromValue( -12 ), addressAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( addressAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", 0, resultAlien );
    ByteArrayPrimitives::alienSetSize( smiOopFromValue( -16 ), addressAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( addressAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", 0, resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1WithOddSizedArgumentShouldCallFunction ) {
    Oop address = asOop( (std::int32_t) &size5 );
    ByteArrayPrimitives::alienSetAddress( address, functionAlien->as_oop() );
    ByteArrayPrimitives::alienSetSize( smiOopFromValue( 5 ), directAlien->as_oop() );
    ByteArrayPrimitives::alienUnsignedLongAtPut( smi0, smi1, directAlien->as_oop() );
    ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 5 ), directAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( directAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "wrong result", 0, resultAlien );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1WithScavengeShouldReturnCorrectResult ) {
    Oop address = asOop( (std::int32_t) &forceScavenge1 );
    ByteArrayPrimitives::alienSetAddress( address, functionAlien->as_oop() );
    ByteArrayPrimitives::alienCallResult1( directAlien->as_oop(), resultAlien->as_oop(), functionAlien->as_oop() );
    bool ok;
    Oop  result = (Oop) asInt( ok, ByteArrayPrimitives::alienUnsignedLongAt( smi1, resultAlien->as_oop() ) );
    EXPECT_TRUE( ok ) << "std::uint32_t at failed";
    EXPECT_TRUE ( vmSymbols::completed() == result ) << "wrong result";
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldReturnMarkedResultForNonAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult1( resultAlien->as_oop(), smi0, smi0 );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldReturnMarkedResultForDirectAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult1( resultAlien->as_oop(), smi0, resultAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = ByteArrayPrimitives::alienCallResult1( resultAlien->as_oop(), smi0, invalidFunctionAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldReturnMarkedResultWhenResultNotAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult1( smi0, smi0, functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout1Tests, alienCallResult1ShouldReturnMarkedResultWhenFunctionParameterNotAlienOrSMI ) {
    Oop result = ByteArrayPrimitives::alienCallResult1( Universe::byteArrayKlassObject(), resultAlien->as_oop(), functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}
