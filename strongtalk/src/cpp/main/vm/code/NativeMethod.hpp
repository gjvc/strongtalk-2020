//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/NativeCode.hpp"
#include "vm/code/NativeMethodScopes.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/code/Zone.hpp"



// nativeMethods (native methods) are the compiled code versions of Delta methods.

struct NativeMethodFlags {

    std::uint32_t version: 8;                 // version number (0 = first version)
    std::uint32_t level: 4;                   // optimization level
    std::uint32_t age: 4;                     // age (in # of sweep steps)

    std::uint32_t state: 2;                   // {alive, zombie, dead)

    std::uint32_t isUncommonRecompiled: 1;    // recompiled because of uncommon trap?
    std::uint32_t isYoung: 1;                 // "young"? (recently recompiled)
    std::uint32_t isToBeRecompiled: 1;        // to be recompiled as soon as it matures
    std::uint32_t isBlock: 1;                // tells whether is is a block NativeMethod;

    std::uint32_t markedForDeoptimization: 1; // Used for stack deoptimization

    void clear();
};


// A NativeMethod has five parts:
//  1) header
//  2) machine instructions	(mostly in NCodeBase)
//  3) Oop location information	(in OopNativeCode)
//  4) mapping from block_closure_index to NonInlinedBlockScope offset
//  5) debugging information
//  6) dependency information

class ProgramCounterDescriptor;

class PrimitiveInlineCache;

class Compiler;

class nativeMethod_patch;


class NativeMethod : public OopNativeCode {

protected:
    std::uint16_t     _specialHandlerCallOffset;   // offset (in bytes) of call to special handler (*) (see comment below)
    std::uint16_t     _entryPointOffset;           // offset (in bytes) of entry point with class check
    std::uint16_t     _verifiedEntryPointOffset;   // offset (in bytes) of entry point without class check
    std::uint16_t     _scopeLen;                   //
    std::uint16_t     _numberOfNoninlinedBlocks;   //
    std::uint16_t     _numberOfLinks;              // # of inline caches (including PICs) calling this NativeMethod
    std::uint16_t     _numberOfFloatTemporaries;   // # of floats in activation frame of this NativeMethod
    std::uint16_t     _floatSectionSize;           // size of float section in words
    std::uint16_t     _floatSectionStartOffset;    // offset of float section relative to frame pointer (in oops)
    int               _invocationCount;            // incremented for each NativeMethod invocation if CountExecution == true
    int               _uncommonTrapCounter;        // # of times uncommon traps have been executed
    static int        _allUncommonTrapCounter;     // # of times uncommon traps have been executed across all nativeMethods
    NativeMethodFlags _nativeMethodFlags;          // various flags to keep track of NativeMethod state

    // (*) At this address there's 5 bytes of extra Space reserved to accommodate for a call to the zombie handler if the NativeMethod is a zombie.
    // If the NativeMethod is not a zombie, there's a call to StubRoutines::recompile_stub_entry() instead (even if the NativeMethod doesn't count invocations).
    // If the method turns zombie, this call is overwritten by the beforementioned call, thereby making sure that the relocation information for the modified NativeMethod stays valid.
    // In both cases this address is jumped to via a std::int16_t jump from one of the entry points.

    // Note: If it becomes important to save some space per NativeMethod, note that both _zombie_handler_jump_offset
    //       and _verified_entry_point_offset are less than 128 bytes away from _entry_point_offset, thus these
    //       offsets could be stored in one byte relative to _entry_point_offset.

    enum {
        alive       = 0,    //
        zombie      = 1,    //
        dead        = 2,    //
        resurrected = 3     //
    };


public:

    char *instructionsStart() const {
        return (char *) ( this + 1 );
    } // machine instructions

    const char *specialHandlerCall() const {
        return instructionsStart() + _specialHandlerCallOffset;
    } // call to special handler

    const char *entryPoint() const {
        return instructionsStart() + _entryPointOffset;
    } // normal entry point

    char *verifiedEntryPoint() const {
        return instructionsStart() + _verifiedEntryPointOffset;
    } // entry point if klass is correct

    bool_t isFree() {
        return Universe::code->contains( (void *) _instructionsLength );
    } // has this NativeMethod been freed


    // debugging information
    NativeMethodScopes *scopes() const {
        return (NativeMethodScopes *) locsEnd();
    }


    ProgramCounterDescriptor *pcs() const {
        return (ProgramCounterDescriptor *) scopes()->pcs();
    }


    ProgramCounterDescriptor *pcsEnd() const {
        return (ProgramCounterDescriptor *) scopes()->pcsEnd();
    }


    MethodOop method() const;

    KlassOop receiver_klass() const;


    std::uint16_t *noninlined_block_offsets() const {
        return (std::uint16_t *) pcsEnd();
    }


    const char *end() const {
        return ( (const char *) ( noninlined_block_offsets() + _numberOfNoninlinedBlocks ) );
    }


public:
    LookupKey   _lookupKey;     // key for code table searching
    JumpTableID _mainId;        //
    JumpTableID _promotedId;    //


    int number_of_float_temporaries() const {
        return _numberOfFloatTemporaries;
    }


    int float_section_size() const {
        return _floatSectionSize;
    }


    int float_section_start_offset() const {
        return _floatSectionStartOffset;
    }


    // Interface for number_of_callers
    int number_of_links() const {
        return _numberOfLinks;
    }


    void inc_number_of_links() {
        _numberOfLinks++;
    }


    void dec_number_of_links() {
        _numberOfLinks--;
    }


    // Interface for uncommon_trap_invocation
    int uncommon_trap_counter() const {
        return _uncommonTrapCounter;
    }


    void inc_uncommon_trap_counter() {
        _uncommonTrapCounter++;
        _allUncommonTrapCounter++;
    }


    // Returns the parent NativeMethod if present, or nullptr otherwise
    NativeMethod *parent();

    // Returns the outermost NativeMethod, or "this" if no parent is present.
    NativeMethod *outermost();

protected:
    NativeMethod( Compiler *c );

    void *operator new( std::size_t size );

public:
    friend NativeMethod *new_nativeMethod( Compiler *c );

    void initForTesting( int size, LookupKey *key ); // to support testing

    int size() const {
        return end() - ( (const char *) this );
    }    // total size of NativeMethod

    int codeSize() const {
        return instructionsEnd() - instructionsStart();
    }    // size of code in bytes

    // Shift the pc relative information by delta.
    // Call this whenever the code is moved.
    void fix_relocation_at_move( int delta );

    void moveTo( void *to, int size );

public:
    bool_t isNativeMethod() const {
        return true;
    }


    // Flag accessing and manipulation.
    bool_t isAlive() const {
        return _nativeMethodFlags.state == alive;
    }


    bool_t isZombie() const {
        return _nativeMethodFlags.state == zombie;
    }


    bool_t isDead() const {
        return _nativeMethodFlags.state == dead;
    }


    bool_t isResurrected() {
        return _nativeMethodFlags.state == resurrected;
    }


    void makeZombie( bool_t clearInlineCaches );


    // allow resurrection of zombies - for use during recompilation
    void resurrect() {
        st_assert( isZombie(), "cannot resurrect non-zombies" );
        _nativeMethodFlags.state = resurrected;
    }


    bool_t isUncommonRecompiled() const {
        return _nativeMethodFlags.isUncommonRecompiled;
    }


    bool_t isYoung();


    void makeYoung() {
        _nativeMethodFlags.isYoung = 1;
    }


    void makeOld();


    int age() const {
        return _nativeMethodFlags.age;
    }


    void incrementAge() {
        const int MaxAge = 15;
        _nativeMethodFlags.age = min( _nativeMethodFlags.age + 1, MaxAge );
    }


    bool_t is_marked_for_deoptimization() const {
        return _nativeMethodFlags.markedForDeoptimization;
    }


    void mark_for_deoptimization() {
        _nativeMethodFlags.markedForDeoptimization = 1;
    }


    void unmark_for_deoptimization() {
        _nativeMethodFlags.markedForDeoptimization = 0;
    }


    bool_t isToBeRecompiled() const {
        return _nativeMethodFlags.isToBeRecompiled;
    }


    void makeToBeRecompiled() {
        _nativeMethodFlags.isToBeRecompiled = 1;
    }


    bool_t is_block() const {
        return _nativeMethodFlags.isBlock == 1;
    }


    bool_t is_method() const {
        return _nativeMethodFlags.isBlock == 0;
    }


    int level() const;


    void setLevel( int newLevel ) {
        _nativeMethodFlags.level = newLevel;
    }


    int version() const {
        return _nativeMethodFlags.version;
    }


    void setVersion( int v );

public:
    int estimatedInvocationCount() const;   // approximation (not all calls have counters)
    int ncallers() const;                   // # of callers (# nativeMethods, *not* # of inline caches)

    bool_t encompasses( const void *p ) const;


    // for zone LRU management
    int lastUsed() const {
        return LRUtable[ 0 ].lastUsed;
    }


    void clear_inline_caches();

    void cleanup_inline_caches();

    void forwardLinkedSends( NativeMethod *to );

    void unlink(); // unlink from codeTable, deps etc.

    // Removes the NativeMethod from the zone
    void flush();

    // Returns the set of nativeMethods to invalidate if THIS NativeMethod is the root of invalidation.
    // The set will be this NativeMethod plus all offspring (seperately compiled block code).
    GrowableArray<NativeMethod *> *invalidation_family();

    // Tells whether this NativeMethod dependes on invalid classes (classes flagged invalid)
    bool_t depends_on_invalid_klass();

private:
    // Recursive helper function for invalidation_family()
    void add_family( GrowableArray<NativeMethod *> *result );

protected:
    void check_store();

public:
    // Iterate over all oops in the NativeMethod
    void oops_do( void f( Oop * ) );

    bool_t switch_pointers( Oop from, Oop to, GrowableArray<NativeMethod *> *nativeMethods_to_invalidate );

    void relocate();

    // Verify operations
    void verify();

    void verify_expression_stacks();            // verify the expression stacks at all interrupt points
    void verify_expression_stacks_at( const char *pc ); // verify the expression stacks at pc

    // Iterates over all inline caches in the NativeMethod
    void CompiledICs_do( void f( CompiledInlineCache * ) );

    // Iterates over all primitive inline caches in the NativeMethod
    void PrimitiveICs_do( void f( PrimitiveInlineCache * ) );

    // programming and debugging
    ProgramCounterDescriptor *containingProgramCounterDescriptor( const char *pc, ProgramCounterDescriptor *start = nullptr ) const;

protected:

    ProgramCounterDescriptor *containingProgramCounterDescriptorOrNULL( const char *pc, ProgramCounterDescriptor *start = nullptr ) const;

public:

    ScopeDescriptor *containingScopeDesc( const char *pc ) const;

    ProgramCounterDescriptor *correspondingPC( ScopeDescriptor *sd, int byteCodeIndex ) const;

    // For debugging
    CompiledInlineCache *IC_at( const char *p ) const;

    PrimitiveInlineCache *primitiveIC_at( const char *p ) const;

    Oop *embeddedOop_at( const char *p ) const;

    JumpTableEntry *jump_table_entry() const;

    // noninlined block mapping
    bool_t has_noninlined_blocks() const;

    int number_of_noninlined_blocks() const;

protected:
    void validate_noninlined_block_scope_index( int index ) const;

public:
    NonInlinedBlockScopeDescriptor *noninlined_block_scope_at( int noninlined_block_index ) const;

    MethodOop noninlined_block_method_at( int noninlined_block_index ) const;

    void noninlined_block_at_put( int noninlined_block_index, int offset ) const;

    JumpTableEntry *noninlined_block_jumpEntry_at( int noninlined_block_index ) const;


    // Returns true if activation frame has been established.
    bool_t has_frame_at( const char *pc ) const;

    // Returns true if pc is not in prologue or epilogue code.
    bool_t in_delta_code_at( const char *pc ) const;

    // Printing support
    void print();

    void printCode();

    void printLocs();

    void printPcs();

    void print_value_on( ConsoleOutputStream *stream );

    // Inlining Database
    void print_inlining_database();

    void print_inlining_database_on( ConsoleOutputStream *stream );

    GrowableArray<ProgramCounterDescriptor *> *uncommonBranchList();

    // prints the inlining structure (one line per scope with indentation if there's no debug info)
    void print_inlining( ConsoleOutputStream *stream = nullptr, bool_t with_debug_info = false );

    friend NativeMethod *nativeMethodContaining( const char *pc, char *likelyEntryPoint );

    friend NativeMethod *findNativeMethod( const void *start );

    friend NativeMethod *findNativeMethod_maybe( void *start );

    friend NativeMethod *nativeMethod_from_insts( const char *insts );

public:
    // Counting & Timing
    int invocation_count() const {
        return _invocationCount;
    }


    void set_invocation_count( int c ) {
        _invocationCount = c;
    }


    // Perform a sweeper task
    void sweeper_step( double decay_factor );

private:
    inline void decay_invocation_count( double decay_factor );

public:
    static int invocationCountOffset() {
        return (int) &( (NativeMethod *) 0 )->_invocationCount;
    }

    // Support for preemption:

    // Overwrites the NativeMethod allowing trap at interrupt point.
    // After the overwrite data contains the information for restoring the NativeMethod.
    void overwrite_for_trapping( nativeMethod_patch *data );

    // Restores the NativeMethod
    void restore_from_patch( nativeMethod_patch *data );
};


NativeMethod *findNativeMethod( const void *start );

NativeMethod *new_nativeMethod( Compiler *c );


inline NativeMethod *nativeMethod_from_insts( const char *insts ) {    // for efficiency
    NativeMethod *nm = (NativeMethod *) insts - 1;
    return findNativeMethod( nm );
}
