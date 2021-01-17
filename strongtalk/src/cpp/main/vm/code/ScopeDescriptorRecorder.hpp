//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/GrowableArray.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/code/NameNode.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/ProgramCounterDescriptorInfoClass.hpp"
#include "vm/runtime/ResourceObject.hpp"


// ScopeDescriptorRecorder provides the interface to generate ScopeDescriptor instances for optimized methods (nativeMethods).
// To retrieve the generated information, use NativeMethodScopes

class Array;

class ByteArray;

class NonInlinedBlockScopeNode;

class NameNode;

class ResourceObject;

class TopLevelBlockScopeNode;

class LogicalAddress;

class ProgramCounterDescriptorInfoClass;

// Interface to generate scope information for a NativeMethod
class ScopeDescriptorRecorder : public ResourceObject {

    private:
        bool_t _hasCodeBeenGenerated;
        Array                             * _oops;
        Array                             * _values;
        ByteArray                         * _codes;
        ProgramCounterDescriptorInfoClass * _programCounterDescriptorInfo;

        GrowableArray <KlassOop> * _dependents;
        int _dependentsEnd;

        NonInlinedBlockScopeNode * _nonInlinedBlockScopeNode;
        NonInlinedBlockScopeNode * _nonInlinedBlockScopesTail;

    public:
        ScopeInfo _root;

        // Returns the offset for a scopeDesc after generation of the scopeDesc info.
        int offset( ScopeInfo scope );

        int offset_for_noninlined_scope_node( NonInlinedBlockScopeNode * scope );

        ScopeDescriptorRecorder( int scopeSize, // estimated size of scopes (in bytes)
                                 int npcDesc );  // estimated number of ProgramCounterDescriptors

        // Adds a method scope
        ScopeInfo addMethodScope( LookupKey * key,                      // lookup key
                                  MethodOop method,                     // result of the lookup
                                  LogicalAddress * receiver_location,   // location of receiver
                                  bool_t allocates_compiled_context,    // tells whether the code allocates a context
                                  bool_t lite = false,                  //
                                  int scopeID = 0,                      //
                                  ScopeInfo senderScope = nullptr,      //
                                  int senderByteCodeIndex = IllegalByteCodeIndex,
                                  bool_t visible = false );


        // Adds an inlined block scope
        ScopeInfo addBlockScope( MethodOop method,                      // block method
                                 ScopeInfo parent,                      // parent scope
                                 bool_t allocates_compiled_context,     // tells whether the code allocates a context
                                 bool_t lite = false, int scopeID = 0,  //
                                 ScopeInfo senderScope = nullptr,       //
                                 int senderByteCodeIndex = IllegalByteCodeIndex,
                                 bool_t visible = false );

        // Adds a top level block scope
        ScopeInfo addTopLevelBlockScope( MethodOop method,                      // block method
                                         LogicalAddress * receiver_location,    // location of receiver
                                         KlassOop receiver_klass,               // receiver klass
                                         bool_t allocates_compiled_context );   // tells whether the code allocates a context

        // Adds an noninlined block scope
        // Used for retrieving information about block closure stubs
        NonInlinedBlockScopeNode * addNonInlinedBlockScope( MethodOop block_method, ScopeInfo parent );

        // Interface for adding name nodes.
        void addTemporary( ScopeInfo scope, int index, LogicalAddress * location );         // all entries [0..max(index)] must be filled.
        void addContextTemporary( ScopeInfo scope, int index, LogicalAddress * location );  // all entries [0..max(index)] must be filled.
        void addExprStack( ScopeInfo scope, int byteCodeIndex, LogicalAddress * location ); // sparse array = some entries may be left empty.

        LogicalAddress * createLogicalAddress( NameNode * initial_value );

        void changeLogicalAddress( LogicalAddress * location, NameNode * new_value, int pc_offset );

        // Providing the locations of the arguments is superfluous but is convenient for verification.
        //  - for top level scopes the argument locations are fixed (on the stack provided by the caller).
        //  - for inlined scopes the expression stack of the caller describes the argument locations.
        //
        // %implementation-note:
        // For now the argument locations are saved since the function to compute expression stack has not been implemented.
        void addArgument( ScopeInfo scope, int index, LogicalAddress * location );

        // Interface for creating the pc-offset <-> (ScopeDescriptor, byteCodeIndex) mapping.
        void addProgramCounterDescriptor( int pcOffset, ScopeInfo scope, int byteCodeIndex );

        void addIllegalProgramCounterDescriptor( int pcOffset );

        // Dependencies
        void add_dependent( LookupKey * key );

        // Returns the size of the generated scopeDescs.
        int size();

        // Copy the generated scopeDescs to 'addr'
        void copyTo( NativeMethod * nativeMethod );

        void verify( NativeMethodScopes * scopes );

        // Generates the the scopeDesc information.
        void generate();

    private:
        // Called before storring a location to resolve the context scope index into an offset
        Location convert_location( Location loc );

        ScopeInfo addScope( ScopeInfo scope, ScopeInfo senderScope );

        NonInlinedBlockScopeNode * addNonInlinedBlockScope( NonInlinedBlockScopeNode * scope );

        void genScopeDescHeader( uint8_t code, bool_t lite, bool_t args, bool_t temps, bool_t context_temps, bool_t expr_stack, bool_t has_context, bool_t bigHeader );

        // Generate the collected dependecies
        void generateDependencies();

        void emit_termination_node();

        void emit_illegal_node( bool_t is_last );

        // Returns true if was possible to save exprOffset and nextOffset in the two pre-allocated bytes.
        int updateScopeDescHeader( int offset, int next );

        void updateExtScopeDescHeader( int offset, int next );

        int getValueIndex( int v );

        int getOopIndex( Oop o );

        void genIndex( int index );

        void genValue( int v );

        void genOop( Oop o );

        // Make the private stuff reachable from the internal nodes
        friend class NameNode;

        friend class LocationName;

        friend class ValueName;

        friend class MemoizedName;

        friend class BlockValueName;

        friend class IllegalName;

        friend class ScopeDescriptorNode;

        friend class MethodScopeNode;

        friend class BlockScopeNode;

        friend class TopLevelBlockScopeNode;

        friend class NonInlinedBlockScopeNode;

        friend class LogicalAddress;

        friend class NameList;
};

enum {
    LOCATION_CODE,      //
    VALUE_CODE,         //
    BLOCKVALUE_CODE,    //
    MEMOIZEDBLOCK_CODE  //
};

// helper data structures used during packing and unpacking
class nameDescHeaderByte : public ValueObject {

    private:
        uint8_t              _byte;
        static const uint8_t _codeWidth;
        static const uint8_t _indexWidth;
        static const uint8_t _isLastBitNum;
        static const uint8_t _maxCode;


        uint8_t raw_index() {
            return lowerBits( _byte >> _codeWidth, _indexWidth );
        }


    public:
        static const uint8_t _maxIndex;
        static const uint8_t _noIndex;
        static const uint8_t _terminationIndex;
        static const uint8_t _illegalIndex;


        uint8_t value() {
            return _byte;
        }


        uint8_t code() {
            return lowerBits( _byte, _codeWidth );
        }


        uint8_t index() {
            st_assert( has_index(), "must have valid index" );
            return raw_index();
        }


        bool_t is_illegal() {
            return raw_index() == _illegalIndex;
        }


        bool_t is_termination() {
            return raw_index() == _terminationIndex;
        }


        bool_t is_last() {
            return isBitSet( _byte, _isLastBitNum );
        }


        bool_t has_index() {
            return raw_index() <= _maxIndex;
        }


        void pack( uint8_t code, bool_t is_last, uint8_t i ) {
            st_assert( code <= _maxCode, "code too high" );
            st_assert( i <= _noIndex, "index too high" );

            _byte = addBits( i << _codeWidth, code );

            if ( is_last )
                _byte = setNthBit( _byte, _isLastBitNum );
        }


        void pack_illegal( bool_t is_last ) {

            _byte = addBits( _illegalIndex << _codeWidth, 0 );

            if ( is_last )
                _byte = setNthBit( _byte, _isLastBitNum );
        }


        void pack_termination( bool_t is_last ) {

            _byte = addBits( _terminationIndex << _codeWidth, 0 );

            if ( is_last )
                _byte = setNthBit( _byte, _isLastBitNum );
        }


        void unpack( uint8_t value ) {
            _byte = value;
        }
};

class ScopeDescriptorHeaderByte : public ValueObject {

    private:
        uint8_t              _byte;
        static const uint8_t _codeWidth;
        static const uint8_t _maxCode;
        static const uint8_t _liteBitNum;
        static const uint8_t _argsBitNum;
        static const uint8_t _tempsBitNum;
        static const uint8_t _contextTempsBitNum;
        static const uint8_t _exprStackBitNum;
        static const uint8_t _contextBitNum;

    public:
        uint8_t value() {
            return _byte;
        }


        uint8_t code() {
            return lowerBits( _byte, _codeWidth );
        }


        bool_t is_lite() {
            return isBitSet( _byte, _liteBitNum );
        }


        bool_t has_args() {
            return isBitSet( _byte, _argsBitNum );
        }


        bool_t has_temps() {
            return isBitSet( _byte, _tempsBitNum );
        }


        bool_t has_context_temps() {
            return isBitSet( _byte, _contextTempsBitNum );
        }


        bool_t has_expr_stack() {
            return isBitSet( _byte, _exprStackBitNum );
        }


        bool_t has_compiled_context() {
            return isBitSet( _byte, _contextBitNum );
        }


        bool_t has_nameDescs() {
            return has_args() or has_temps() or has_context_temps() or has_expr_stack();
        }


        void pack( uint8_t code, bool_t lite, bool_t args, bool_t temps, bool_t context_temps, bool_t expr_stack, bool_t has_compiled_context ) {

            st_assert( code <= _maxCode, "code too high" );

            _byte = code;

            if ( lite )
                _byte = setNthBit( _byte, _liteBitNum );

            if ( args )
                _byte = setNthBit( _byte, _argsBitNum );

            if ( temps )
                _byte = setNthBit( _byte, _tempsBitNum );

            if ( context_temps )
                _byte = setNthBit( _byte, _contextTempsBitNum );

            if ( expr_stack )
                _byte = setNthBit( _byte, _exprStackBitNum );

            if ( has_compiled_context )
                _byte = setNthBit( _byte, _contextBitNum );

        }


        void unpack( uint8_t value ) {
            _byte = value;
        }

};
