//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/AssociationOopDescriptor.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"

#include "test/compiler/CompilerTests.hpp"
#include "test/runtime/testProcess.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"

#include <gtest/gtest.h>


// forces deoptimization, resulting in construction of canonical_context where the not all of the vframes are contained within a stack frame.
// Was causing MNU due to nils in the outer contexts, since the code was not passing the outer VirtualFrame when crossing stack boundaries.


void CompilerTests::SetUp() {
    rm    = new HeapResourceMark();
    count = 0;
}


void CompilerTests::TearDown() {
    count = 0;
    Universe::code->_methodHeap->_combineOnDeallocation = true;
    Universe::code->flush(); /* free all nativeMethods*/ Universe::code->compact();
    Universe::methodOops_do( &CompilerTests::resetInvocationCounter );
    delete rm;
    rm = nullptr;
}


NativeMethod *CompilerTests::alloc_nativeMethod( LookupKey *key, std::int32_t size ) {
    ZoneHeap     *heap = Universe::code->_methodHeap;
    NativeMethod *nm   = nullptr;
    nm = (NativeMethod *) heap->allocate( size );
    if ( !nm )
        return nullptr;
    *( (void **) nm ) = *( (void **) seed ); // ugly hack to copy the vftable
    nm->initForTesting( size - sizeof( NativeMethod ), key );
    nm->makeZombie( false );
    return nm;
}


void CompilerTests::initializeSmalltalkEnvironment() {
    HandleMark mark;
    Handle     _new( OopFactory::new_symbol( "new" ) );
    Handle     initialize( OopFactory::new_symbol( "initialize" ) );
    Handle     processorScheduler( Universe::find_global( "ProcessorScheduler" ) );
    Handle     run( OopFactory::new_symbol( "run" ) );
    Handle     systemInitializer( Universe::find_global( "SystemInitializer" ) );

    Handle         processor( Delta::call( processorScheduler.as_oop(), _new.as_oop() ) );
    AssociationOop processorAssoc = Universe::find_global_association( "Processor" );
    processorAssoc->set_value( processor.as_oop() );

    Delta::call( processor.as_oop(), initialize.as_oop() );
    Delta::call( systemInitializer.as_oop(), run.as_oop() );
}


void CompilerTests::exhaustMethodHeap( LookupKey &key, std::int32_t requiredSize ) {

    GrowableArray<NativeMethod *> *nativeMethods = new GrowableArray<NativeMethod *>;

//    std::int32_t blockSize = Universe::code->_methodHeap->blockSize;
    std::int32_t size = Universe::code->_methodHeap->freeBytes();

    bool hasFailed = false;
    while ( !hasFailed ) {
        NativeMethod *newnm = alloc_nativeMethod( &key, size );
        if ( newnm ) {
            nativeMethods->append( newnm );
        } else {
            if ( size == requiredSize ) {
                hasFailed = true;
            }

            size /= 2;
            if ( size < requiredSize ) {
                size = requiredSize;
            }
        }
    }
}


NativeMethod *CompilerTests::compile( const char *className, const char *selectorName ) {
    HandleMark mark;
    Handle     toCompile( OopFactory::new_symbol( selectorName ) );
    Handle     varClass( Universe::find_global( className ) );
    return compile( varClass, toCompile );
}


NativeMethod *CompilerTests::compile( Handle &klassHandle, Handle &selectorHandle ) {
    KlassOop  klass    = klassHandle.as_klass();
    SymbolOop selector = SymbolOop( selectorHandle.as_oop() );

    LookupResult result = interpreter_normal_lookup( klass, selector );
    LookupKey    key( klass, selector );

    VM_OptimizeMethod op( &key, result.method() );
    VMProcess::execute( &op );
    DeltaCallCache::clearAll();
    LookupCache::flush();

    return op.result();
}


void CompilerTests::clearICs( const char *className, const char *selectorName ) {
    HandleMark mark;
    Handle     varClass( Universe::find_global( className ) );
    Handle     setup( OopFactory::new_symbol( selectorName ) );
    clearICs( varClass, setup );
}


void CompilerTests::clearICs( Handle &klassHandle, Handle &selectorHandle ) {
    KlassOop     klass    = klassHandle.as_klass();
    SymbolOop    selector = SymbolOop( selectorHandle.as_oop() );
    LookupResult result   = interpreter_normal_lookup( klass, selector );

    result.method()->cleanup_inline_caches();
}


NativeMethod *CompilerTests::lookup( const char *className, const char *selectorName ) {
    HandleMark mark;
    Handle     classHandle( Universe::find_global( className ) );
    Handle     selectorHandle( OopFactory::new_symbol( selectorName ) );

    KlassOop  klass    = classHandle.as_klass();
    SymbolOop selector = SymbolOop( selectorHandle.as_oop() );

    LookupKey key( klass, selector );
    return Universe::code->lookup( &key );
}


void CompilerTests::call( const char *className, const char *selectorName ) {

    HandleMark mark;
    Handle     _new( OopFactory::new_symbol( "new" ) );
    Handle     setup( OopFactory::new_symbol( selectorName ) );
    Handle     testClass( Universe::find_global( className ) );

    Handle newTest( Delta::call( testClass.as_klass(), _new.as_oop() ) );

    Delta::call( newTest.as_oop(), setup.as_oop() );
}


void CompilerTests::resetInvocationCounter( MethodOop method ) {
    method->set_invocation_count( 0 );
}


TEST_F( CompilerTests, compileContentsDo
) {
    call( "ContextNestingTest", "testOnce" );
    compile( "ContextNestingTest", "testWith:" );
    call( "ContextNestingTest", "testTwice" );
}


TEST_F( CompilerTests, uncommonTrap
) {
    AddTestProcess addTest;
    {
        HandleMark mark;
        initializeSmalltalkEnvironment();
        Handle _new( OopFactory::new_symbol( "new" ) );
        Handle setup( OopFactory::new_symbol( "populatePIC" ) );
        Handle toCompile( OopFactory::new_symbol( "type" ) );
        Handle triggerTrap( OopFactory::new_symbol( "testTriggerUncommonTrap" ) );
        Handle testClass( Universe::find_global( "DeltaParameterTest" ) );
        Handle varClass( Universe::find_global( "DeltaParameter" ) );
        Handle newTest( Delta::call( testClass.as_klass(), _new.as_oop() ) );
        call( "DeltaParameterTest", "populatePIC" );
        LookupResult result = interpreter_normal_lookup( varClass.as_klass(), SymbolOop( toCompile.as_oop() ) );
        LookupKey    key( varClass.as_klass(), toCompile.as_oop() );
        ASSERT_TRUE( !result.
            is_empty()
        );
        VM_OptimizeMethod op( &key, result.method() );
        VMProcess::execute( &op );
        DeltaCallCache::clearAll();
        LookupCache::flush();
        std::int32_t trapCount = op.result()->uncommon_trap_counter();
        Delta::call( newTest
                         .
                             as_oop(), triggerTrap
                         .
                             as_oop()
        );
        ASSERT_EQ( trapCount
                       +1, op.result()->uncommon_trap_counter() );
    }
}


TEST_F( CompilerTests, invalidJumptableID
) {
    AddTestProcess addTest;
    {
        initializeSmalltalkEnvironment();
        call( "BlockMaterializeTest", "testIgnoredBlock" );
        compile( "BlockMaterializeTest", "testIgnoredBlock" );
/* was causing assertion failure in CompileTimeClosure::jump_table_entry() due to no _id*/
    }
}


TEST_F( CompilerTests, toplevelBlockScopeOuterContextFilledWithNils
) {
    AddTestProcess addTest;
    {
        initializeSmalltalkEnvironment();
        call( "NonInlinedBlockTest", "testSetup" );
        compile( "NonInlinedBlockTest", "exercise:value:" );
        clearICs( "NonInlinedBlockTest", "testSetup" );
        call( "NonInlinedBlockTest", "testSetup" );
        call( "NonInlinedBlockTest", "testTrap" );
    }
}


TEST_F( CompilerTests, toplevelBlockScopeWithContextContainingBlockReferencingContext
) {
    AddTestProcess addTest;
    {
        initializeSmalltalkEnvironment();
        call( "NonInlinedBlockTest", "testSetup2" );
        compile( "NonInlinedBlockTest", "exercise2:value:" );
        clearICs( "NonInlinedBlockTest", "testSetup2" );
        call( "NonInlinedBlockTest", "testSetup2" );

        call( "NonInlinedBlockTest", "testTrap2" );
    }
}


TEST_F( CompilerTests, recompileZombieForcingFlush
) {
    AddTestProcess addTest;
    {
        HandleMark mark;
        initializeSmalltalkEnvironment();
        Handle setup( OopFactory::new_symbol( "testSetup2" ) );
        Handle varClass( Universe::find_global( "NonInlinedBlockTest" ) );
        Universe::code->
            flush();
        Universe::code->
            compact();
        LookupCache::flush();
        ASSERT_TRUE( lookup( "NonInlinedBlockTest", "exercise2:value:" )
                     == nullptr );
        clearICs( "NonInlinedBlockTest", "testSetup2" );
        call( "NonInlinedBlockTest", "testSetup2" );
        ASSERT_TRUE( lookup( "NonInlinedBlockTest", "exercise2:value:" )
                     == nullptr );
        seed = compile( "NonInlinedBlockTest", "exercise2:value:" );
        clearICs( "NonInlinedBlockTest", "testSetup2" );
        call( "NonInlinedBlockTest", "testSetup2" );
        JumpTableEntry *entry   = seed->noninlined_block_jumpEntry_at( 1 );
        NativeMethod   *blocknm = entry->block_nativeMethod();
        LookupKey      bogus( varClass.as_klass(), setup.as_oop() );
        exhaustMethodHeap( bogus, blocknm
            ->
                size()
        );
        blocknm->
            inc_uncommon_trap_counter();
        blocknm->
            inc_uncommon_trap_counter();
        blocknm->
            inc_uncommon_trap_counter();
        blocknm->
            inc_uncommon_trap_counter();
        blocknm->
            inc_uncommon_trap_counter();
        call( "NonInlinedBlockTest", "testTrap2" );
    }
}


TEST_F( CompilerTests, recompileZombieWhenMethodHeapExhausted
) {
    AddTestProcess addTest;
    {
        initializeSmalltalkEnvironment();
        call( "CompilerTest", "testOnce" );
        compile( "CompilerTest", "with:" );
        clearICs( "CompilerTest", "testOnce" );
        call( "CompilerTest", "testOnce" );
        seed             = lookup( "CompilerTest", "with:" );
        seed->
            inc_uncommon_trap_counter();
        seed->
            inc_uncommon_trap_counter();
        seed->
            inc_uncommon_trap_counter();
        seed->
            inc_uncommon_trap_counter();
        seed->
            inc_uncommon_trap_counter();
        exhaustMethodHeap( seed
                               ->_lookupKey, seed->
            size()
        );
        ASSERT_FALSE( seed
                          ->
                              isZombie()
        ); /* forces deoptimization and recompilation.*/ call( "CompilerTest", "testTwice" );
        ASSERT_TRUE( seed
                         ->
                             isZombie()
        );
        NativeMethod *nm = lookup( "CompilerTest", "with:" );
        ASSERT_FALSE( ( nm
                        == seed ) );
    }
}
