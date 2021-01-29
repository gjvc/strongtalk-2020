//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"


//
// A blockClosureOop is the Oop of a block closure, i.e., the object created when a block literal like [3 + 4] is evaluated.
//
// Blocks can be...
// "clean" (have no free variables, like [3 + 4]), in which case their _lexical_scope pointer contains nil or self
// or they can be full blocks (like [x + 4]) in which case their _lexical_scope is used to resolve accesses to free variables.
//

class JumpTableEntry;

class BlockClosureOopDescriptor : public MemOopDescriptor {

private:
    Oop        _methodOrJumpAddr;       // block method (if interpreted), JumpTable stub address (if compiled)
    ContextOop _lexical_scope;          // lexical context or nil (if no free variables)

    BlockClosureOop addr() const {
        return BlockClosureOop( MemOopDescriptor::addr() );
    }


public:

    static std::int32_t method_or_entry_byte_offset() {
        return ( 2 * OOP_SIZE ) - MEMOOP_TAG;
    }


    static std::int32_t context_byte_offset() {
        return ( 3 * OOP_SIZE ) - MEMOOP_TAG;
    }


    friend BlockClosureOop as_blockClosureOop( void *p );

    static BlockClosureOop create_clean_block( std::int32_t nofArgs, const char *entry_point );    // create a clean block

    inline bool isCompiledBlock() const {
        return not Oop( addr()->_methodOrJumpAddr )->is_mem();
    }


    void set_method( MethodOop m ) {
        STORE_OOP( &addr()->_methodOrJumpAddr, m );
    }


    void set_jumpAddr( const void *jmp_addr ) {
        st_assert( not Oop( jmp_addr )->is_mem(), "not properly aligned" );
        addr()->_methodOrJumpAddr = (Oop) jmp_addr;
    }


    MethodOop method() const;

    JumpTableEntry *jump_table_entry() const;

    // returns the number of arguments for the method Oop belonging to this closure
    std::int32_t number_of_arguments();


    // sizing
    static std::int32_t header_size() {
        return sizeof( BlockClosureOopDescriptor ) / OOP_SIZE;
    }


    static std::int32_t object_size() {
        return header_size();
    }


    void set_lexical_scope( ContextOop l ) {
        STORE_OOP( &addr()->_lexical_scope, l );
    }


    ContextOop lexical_scope() const {
        return addr()->_lexical_scope;
    }


    bool is_pure() const;

    // deoptimization
    void deoptimize();


    const char *name() const {
        return "blockClosure";
    }


    void verify();

    friend class BlockClosureKlass;
};


inline BlockClosureOop as_blockClosureOop( void *p ) {
    return BlockClosureOop( as_memOop( p ) );
}
