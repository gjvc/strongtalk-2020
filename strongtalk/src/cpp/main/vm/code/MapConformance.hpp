//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



// Compiler/New Backend/Value Conformance
//
// Used by the new backed to make mappings conform before merging code pieces.
//
// Usage:
//   MapConformance mp(1);
//   mp.append_mapping(...);
//   mp.generate();
//

// Variable is an abstraction describing a register or stack location
// Format as requested by Robert 9/6/96
// 30 bits signed offset
// 2  bits describes type

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/code/MapConformance.hpp"
#include "vm/runtime/ResourceObject.hpp"


class Variable : ValueObject {

private:
    std::int32_t _value;

public:
    enum {
        special_type = 0, //
        reg_type     = 1, //
        stack_type   = 2, //
    };


    std::int32_t type() const {
        return _value & 0x3;
    }


    std::int32_t offset() const {
        return _value >> 2;
    }


    std::int32_t value() const {
        return _value;
    }


    static Variable new_variable( std::int32_t type, std::int32_t offset ) {
        Variable result;
        result._value = ( offset << 2 ) | type;
        return result;
    }


    Variable() {
        _value = 0;
    }


    // Generators
    static Variable new_register( std::int32_t offset );


    static Variable new_stack( std::int32_t offset );


    static Variable unused();


    static Variable top_of_stack();


    // Testing
    bool in_register() const;


    bool on_stack() const {
        return type() == stack_type;
    }


    bool is_unused() const {
        return type() == special_type and offset() == 0;
    }


    bool is_top_of_stack() const {
        return type() == special_type and offset() == 1;
    }


    // Accessors
    std::int32_t register_number() const {
        return offset();
    }


    std::int32_t stack_offset() const {
        return offset();
    }


    void set_unused() {
        _value = 0;
    }


    // Prints the variable.
    void print();


    // Comparison
    bool operator==( const Variable &rhs ) {
        return rhs.value() == value();
    }


    bool operator!=( const Variable &rhs ) {
        return rhs.value() != value();
    }

};


class MappingEntry : public ValueObject {
private:
    Variable _reg;
    Variable _stack;
public:
    MappingEntry( Variable reg, Variable stack ) {
        _reg   = reg;
        _stack = stack;
    }


    Variable reg() const {
        return _reg;
    }


    Variable stack() const {
        return _stack;
    }


    bool has_reg() const {
        return not reg().is_unused();
    }


    bool has_stack() const {
        return not stack().is_unused();
    }


    void set_reg( Variable r ) {
        _reg = r;
    }


    void set_stack( Variable s ) {
        _stack = s;
    }


    void print();
};


class MapConformance;

class MappingTask : public ResourceObject {

private:
    MappingTask *_next;         // next task with same source
    MappingTask *_parent;       // parent chain for recursion
    bool        _is_processed;
    const char  *_what_happened; // what happened to this task
    bool        _uses_top_of_stack;
    Variable    _variable_to_free;

public:
    MappingTask( Variable src_register, Variable src_stack, Variable dst_register, Variable dst_stack ) :
            src( src_register, src_stack ), dst( dst_register, dst_stack ) {
        _next              = nullptr;
        _is_processed      = false;
        _what_happened     = "Nothing";
        _parent            = nullptr;
        _uses_top_of_stack = false;
        _variable_to_free  = Variable::unused();
    }


    bool is_processed() const {
        return _is_processed;
    }


    void set_processed( const char *reason ) {
        _is_processed  = true;
        _what_happened = reason;
    }


    void append( MappingTask *son ) {
        st_assert( not is_processed(), "should be un touched" );
        st_assert( not son->is_processed(), "should be un touched" );
        son->set_next( next() );
        set_next( son );
        son->set_processed( "Linked" );
    }


    MappingTask *next() const {
        return _next;
    }


    void set_next( MappingTask *n ) {
        _next = n;
    }


    MappingTask *parent() const {
        return _parent;
    }


    void set_parent( MappingTask *p ) {
        _parent = p;
    }


    Variable variable_to_free() const {
        return _variable_to_free;
    }


    void set_variable_to_free( Variable var ) {
        _variable_to_free = var;
    }


    bool uses_top_of_stack() const {
        return _uses_top_of_stack;
    }


    void set_uses_top_of_stack( bool b ) {
        _uses_top_of_stack = b;
    }


    MappingEntry src;
    MappingEntry dst;

    void process_task( MapConformance *mc, MappingTask *p );

    void generate_code( MapConformance *mc );

    bool target_includes( Variable var ) const;

    bool is_dependent( MapConformance *mc, MappingTask *task ) const;

    bool in_parent_chain( MappingTask *task ) const;

    std::int32_t number_of_targets() const;

    void print( std::int32_t index );
};

class MapConformance : public ResourceObject {
private:
    Variable                     _free_register;
    GrowableArray<MappingTask *> *_mappings;
    Variable                     *_usedVariables;
    std::int32_t                 _numberOfUsedVariables;

    bool reduce_noop_task( MappingTask *task );

    void simplify();

    void process_tasks();

    Variable pop_temporary();

    void push_temporary( Variable var );

    void push( Variable src, std::int32_t n );

    friend class MappingTask;

public:
    MapConformance();

    // Appends mapping
    void append_mapping( Variable src_register, Variable src_stack, Variable dst_register, Variable dst_stack );

    // Generates the move operations. free_register is a register that can be used during code generation.
    void generate( Variable free_register1, Variable free_register2 );

    // Callback for the generated move operations. Default behavior will print a line per move.
    virtual void move( Variable src, Variable dst );

    virtual void push( Variable src );

    virtual void pop( Variable dst );

    // Print the status of the conformance
    void print();
};
