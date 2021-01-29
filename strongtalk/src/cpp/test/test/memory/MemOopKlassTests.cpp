//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"

#include <gtest/gtest.h>

extern "C" Oop *eden_top;
extern "C" Oop *eden_end;


class MemOopKlassTests : public ::testing::Test {

protected:
    void SetUp() override {
        theClass   = KlassOop( Universe::find_global( "Object" ) );
        oldEdenTop = eden_top;
    }


    void TearDown() override {
        eden_top = oldEdenTop;
        MarkSweep::collect();
    }


    MixinOop createMixin() {
        PersistentHandle classMixinClass( Universe::find_global( "ClassMixin" ) );
        PersistentHandle metaClassMixinClass( Universe::find_global( "MetaClassMixin" ) );

        PersistentHandle methods( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle ivars( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle classMethods( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle classIvars( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle classVars( oopFactory::new_objArray( std::int32_t{0} ) );

        PersistentHandle classMixin( classMixinClass.as_klassOop()->klass_part()->allocateObject() );
        PersistentHandle metaClassMixin( metaClassMixinClass.as_klassOop()->klass_part()->allocateObject() );

        MixinOop klassMixin = MixinOop( classMixin.as_oop() );
        klassMixin->set_methods( ObjectArrayOop( methods.as_oop() ) );
        klassMixin->set_instVars( ObjectArrayOop( ivars.as_oop() ) );
        klassMixin->set_classVars( ObjectArrayOop( classVars.as_oop() ) );
        klassMixin->set_class_mixin( MixinOop( metaClassMixin.as_oop() ) );
        klassMixin->set_installed( falseObject );

        MixinOop metaKlassMixin = MixinOop( classMixin.as_oop() );
        metaKlassMixin->set_methods( ObjectArrayOop( classMethods.as_oop() ) );
        metaKlassMixin->set_instVars( ObjectArrayOop( classIvars.as_oop() ) );
        metaKlassMixin->set_classVars( ObjectArrayOop( classVars.as_oop() ) );
        metaKlassMixin->set_installed( falseObject );

        return klassMixin;
    }


    KlassOop theClass;
    Oop *oldEdenTop;


};

TEST_F( MemOopKlassTests, createSubclassShouldCreateClassWithCorrectSuperForClassAndMeta ) {
    PersistentHandle kl( Universe::find_global( "Class" ) );
    PersistentHandle mk( Universe::find_global( "Metaclass" ) );
    PersistentHandle instSuper( Universe::find_global( "Test" ) );
    ASSERT_TRUE( kl.as_oop() );
    PersistentHandle classMixin( createMixin() );
    KlassOop         newKlass = kl.as_klassOop()->klass_part()->create_subclass( MixinOop( classMixin.as_oop() ), instSuper.as_klassOop(), mk.as_klassOop(), Klass::Format::mem_klass );
    ASSERT_TRUE( newKlass );
    ASSERT_TRUE( newKlass->klass_part()->superKlass() == instSuper.as_klassOop() );
    ASSERT_TRUE( newKlass->klass()->klass_part()->superKlass() == kl.as_klassOop() );
    ASSERT_TRUE( newKlass->klass()->klass() == mk.as_klassOop() );
    ASSERT_TRUE( newKlass->klass_part()->mixin() == classMixin.as_oop() );
    ASSERT_TRUE( newKlass->klass()->klass_part()->mixin() == MixinOop( classMixin.as_oop() )->class_mixin() );
}


TEST_F( MemOopKlassTests, oldCreateSubclassShouldCreateClassWithCorrectSuperForClassAndMeta ) {
    PersistentHandle kl( Universe::find_global( "Class" ) );
    PersistentHandle mk( Universe::find_global( "Metaclass" ) );
    PersistentHandle instSuper( Universe::find_global( "Test" ) );
    ASSERT_TRUE( kl.as_oop() );
    PersistentHandle classMixin( createMixin() );
    KlassOop         newKlass = instSuper.as_klassOop()->klass_part()->create_subclass( MixinOop( classMixin.as_oop() ), Klass::Format::mem_klass );
    KlassOop         superClass = instSuper.as_klassOop();
    MixinOop         mixin = MixinOop( classMixin.as_oop() );
    ASSERT_TRUE( newKlass );
    ASSERT_TRUE( newKlass->klass_part()->superKlass() == superClass );
    ASSERT_TRUE( newKlass->klass()->klass_part()->superKlass() == superClass->klass() );
    ASSERT_TRUE( newKlass->klass()->klass() == superClass->klass()->klass() );
    ASSERT_TRUE( newKlass->klass_part()->mixin() == mixin );
    ASSERT_TRUE( newKlass->klass()->klass_part()->mixin() == mixin->class_mixin() );
}
