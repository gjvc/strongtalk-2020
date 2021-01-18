//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/memory/Handle.hpp"

#include <cstddef>
#include <gtest/gtest.h>


class CompilerTests : public ::testing::Test {

protected:

    HeapResourceMark *rm;
    int count;
    NativeMethod *seed;

    void SetUp() override;
    void TearDown() override;

    NativeMethod *alloc_nativeMethod( LookupKey *key, int size );
    void initializeSmalltalkEnvironment();

    void exhaustMethodHeap( LookupKey &key, int requiredSize );
    NativeMethod *compile( const char *className, const char *selectorName );
    NativeMethod *compile( Handle &klassHandle, Handle &selectorHandle );

    void clearICs( const char *className, const char *selectorName );
    void clearICs( Handle &klassHandle, Handle &selectorHandle );
    NativeMethod *lookup( const char *className, const char *selectorName );
    void call( const char *className, const char *selectorName );
    static void resetInvocationCounter( MethodOop method );

};
