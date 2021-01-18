
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/interpreter/MissingMethodBuilder.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/code/StubRoutines.hpp"


#include <gtest/gtest.h>


class MissingMethodBuilderTests : public ::testing::Test {

protected:

    void SetUp() override {
        rm = new HeapResourceMark();
    }


    void TearDown() override {
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark *rm;
    char msg[200];


    int instVarIndex( KlassOop targetClass, const char *instVarName ) {
        SymbolOop varNameSymbol = oopFactory::new_symbol( instVarName );
        return targetClass->klass_part()->lookup_inst_var( varNameSymbol );
    }


    void CHECK_OOPS( Oop expectedOops[], ObjectArrayOop oops, int index ) {
        Oop expected = expectedOops[ index ];
        Oop actual   = oops->obj_at( index + 1 );
        sprintf( msg, "Incorrect Oop at %d. Expected: 0x%x, but was: 0x%x", index, expected, actual );
        EXPECT_EQ( int( expected ), int( actual ) ) << msg;
    }
};


TEST_F( MissingMethodBuilderTests, buildWithNoArgSelectorShouldBuildCorrectBytes
) {
BlockScavenge        bs;
char                 msg[200];
SymbolOop            selector          = oopFactory::new_symbol( "value" );
std::uint8_t         expectedBytes[52] = { static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_self), static_cast<std::uint8_t>(ByteCodes::Code::push_literal), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_literal), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_n), 3, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_self), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_0), 0xFF, 0xFF, 0xFF };
MissingMethodBuilder builder( selector );
builder.
build();
ByteArrayOop         bytes             = builder.bytes();
EXPECT_EQ( 52, bytes->
length()
) << "wrong length";
for (
int index = 0;
index < 52; index++ ) {
std::uint8_t expected         = expectedBytes[ index ];
std::uint8_t actual           = bytes->byte_at( index + 1 );
sprintf( msg,
"Incorrect byte at %d. Expected: %d, but was: %d", index, expected, actual );
EXPECT_EQ( expected, actual
) <<
msg;
}
}


TEST_F( MissingMethodBuilderTests, buildWithNoArgSelectorShouldBuildCorrectOops
) {
BlockScavenge        bs;
char                 msg[200];
SymbolOop            selector = oopFactory::new_symbol( "value" );
MissingMethodBuilder builder( selector );
builder.
build();
ObjectArrayOop       oops     = builder.oops();

Oop expectedOops[13] = { smiOopFromValue( 0 ), Universe::find_global_association( "Message" ), smiOopFromValue( 0 ), selector, smiOopFromValue( 0 ), smiOopFromValue( 0 ), smiOopFromValue( 0 ), oopFactory::new_symbol( "receiver:selector:arguments:" ), smiOopFromValue( 0 ), smiOopFromValue( 0 ), oopFactory::new_symbol( "doesNotUnderstand:" ), smiOopFromValue( 0 ), smiOopFromValue( 0 ) };

for (
int index = 0;
index < 5; index++ ) {
CHECK_OOPS( expectedOops, oops, index
);
}

Oop obj = oops->obj_at( 6 );

EXPECT_TRUE( obj
->
is_objArray()
) << "Wrong type";
ObjectArrayOop array = ObjectArrayOop( obj );
EXPECT_EQ( 0, array->
length()
) << "Wrong length";
for (
int index = 6;
index < 13; index++ ) {
CHECK_OOPS( expectedOops, oops, index
);
}
}


typedef Oop (call_delta_func)( void *method, Oop receiver, int nofArgs, Oop *args );

TEST_F( MissingMethodBuilderTests, buildWithNoArgSelectorShouldBuildCorrectMethod
) {
call_delta_func *_call_delta      = (call_delta_func *) StubRoutines::call_delta();
BlockScavenge        bs;
SymbolOop            selector     = oopFactory::new_symbol( "value" );
MissingMethodBuilder builder( selector );
builder.
build();
MethodOop            method       = builder.method();
KlassOop             objectClass  = KlassOop( Universe::find_global( "DoesNotUnderstandFixture" ) );
KlassOop             messageClass = KlassOop( Universe::find_global( "Message" ) );
Oop                  fixture      = objectClass->klass_part()->allocateObject();
MemOop               result       = MemOop( _call_delta( method, fixture, 0, nullptr ) );
EXPECT_TRUE( result
->
is_mem()
) << "Wrong type";
EXPECT_TRUE( result
->
klass()
== messageClass ) << "Wrong class";
int receiverIndex  = instVarIndex( messageClass, "receiver" );
int selectorIndex  = instVarIndex( messageClass, "selector" );
int argumentsIndex = instVarIndex( messageClass, "arguments" );
EXPECT_TRUE( fixture
== result->
instVarAt( receiverIndex )
) << "Wrong receiver";
EXPECT_TRUE( selector
== result->
instVarAt( selectorIndex )
) << "Wrong selector";
ObjectArrayOop arguments = ObjectArrayOop( result->instVarAt( argumentsIndex ) );
EXPECT_TRUE( arguments
->
is_objArray()
) << "Wrong type";
EXPECT_TRUE( 0 == arguments->
length()
) << "Wrong length";
}


TEST_F( MissingMethodBuilderTests, buildWithOneArgSelectorShouldBuildCorrectMethod
) {
call_delta_func *_call_delta      = (call_delta_func *) StubRoutines::call_delta();
BlockScavenge        bs;
SymbolOop            selector     = oopFactory::new_symbol( "value:" );
MissingMethodBuilder builder( selector );
builder.
build();
MethodOop            method       = builder.method();
KlassOop             objectClass  = KlassOop( Universe::find_global( "DoesNotUnderstandFixture" ) );
KlassOop             messageClass = KlassOop( Universe::find_global( "Message" ) );
Oop                  fixture      = objectClass->klass_part()->allocateObject();
Oop                  arg1in       = smiOopFromValue( 53 );
MemOop               result       = MemOop( _call_delta( method, fixture, 1, &arg1in ) );
EXPECT_TRUE( result
->
is_mem()
) << "Wrong type";
EXPECT_TRUE( result
->
klass()
== messageClass ) << "Wrong class";
int receiverIndex  = instVarIndex( messageClass, "receiver" );
int selectorIndex  = instVarIndex( messageClass, "selector" );
int argumentsIndex = instVarIndex( messageClass, "arguments" );
EXPECT_TRUE( fixture
== result->
instVarAt( receiverIndex )
) << "Wrong receiver";
EXPECT_TRUE( selector
== result->
instVarAt( selectorIndex )
) << "Wrong selector";
ObjectArrayOop arguments = ObjectArrayOop( result->instVarAt( argumentsIndex ) );
EXPECT_TRUE( arguments
->
is_objArray()
) << "Arguments has wrong type";
EXPECT_TRUE( 1 == arguments->
length()
) << "Wrong argument count";
SMIOop arg1 = SMIOop( arguments->obj_at( 1 ) );
EXPECT_TRUE( arg1
->
is_smi()
) << "Argument 1 has wrong type";
EXPECT_EQ( 53, arg1->
value()
) << "Argument 1 has wrong value";
}


TEST_F( MissingMethodBuilderTests, buildWithTwoArgSelectorShouldBuildCorrectMethod
) {
call_delta_func *_call_delta      = (call_delta_func *) StubRoutines::call_delta();
BlockScavenge        bs;
SymbolOop            selector     = oopFactory::new_symbol( "value:value:" );
MissingMethodBuilder builder( selector );
builder.
build();
MethodOop            method       = builder.method();
KlassOop             objectClass  = KlassOop( Universe::find_global( "DoesNotUnderstandFixture" ) );
KlassOop             messageClass = KlassOop( Universe::find_global( "Message" ) );
Oop                  fixture      = objectClass->klass_part()->allocateObject();
Oop                  args[]       = { smiOopFromValue( 42 ), smiOopFromValue( 53 ) };
MemOop               result       = MemOop( _call_delta( method, fixture, 2, args ) );
EXPECT_TRUE( result
->
is_mem()
) << "Wrong type";
EXPECT_TRUE( result
->
klass()
== messageClass ) << "Wrong class";
int receiverIndex  = instVarIndex( messageClass, "receiver" );
int selectorIndex  = instVarIndex( messageClass, "selector" );
int argumentsIndex = instVarIndex( messageClass, "arguments" );
EXPECT_TRUE( fixture
== result->
instVarAt( receiverIndex )
) << "Wrong receiver";
EXPECT_TRUE( selector
== result->
instVarAt( selectorIndex )
) << "Wrong selector";
ObjectArrayOop arguments = ObjectArrayOop( result->instVarAt( argumentsIndex ) );
EXPECT_TRUE( arguments
->
is_objArray()
) << "Arguments has wrong type";
EXPECT_TRUE( 2 == arguments->
length()
) << "Wrong argument count";
SMIOop arg1 = SMIOop( arguments->obj_at( 1 ) );
EXPECT_TRUE( arg1
->
is_smi()
) << "Argument 1 has wrong type";
EXPECT_EQ( 42, arg1->
value()
) << "Argument 1 has wrong value";
SMIOop arg2 = SMIOop( arguments->obj_at( 2 ) );
EXPECT_TRUE( arg2
->
is_smi()
) << "Argument 2 has wrong type";
EXPECT_EQ( 53, arg2->
value()
) << "Argument 2 has wrong value";
}


TEST_F( MissingMethodBuilderTests, buildWithThreeArgSelectorShouldBuildCorrectMethod
) {
call_delta_func *_call_delta      = (call_delta_func *) StubRoutines::call_delta();
BlockScavenge        bs;
SymbolOop            selector     = oopFactory::new_symbol( "value:value:value:" );
MissingMethodBuilder builder( selector );
builder.
build();
MethodOop            method       = builder.method();
KlassOop             objectClass  = KlassOop( Universe::find_global( "DoesNotUnderstandFixture" ) );
KlassOop             messageClass = KlassOop( Universe::find_global( "Message" ) );
Oop                  fixture      = objectClass->klass_part()->allocateObject();
Oop                  args[]       = { smiOopFromValue( 11 ), smiOopFromValue( 42 ), smiOopFromValue( 53 ) };
MemOop               result       = MemOop( _call_delta( method, fixture, 3, args ) );
EXPECT_TRUE( result
->
is_mem()
) << "Wrong type";
EXPECT_TRUE( result
->
klass()
== messageClass ) << "Wrong class";
int receiverIndex  = instVarIndex( messageClass, "receiver" );
int selectorIndex  = instVarIndex( messageClass, "selector" );
int argumentsIndex = instVarIndex( messageClass, "arguments" );
EXPECT_TRUE( fixture
== result->
instVarAt( receiverIndex )
) << "Wrong receiver";
EXPECT_TRUE( selector
== result->
instVarAt( selectorIndex )
) << "Wrong selector";
ObjectArrayOop arguments = ObjectArrayOop( result->instVarAt( argumentsIndex ) );
EXPECT_TRUE( arguments
->
is_objArray()
) << "Arguments has wrong type";
EXPECT_TRUE( 3 == arguments->
length()
) << "Wrong argument count";
SMIOop arg1 = SMIOop( arguments->obj_at( 1 ) );
EXPECT_TRUE( arg1
->
is_smi()
) << "Argument 1 has wrong type";
EXPECT_EQ( 11, arg1->
value()
) << "Argument 1 has wrong value";
SMIOop arg2 = SMIOop( arguments->obj_at( 2 ) );
EXPECT_TRUE( arg2
->
is_smi()
) << "Argument 2 has wrong type";
EXPECT_EQ( 42, arg2->
value()
) << "Argument 2 has wrong value";
SMIOop arg3                            = SMIOop( arguments->obj_at( 3 ) );
EXPECT_TRUE( arg3
->
is_smi()
) << "Argument 3 has wrong type";
EXPECT_EQ( 53, arg3->
value()
) << "Argument 3 has wrong value";
}


TEST_F( MissingMethodBuilderTests, buildWithOneArgSelectorShouldBuildCorrectBytes
) {
BlockScavenge        bs;
char                 msg[200];
SymbolOop            selector          = oopFactory::new_symbol( "value:" );
std::uint8_t         expectedBytes[80] = { static_cast<std::uint8_t>(ByteCodes::Code::allocate_temp_1), static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_self), static_cast<std::uint8_t>(ByteCodes::Code::push_literal), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_1), 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::store_temp_n), 0xFF, static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop), 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0), static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_n), 3, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0,
                                           static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_self),
                                           0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_1), 0xff, 0xff, 0xff };
MissingMethodBuilder builder( selector );
builder.
build();
ByteArrayOop         bytes             = builder.bytes();
sprintf( msg,
"Wrong length. Expected: %d, but was: %d", 65, bytes->
length()
);
EXPECT_EQ( 80, bytes->
length()
) <<
msg;
for (
int index = 0;
index < 80; index++ ) {
std::uint8_t expected                 = expectedBytes[ index ];
std::uint8_t actual                   = bytes->byte_at( index + 1 );
sprintf( msg,
"Incorrect byte at %d. Expected: %d, but was: %d", index, expected, actual );
EXPECT_EQ( expected, actual
) <<
msg;
}
}


TEST_F( MissingMethodBuilderTests, buildWithOneArgSelectorShouldBuildCorrectOops
) {
BlockScavenge        bs;
char                 msg[200];
SymbolOop            selector         = oopFactory::new_symbol( "value:" );
Oop                  expectedOops[20] = { smiOopFromValue( 0 ), Universe::find_global_association( "Message" ), smiOopFromValue( 0 ), selector, smiOopFromValue( 0 ), Universe::find_global_association( "Array" ), smiOopFromValue( 0 ), oopFactory::new_symbol( "new:" ), smiOopFromValue( 0 ), smiOopFromValue( 0 ), smiOopFromValue( 0 ), oopFactory::new_symbol( "at:put:" ), smiOopFromValue( 0 ), smiOopFromValue( 0 ), oopFactory::new_symbol( "receiver:selector:arguments:" ), smiOopFromValue( 0 ), smiOopFromValue( 0 ), oopFactory::new_symbol( "doesNotUnderstand:" ), smiOopFromValue( 0 ), smiOopFromValue( 0 ) };
MissingMethodBuilder builder( selector );
builder.
build();
ObjectArrayOop       oops             = builder.oops();
for (
int index                              = 0;
index < 20; index++ ) {
CHECK_OOPS( expectedOops, oops, index
);
}
}


TEST_F( MissingMethodBuilderTests, buildWithTwoArgSelectorShouldBuildCorrectBytes
) {
BlockScavenge        bs;
char                 msg[200];
SymbolOop            selector          = oopFactory::new_symbol( "value:value:" );
std::uint8_t         expectedBytes[96] = { static_cast<std::uint8_t>(ByteCodes::Code::allocate_temp_1), static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_self), static_cast<std::uint8_t>(ByteCodes::Code::push_literal), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 1, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_1), 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::store_temp_n), 0xFF, static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n), 1, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop), 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0), static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 1,
                                           static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop), 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0,
                                           static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0), static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_n), 3, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_self), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0,
                                           static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_2), 0xff, 0xff, 0xff };
MissingMethodBuilder builder( selector );
builder.
build();
ByteArrayOop         bytes             = builder.bytes();
sprintf( msg,
"Wrong length. Expected: %d, but was: %d", 93, bytes->
length()
);
EXPECT_EQ( 96, bytes->
length()
) <<
msg;
for (
int index = 0;
index < 96; index++ ) {
std::uint8_t expected                   = expectedBytes[ index ];
std::uint8_t actual                     = bytes->byte_at( index + 1 );
sprintf( msg,
"Incorrect byte at %d. Expected: %d, but was: %d", index, expected, actual );
EXPECT_EQ( expected, actual
) <<
msg;
}
}


TEST_F( MissingMethodBuilderTests, buildWithThreeArgSelectorShouldBuildCorrectBytes
) {
BlockScavenge        bs;
char                 msg[200];
SymbolOop            selector           = oopFactory::new_symbol( "value:value:value:" );
std::uint8_t         expectedBytes[112] = { static_cast<std::uint8_t>(ByteCodes::Code::allocate_temp_1), static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_self), static_cast<std::uint8_t>(ByteCodes::Code::push_literal), 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_global), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 2, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_1), 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::store_temp_n), 0xFF, static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n), 2, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop), 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0), static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 1,
                                            static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n), 1, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop), 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0), static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n), 2, static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n), 0, static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop), 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0),
                                            static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_n), 3, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0,
                                            static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_self), 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_n), 3, 0xff, 0xff };
MissingMethodBuilder builder( selector );
builder.
build();
ByteArrayOop         bytes              = builder.bytes();
sprintf( msg,
"Wrong length. Expected: %d, but was: %d", 110, bytes->
length()
);
EXPECT_EQ( 112, bytes->
length()
) <<
msg;
for (
int index = 0;
index < 112; index++ ) {
std::uint8_t expected = expectedBytes[ index ];
std::uint8_t actual   = bytes->byte_at( index + 1 );
sprintf( msg,
"Incorrect byte at %d. Expected: %d, but was: %d", index, expected, actual );
EXPECT_EQ( expected, actual
) <<
msg;
}
}
