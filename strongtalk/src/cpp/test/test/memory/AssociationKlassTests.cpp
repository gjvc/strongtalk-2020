//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/oops/AssociationKlass.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/system/sizes.hpp"


#include <gtest/gtest.h>


class AssociationKlassTests : public ::testing::Test {
protected:

    void SetUp() override {

    }


    void TearDown() override {
        MarkSweep::collect();
    }

};


TEST_F( AssociationKlassTests, shouldAllocateTenured
) {
HandleMark mark;
Handle     objectClass( Universe::find_global( "GlobalAssociation" ) );
Oop        assoc       = ( (AssociationKlass *) objectClass.as_klass()->klass_part() )->allocateObject( true );
ASSERT_TRUE( Universe::old_gen
.
contains( assoc )
);
}


TEST_F( AssociationKlassTests, allocateShouldFailWhenAllowedAndNoSpace
) {
HandleMark mark;
Handle     objectClass( Universe::find_global( "GlobalAssociation" ) );
{
OldSpaceMark oldMark( Universe::old_gen.top_mark()._space );
int          freeSpace = Universe::old_gen.free();
Universe::allocate_tenured( freeSpace
/ oopSize - 1 );
ASSERT_TRUE( Universe::old_gen
.
free()
< 5 * oopSize );
ASSERT_EQ( ( int ) nullptr, ( int ) ( ( AssociationKlass * ) objectClass.as_klass()->klass_part() )->allocateObject( false ) );
}
}


TEST_F( AssociationKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace
) {
HandleMark mark;
Handle     objectClass( Universe::find_global( "GlobalAssociation" ) );
{
OldSpaceMark oldMark( Universe::old_gen.top_mark()._space );
int          freeSpace = Universe::old_gen.free();
Universe::allocate_tenured( freeSpace
/ oopSize - 1 );
ASSERT_TRUE( Universe::old_gen
.
free()
< 5 * oopSize );
ASSERT_TRUE( Universe::old_gen
.
contains( ( ( AssociationKlass
* ) objectClass.as_klass()->klass_part() )->allocateObject( true ) ) );
}
}
