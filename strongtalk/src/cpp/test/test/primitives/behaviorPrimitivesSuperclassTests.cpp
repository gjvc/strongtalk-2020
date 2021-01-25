//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/primitives/behavior_primitives.hpp"
#include "vm/primitives/system_primitives.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/ResourceMark.hpp"

#include <gtest/gtest.h>


#define checkDoesntExist( className ) {\
    ResourceMark resourceMark;\
    char message[100];\
    const char* name = as_symbol(className)->as_string();\
    \
    sprintf(message, "Class '%s' already exists", name);\
    EXPECT_TRUE( !Universe::find_global(name) ) << message;\
  }

#define checkClass( classHandle ) {\
    EXPECT_TRUE( classHandle->as_oop()->is_mem() ) << "Should be MemOop";\
    EXPECT_TRUE( Universe::really_contains( classHandle->as_oop() ) ) << "Should be in Universe";\
  }


class BehaviorPrimitivesSuperclassTests : public ::testing::Test {

protected:
    void SetUp() override {
        delta                = new PersistentHandle( Universe::find_global( "Smalltalk" ) );
        objectClassHandle    = new PersistentHandle( Universe::find_global( "Object" ) );
        classClassHandle     = new PersistentHandle( Universe::find_global( "Class" ) );
        delayClassHandle     = new PersistentHandle( Universe::find_global( "Delay" ) );
        metaclassClassHandle = new PersistentHandle( Universe::find_global( "Metaclass" ) );
        classMetaclass       = new PersistentHandle( classClassHandle->as_klassOop()->klass_part()->superKlass() );
        subclassName         = new PersistentHandle( oopFactory::new_symbol( "BehaviorPrimsSubclassFixture" ) );
        superclassName       = new PersistentHandle( oopFactory::new_symbol( "BehaviorPrimsSuperclassFixture" ) );
        topClassName         = new PersistentHandle( oopFactory::new_symbol( "BehaviorPrimsTopClassFixture" ) );
        delaySubclassName    = new PersistentHandle( oopFactory::new_symbol( "BehaviorPrimsDelayFixture" ) );

        checkDoesntExist( superclassName );
        checkDoesntExist( subclassName );
        checkDoesntExist( delaySubclassName );
        checkDoesntExist( topClassName );

        subclassHandle = new PersistentHandle( createClass( as_symbol( subclassName ), objectClass() ) );
        checkClass( subclassHandle );
        superclassHandle = new PersistentHandle( createClass( as_symbol( superclassName ), objectClass() ) );
        checkClass( superclassHandle );
        delaySubclassHandle = new PersistentHandle( createClass( as_symbol( delaySubclassName ), delayClass() ) );
        checkClass( delaySubclassHandle );
        topClassHandle = new PersistentHandle( createClass( as_symbol( topClassName ), objectClass() ) );
        checkClass( topClassHandle );
    }


    void TearDown() override {
        if ( subclassHandle )
            remove( subclassName );
        if ( superclassHandle )
            remove( superclassName );
        if ( delaySubclassHandle )
            remove( delaySubclassName );
        if ( topClassHandle )
            remove( topClassName );

        safeDelete( delta );
        safeDelete( classMetaclass );
        safeDelete( classClassHandle );
        safeDelete( metaclassClassHandle );
        safeDelete( objectClassHandle );
        safeDelete( subclassName );
        safeDelete( delaySubclassName );
        safeDelete( superclassName );
        safeDelete( subclassHandle );
        safeDelete( topClassHandle );
        safeDelete( delaySubclassHandle );
        safeDelete( superclassHandle );
    }


    PersistentHandle *delta;
    PersistentHandle *objectClassHandle;
    PersistentHandle *classClassHandle;
    PersistentHandle *delayClassHandle;
    PersistentHandle *metaclassClassHandle;
    PersistentHandle *classMetaclass;
    PersistentHandle *subclassHandle;
    PersistentHandle *topClassHandle;
    PersistentHandle *delaySubclassHandle;
    PersistentHandle *superclassHandle;
    PersistentHandle *subclassName;
    PersistentHandle *topClassName;
    PersistentHandle *delaySubclassName;
    PersistentHandle *superclassName;


    MixinOop createEmptyMixin() {
        PersistentHandle metaclassMixinHandle = createMixinSide( "MetaClassMixin" );
        MixinOop         classMixin           = createMixinSide( "ClassMixin" );

        classMixin->set_class_mixin( MixinOop( metaclassMixinHandle.as_oop() ) );

        return classMixin;
    }


    MixinOop createMixinSide( const char *mixinClassName ) {
        PersistentHandle classHandle( Universe::find_global( mixinClassName ) );
        PersistentHandle methods( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle ivars( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle classMethods( oopFactory::new_objArray( std::int32_t{0} ) );
        PersistentHandle classVars( oopFactory::new_objArray( std::int32_t{0} ) );

        MixinOop mixin = MixinOop( classHandle.as_klassOop()->klass_part()->allocateObject() );
        mixin->set_methods( ObjectArrayOop( methods.as_oop() ) );
        mixin->set_instVars( ObjectArrayOop( ivars.as_oop() ) );
        mixin->set_classVars( ObjectArrayOop( classVars.as_oop() ) );
        mixin->set_installed( falseObject );

        return mixin;
    }


    KlassOop createClass( SymbolOop className, KlassOop superclass ) {
        PersistentHandle superclassHandle( superclass );
        char             name[100];
        {
            ResourceMark resourceMark;
            strcpy( name, className->as_string() );
        }

        PersistentHandle classNameHandle( className );
        SymbolOop        format = oopFactory::new_symbol( "Oops" );
        MixinOop         mixin  = createEmptyMixin();
        return KlassOop( SystemPrimitives::createNamedInvocation( format, superclassHandle.as_oop(), trueObject, className, mixin ) );
    }


    SymbolOop as_symbol( PersistentHandle *handle ) {
        return SymbolOop( handle->as_oop() );
    }


    void safeDelete( PersistentHandle *handle ) {
        if ( handle )
            delete ( handle );
    }


    void remove( PersistentHandle *classNameHandle ) {
        ResourceMark resourceMark;
        SymbolOop    className = as_symbol( classNameHandle );
        const char *name = className->as_string();

        if ( Universe::find_global( name ) )
            Delta::call( delta->as_oop(), oopFactory::new_symbol( "removeKey:" ), className );
    }


    KlassOop superclassOf( KlassOop aClass ) {
        return aClass->klass_part()->superKlass();
    }


    KlassOop subclass() {
        return subclassHandle->as_klassOop();
    }


    KlassOop delaySubclass() {
        return delaySubclassHandle->as_klassOop();
    }


    KlassOop superclass() {
        return superclassHandle->as_klassOop();
    }


    KlassOop objectClass() {
        return objectClassHandle->as_klassOop();
    }


    KlassOop delayClass() {
        return delayClassHandle->as_klassOop();
    }


    KlassOop classClass() {
        return classClassHandle->as_klassOop();
    }


    KlassOop topClass() {
        return topClassHandle->as_klassOop();
    }


    KlassOop metaclassClass() {
        return metaclassClassHandle->as_klassOop();
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        ResourceMark resourceMark;
        char         text[200];
        EXPECT_TRUE( result->is_mark() ) << "Should be marked";
        sprintf( text, "%s. Should be: %s, was: %s", message, expected->as_string(), unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkNotMarkedSymbol( Oop result ) {
        if ( !result->is_mark() )
            return;
        ResourceMark resourceMark;
        char         text[200];
        sprintf( text, "Unexpected marked result was: %s", unmarkSymbol( result )->as_string() );
        FAIL() << text;
    }


};


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldChangeSuperclassToNewClass
) {
EXPECT_TRUE( superclassOf( subclass() )
==
objectClass()
) << "Original superclassHandle";

Oop result = behaviorPrimitives::setSuperclass( superclass(), subclass() );

EXPECT_TRUE ( subclass()
== result ) << "Should return receiver";
EXPECT_TRUE( superclassOf( subclass() )
==
superclass()
) << "Superclass should have changed";
EXPECT_TRUE( superclassOf( subclass()->klass() )
== objectClass()->klass() ) << "Metasuperclass should be unchanged";
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldChangeSuperclassToNil
) {
EXPECT_TRUE( superclassOf( subclass() )
==
objectClass()
) << "Original superclassHandle";

Oop result = behaviorPrimitives::setSuperclass( nilObject, subclass() );

EXPECT_TRUE ( subclass()
== result ) << "Should return receiver";
EXPECT_TRUE( superclassOf( subclass() )
== nilObject ) << "Superclass should have changed";
EXPECT_TRUE( superclassOf( subclass()->klass() )
== objectClass()->klass() ) << "Metasuperclass should be unchanged";
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldChangeNilSuperclassToNotNil
) {
EXPECT_TRUE( superclassOf( subclass() )
==
objectClass()
) << "Original superclassHandle";

Oop result = behaviorPrimitives::setSuperclass( nilObject, subclass() );

EXPECT_TRUE( superclassOf( subclass() )
== nilObject ) << "Superclass should have changed";

result = behaviorPrimitives::setSuperclass( objectClass(), subclass() );

EXPECT_TRUE( superclassOf( subclass() )
==
objectClass()
) << "Superclass should have changed back";
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldChangeTopSuperclassToClass
) {
Oop result = behaviorPrimitives::setSuperclass( classClass(), topClass()->klass() );

checkNotMarkedSymbol( result );
EXPECT_TRUE ( topClass()
->
klass()
== result ) << "Should return receiver";
EXPECT_TRUE( superclassOf( topClass()->klass() )
==
classClass()
) << "Superclass should have changed";
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldNotChangeSuperclassToNilWhenSuperclasHasIvars
) {
Oop result = behaviorPrimitives::setSuperclass( nilObject, delaySubclass() );

checkMarkedSymbol( "Should report error", result,
vmSymbols::argument_is_invalid()
);
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldReportErrorWhenReceiverNotAClass
) {
Oop result = behaviorPrimitives::setSuperclass( superclass(), oopFactory::new_objArray( std::int32_t{0} ) );

checkMarkedSymbol( "Should report error", result,
vmSymbols::receiver_has_wrong_type()
);
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldReportErrorWhenNewSuperclassNotAClass
) {
Oop result = behaviorPrimitives::setSuperclass( oopFactory::new_objArray( std::int32_t{0} ), subclass() );

checkMarkedSymbol( "Should report error", result,
vmSymbols::first_argument_has_wrong_type()
);
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldReportErrorWhenNewSuperclassHasDifferentSize
) {
Oop result = behaviorPrimitives::setSuperclass( classClass(), subclass() );

checkMarkedSymbol( "Should report error", result,
vmSymbols::invalid_klass()
);
}


TEST_F( BehaviorPrimitivesSuperclassTests, setSuperclassShouldReportErrorWhenInstanceVariableNamesAreDifferent
) {
subclass()->klass_part()->
set_superKlass ( classClass() );
Oop result = behaviorPrimitives::setSuperclass( metaclassClass(), subclass() );

checkMarkedSymbol( "Should report error", result,
vmSymbols::invalid_klass()
);
}
