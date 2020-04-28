//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/interpreter/HeapCodeBuffer.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/DoubleValueArrayKlass.hpp"
#include "vm/system/sizes.hpp"

#include <gtest/gtest.h>

// reinstate once status of dValueArray* is firmly established

extern "C" Oop * eden_top;
extern "C" Oop * eden_end;


class DoubleValueArrayKlassTests : public ::testing::Test {


    protected:

        void SetUp() override {
            theClass   = KlassOop( Universe::find_global( "DoubleValueArray" ) );
            oldEdenTop = eden_top;
        }


        void TearDown() override {
            eden_top = oldEdenTop;
            MarkSweep::collect();
        }


        KlassOop theClass;
        Oop * oldEdenTop;

};

TEST_F( DoubleValueArrayKlassTests, shouldBeDoubleValueArray ) {
    eden_top = eden_end;
    ASSERT_TRUE( theClass->klass_part()->oop_is_doubleValueArray() );
}


TEST_F( DoubleValueArrayKlassTests, allocateShouldFailWhenAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_EQ( ( int ) nullptr, ( int ) ( theClass->klass_part()->allocateObjectSize( 100, false ) ) );
}


TEST_F( DoubleValueArrayKlassTests, allocateShouldAllocateTenuredWhenRequired ) {
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObjectSize( 100, false, true ) ) );
}


TEST_F( DoubleValueArrayKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * oopSize );
    ASSERT_TRUE( Universe::new_gen.contains( theClass->klass_part()->allocateObjectSize( 100, true ) ) );
}


class findDoubleValueArray : public klassOopClosure {
    public:
        char * className;
        bool_t found;


        findDoubleValueArray() {
            className = nullptr;
            found     = false;
        }


        void do_klass( KlassOop klass ) {
            Oop instance = klass->primitive_allocate_size( 1 );
            if ( instance->is_doubleValueArray() ) {
                SymbolOop name = SymbolOop( klass->instVarAt( KlassOopDescriptor::header_size() ) );
                char * sname = name->chars();
                className = sname;
                found     = true;
                printf( "Class name is : %s)", sname );
            }
        }
};

TEST_F( DoubleValueArrayKlassTests, findDoubleValueArrayClass ) {
    findDoubleValueArray closure;
    Universe::classes_do( &closure );
    if ( !closure.found )
        printf( "No matching class found :(" );
}
