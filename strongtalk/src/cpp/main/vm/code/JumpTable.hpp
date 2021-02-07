//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/memory/util.hpp"
#include "vm/oops/OopDescriptor.hpp"

// The JumpTable constitutes the interface between interpreter code and optimized code.
// This indirection makes it possible to invalidate optimized code without
//  - keeping track of dependencies between sends in methodsOops and optimized code, or
//  - traverse all methodOops.
// If optimized code has become invalid the jump table entry is simply snapped.
// In addition the JumpTable serves as a dispatch table for block closures for optimized code.

// %implementation note:
//   make JumpTable growable

class JumpTableEntry;

class JumpTable;

class NativeMethod;

class JumpTableID : ValueObject {

private:
    std::uint16_t _major;
    std::uint16_t _minor;

    friend class JumpTable;

    static constexpr std::int32_t max_value = nthMask( 16 ); //


public:
    JumpTableID() :
        _major( max_value ), _minor( max_value ) {
    }


    JumpTableID( std::uint16_t major ) :
        _major( major ), _minor( max_value ) {
    }


    JumpTableID( std::uint16_t major, std::uint16_t minor ) :
        _major( major ), _minor( minor ) {
    }


    bool has_minor() const {
        return _minor not_eq max_value;
    }


    bool is_block() const {
        return _minor > 0;
    }


    bool is_valid() const {
        return _major not_eq max_value;
    }


    std::uint16_t major() const {
        return _major;
    }


    std::uint16_t minor() const {
        return _minor;
    }


    JumpTableID sub( std::uint16_t minor ) const {
        return JumpTableID( _major, minor );
    }
};


class JumpTable : public ValueObject {

protected:
    std::int32_t _firstFree;    // index of first free elem
    static const char *allocate_jump_entries( std::int32_t size );

    static JumpTableEntry *jump_entry_for_at( const char *entries, std::int32_t index );

    JumpTableEntry *major_at( std::uint16_t index );

public:
    const char   *_entries;
    std::int32_t length;        // max. number of IDs
    std::int32_t usedIDs;        // # of used ID

public:
    JumpTable();
    virtual ~JumpTable();
    JumpTable( const JumpTable & ) = default;
    JumpTable &operator=( const JumpTable & ) = default;
    void operator delete( void *ptr ) { (void)ptr; }


    void init();

    // Allocates a block of adjacent jump table entries.
    JumpTableID allocate( std::int32_t number_of_entries );

    // returns the jumptable entry for id
    JumpTableEntry *at( JumpTableID id );

    std::int32_t newID();               // return a new ID
    std::int32_t peekID();           // return value which would be returned by newID,
    // but don't actually allocate the ID
    void freeID( std::int32_t index ); // index is unused again

    void verify();

    void print();

    // compilation of blocks
    static const char *compile_new_block( BlockClosureOop blk );       // create NativeMethod, return entry point
    static NativeMethod *compile_block( BlockClosureOop blk );         // (re)compile block NativeMethod

    friend class JumpTableEntry;
};


// implementation note: JumpTableEntry should be an abstract class with two
// subclasses for NativeMethod and block entries, but these classes are combined
// in order to save Space (no vtbl pointer needed)
class JumpTableEntry : public ValueObject {

private:
    char *jump_inst_addr() const {
        st_assert( Oop( this )->is_smi(), "misaligned" );
        return (char *) this;
    }


    char *state_addr() const {
        return ( (char *) this ) + jump_inst_size();
    }


    static std::int32_t jump_inst_size() {
        return 1 + sizeof( const char * );
    } // x86 specific

    char state() const {
        return *state_addr();
    }


    void fill_entry( const char instr, const char *dest, char state );

    void initialize_as_unused( std::int32_t index );

    void initialize_as_link( const char *link );

    void initialize_NativeMethod_stub( char *dest );

    void initialize_block_closure_stub();

    inline JumpTableEntry *previous_stub() const;

    inline JumpTableEntry *next_stub() const;

    JumpTableEntry *parent_entry( std::int32_t &index ) const;

    void report_verify_error( const char *message );

public:
    // testing operations	    LARS: please add comments explaining what the 4 cases are  -Urs 4/96
    bool is_NativeMethod_stub() const;

    bool is_block_closure_stub() const;

    bool is_link() const;

    bool is_unused() const;


    // entry point
    const char *entry_point() const {
        return jump_inst_addr();
    }


    // destination
    const char **destination_addr() const;       // the address of the destination
    const char *destination() const;             // current destination
    void set_destination( const char *dest );    // sets the destination

    // operations for link stubs
    const char *link() const;

    // operations for unused
    std::int32_t next_free() const;

    // operations for NativeMethod stubs (is_NativeMethod_stub() == true)
    NativeMethod *method() const;        // nullptr if not pointing to a method

    // operations for block stubs (is_block_closure_stub() == true)
    bool block_has_nativeMethod() const;

    NativeMethod *block_nativeMethod() const;    // block NativeMethod (or nullptr if not compiled yet)
    MethodOop block_method() const;    // block method

    // NativeMethod creating the blockClosureOops pointing to this entry
    // index is set to the distance |parent_entry - this|
    NativeMethod *parent_nativeMethod( std::int32_t &index ) const;

    // printing
    void print();

    void verify();


    // size of jump table entry
    static std::int32_t size() {
        return (std::int32_t) align( (void *) ( sizeof( char ) + jump_inst_size() ), sizeof( Oop ) );
    }


    friend class JumpTable;
};
