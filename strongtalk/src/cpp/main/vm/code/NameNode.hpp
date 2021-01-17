//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"

//
// the "NameNode" hierarchy is parallel to the "NameDescriptor" hierarchy
//
// NameNode
//  - LocationName
//  - ValueName
//  - MemoizedName
//  - BlockValueName
//  - IllegalName
//

class ScopeDescriptorRecorder;

class NameNode : public ResourceObject {        // abstract superclass of all NameNodes

    public:
        bool_t genHeaderByte( ScopeDescriptorRecorder * rec, uint8_t code, bool_t is_last, int index );


        virtual bool_t hasLocation() {
            return false;
        }


        virtual bool_t isIllegal() {
            return false;
        }


        virtual Location location() {
            ShouldNotCallThis();
            return unAllocated;
        }


        virtual void generate( ScopeDescriptorRecorder * rec, bool_t is_last ) = 0;
};


// a LocationName describes a location; i.e., the corresponding source name (e.g., method temporary)
// lives in this location for its entire lifetime
class LocationName : public NameNode {
    private:
        Location _location;

        void generate( ScopeDescriptorRecorder * rec, bool_t is_last );

    public:
        LocationName( Location location ) {
            _location = location;
        }


        bool_t hasLocation() {
            return true;
        }


        Location location() {
            return _location;
        }
};


// a ValueName is a constant; i.e., the corresponding source name is a compile-time constant
// (maybe because it is a source constant, or because its computation has been constant-folded)
class ValueName : public NameNode {
    private:
        Oop _value;

        void generate( ScopeDescriptorRecorder * rec, bool_t is_last );

    public:
        ValueName( Oop val ) {
            _value = val;
            st_assert( not val->is_block(), "should use BlockValueName" );
        }
};


// a BlockValueName describes a block closure that has been completely optimized away; i.e., no
// closure will ever be created at runtime during normal execution of the program
class BlockValueName : public NameNode {
    private:
        MethodOop _blockMethod;   // The block method
        ScopeInfo _parentScope; // The scope where to find the context

        void generate( ScopeDescriptorRecorder * rec, bool_t is_last );

    public:
        BlockValueName( MethodOop block_method, ScopeInfo parent_scope ) {
            _blockMethod = block_method;
            _parentScope = parent_scope;
        }
};


// a MemoizedName describes a block closure that has been partially optimized away; i.e., a
// closure may or may not be created at runtime.
//
// The closure's location is initialized to a special
// value, and the actual closure is created on demand after testing if it has been created already.
// If we ever look at this name during debugging and the block doesn't exist yet, we have to create
// one and store it in the location.

class MemoizedName : public NameNode {
    private:
        Location  _location;
        MethodOop _blockMethod;
        ScopeInfo _parentScope;

        void generate( ScopeDescriptorRecorder * rec, bool_t is_last );

    public:
        MemoizedName( Location loc, MethodOop block_method, ScopeInfo parent_scope ) {
            _location    = loc;
            _blockMethod = block_method;
            _parentScope = parent_scope;
        }


        bool_t hasLocation() {
            return true;
        }


        Location location() {
            return _location;
        }
};


// newValueName creates a ValueName or BlockValueName (if value is a block)
NameNode * newValueName( Oop value );


// an IllegalName marks a name that cannot be inspected at runtime because it is never visible
// at any interrupt point (i.e., it is live only between two interrupt points)
// mainly exists for compiler/runtime system debugging
class IllegalName : public NameNode {

    private:
        bool_t isIllegal() {
            return true;
        }


        void generate( ScopeDescriptorRecorder * rec, bool_t is_last );
};
