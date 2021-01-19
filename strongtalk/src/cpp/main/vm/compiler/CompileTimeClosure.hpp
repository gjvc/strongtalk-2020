//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/runtime/ResourceObject.hpp"



// A CompileTimeClosure represents a block closure at compile time. It holds the noninlined
// block method and a unique identification of the compiled & customized home method
// (the non-block method lexically enclosing the block). CompileTimeClosures are needed to
// distinguish multiple versions of the same source block at compile time.  Multiple
// versions can exist when the block's home method is inlined several times.
//
// Note: Having a closure stub doesn't mean that there exists a blockClosureOop at
// run-time.  If the block is inlined, none of this ever matters.  However, if a
// block belonging to a compiled method can potentially be created at run-time, it
// must be different from other versions of the same method (i.e., it must be customized to
// the version of its home method).  This customization is achieved by putting different
// code addresses in the blockClosureOops so they invoke different nativeMethods.

class NonInlinedBlockScopeNode;

class PseudoRegister;

class InlinedScope;


class CompileTimeClosure : public PrintableResourceObject {

protected:
    InlinedScope *_parent_scope;           // scope to which the closure belongs
    MethodOop _method;                   // block method
    PseudoRegister *_context;                // parent context
    int         _nofArgs;                  // number of arguments for the block
    JumpTableID _id;                       // unique identification of this closure within the parent NativeMethod.
    NonInlinedBlockScopeNode *_noninlined_block_scope; // an NonInlinedScopeDesc

public:
    CompileTimeClosure( InlinedScope *s, MethodOop method, PseudoRegister *context, int nofArgs ) {
        _parent_scope           = s;
        _method                 = method;
        _context                = context;
        _nofArgs                = nofArgs;
        _noninlined_block_scope = nullptr;
    }


    InlinedScope *parent_scope() const {
        return _parent_scope;
    }


    MethodOop method() const {
        return _method;
    }


    PseudoRegister *context() const {
        return _context;
    }


    int nofArgs() const {
        return _nofArgs;
    }


    JumpTableID id() const {
        return _id;
    }


    NonInlinedBlockScopeNode *noninlined_block_scope();

    void generateDebugInfo();


    void set_id( JumpTableID id ) {
        _id = id;
    }    // sets the indices for computing the jump table entry

    const char *jump_table_entry();            // returns the code entry point for the jump table entry

    void print();

    virtual bool_t verify() const;
};
