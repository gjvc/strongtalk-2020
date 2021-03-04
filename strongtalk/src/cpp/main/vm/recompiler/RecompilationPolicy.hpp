
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"
#include "vm/oop/BlockClosureOopDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/ResourceObject.hpp"

// The RecompilationPolicy selects which method (if any) should be recompiled.

class Recompilee;

class RecompilationPolicy : public ResourceObject {

protected:
    GrowableArray<RecompilerFrame *> *_stack;
    const char                       *_msg;                     // for (performance) debugging: reason for not going up, etc.

    RecompilerFrame *senderOf( RecompilerFrame *rf );           // return rf->sender() and update stack if necessary
    RecompilerFrame *parentOfBlock( BlockClosureOop blk );      // block's parent frame (or nullptr)
    RecompilerFrame *parentOf( RecompilerFrame *rf );           // same for rf->parent()
    RecompilerFrame *senderOrParentOf( RecompilerFrame *rf );   // either sender or parent, depending on various factors
    RecompilerFrame *findTopInlinableFrame();

    void checkCurrent( RecompilerFrame *&current, RecompilerFrame *&prev, RecompilerFrame *&prevMethod );

    void fixBlockParent( RecompilerFrame *rf );

    void printStack();

public:
    RecompilationPolicy( RecompilerFrame *first );
    RecompilationPolicy() = default;
    virtual ~RecompilationPolicy() = default;
    RecompilationPolicy( const RecompilationPolicy & ) = default;
    RecompilationPolicy &operator=( const RecompilationPolicy & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    Recompilee *findRecompilee();       // determine what to recompile
    void cleanupStaleInlineCaches();    // clean up inline caches of top methods

    static bool needRecompileCounter( Compiler *c );                      // does this compilation (NativeMethod) need an invocation counter?
    static bool shouldRecompileAfterUncommonTrap( NativeMethod *nm );     // NativeMethod encountered an uncommon case; should it be recompiled?
    static bool shouldRecompileUncommonNativeMethod( NativeMethod *nm );  // NativeMethod is in uncommon mode; ok to recompile and reoptimize it?
    static const char *shouldNotRecompileNativeMethod( NativeMethod *nm );              // is NativeMethod fit to be recompiled?  return nullptr if yes, reason otherwise
    static std::int32_t uncommonNativeMethodInvocationLimit( std::int32_t version );    // return invocation counter limit for an uncommon NativeMethod
    static std::int32_t uncommonNativeMethodAgeLimit( std::int32_t version );           // return NativeMethod age limit for an uncommon NativeMethod

};
