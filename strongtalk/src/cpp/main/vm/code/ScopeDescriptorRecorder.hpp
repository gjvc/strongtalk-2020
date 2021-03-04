
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utility/GrowableArray.hpp"
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
    bool                              _hasCodeBeenGenerated;
    Array                             *_oops;
    Array                             *_values;
    ByteArray                         *_codes;
    ProgramCounterDescriptorInfoClass *_programCounterDescriptorInfo;

    GrowableArray<KlassOop> *_dependents;
    std::int32_t            _dependentsEnd;

    NonInlinedBlockScopeNode *_nonInlinedBlockScopeNode;
    NonInlinedBlockScopeNode *_nonInlinedBlockScopesTail;

public:
    ScopeInfo _root;


    ScopeDescriptorRecorder() = default;
    virtual ~ScopeDescriptorRecorder() = default;
    ScopeDescriptorRecorder( const ScopeDescriptorRecorder & ) = default;
    ScopeDescriptorRecorder &operator=( const ScopeDescriptorRecorder & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    // Returns the offset for a scopeDesc after generation of the scopeDesc info.
    std::int32_t offset( ScopeInfo scope );

    std::int32_t offset_for_noninlined_scope_node( NonInlinedBlockScopeNode *scope );

    ScopeDescriptorRecorder( std::int32_t scopeSize, // estimated size of scopes (in bytes)
                             std::int32_t npcDesc );  // estimated number of ProgramCounterDescriptors

    // Adds a method scope
    ScopeInfo addMethodScope( LookupKey *key,                      // lookup key
                              MethodOop method,                     // result of the lookup
                              LogicalAddress *receiver_location,   // location of receiver
                              bool allocates_compiled_context,    // tells whether the code allocates a context
                              bool lite = false,                  //
                              std::int32_t scopeID = 0,                      //
                              ScopeInfo senderScope = nullptr,      //
                              std::int32_t senderByteCodeIndex = IllegalByteCodeIndex,
                              bool visible = false );


    // Adds an inlined block scope
    ScopeInfo addBlockScope( MethodOop method,                      // block method
                             ScopeInfo parent,                      // parent scope
                             bool allocates_compiled_context,     // tells whether the code allocates a context
                             bool lite = false, std::int32_t scopeID = 0,  //
                             ScopeInfo senderScope = nullptr,       //
                             std::int32_t senderByteCodeIndex = IllegalByteCodeIndex,
                             bool visible = false );

    // Adds a top level block scope
    ScopeInfo addTopLevelBlockScope( MethodOop method,                      // block method
                                     LogicalAddress *receiver_location,    // location of receiver
                                     KlassOop receiver_klass,               // receiver klass
                                     bool allocates_compiled_context );   // tells whether the code allocates a context

    // Adds an noninlined block scope
    // Used for retrieving information about block closure stubs
    NonInlinedBlockScopeNode *addNonInlinedBlockScope( MethodOop block_method, ScopeInfo parent );

    // Interface for adding name nodes.
    void addTemporary( ScopeInfo scope, std::int32_t index, LogicalAddress *location );         // all entries [0..max(index)] must be filled.
    void addContextTemporary( ScopeInfo scope, std::int32_t index, LogicalAddress *location );  // all entries [0..max(index)] must be filled.
    void addExprStack( ScopeInfo scope, std::int32_t byteCodeIndex, LogicalAddress *location ); // sparse array = some entries may be left empty.

    LogicalAddress *createLogicalAddress( NameNode *initial_value );

    void changeLogicalAddress( LogicalAddress *location, NameNode *new_value, std::int32_t pc_offset );

    // Providing the locations of the arguments is superfluous but is convenient for verification.
    //  - for top level scopes the argument locations are fixed (on the stack provided by the caller).
    //  - for inlined scopes the expression stack of the caller describes the argument locations.
    //
    // %implementation-note:
    // For now the argument locations are saved since the function to compute expression stack has not been implemented.
    void addArgument( ScopeInfo scope, std::int32_t index, LogicalAddress *location );

    // Interface for creating the pc-offset <-> (ScopeDescriptor, byteCodeIndex) mapping.
    void addProgramCounterDescriptor( std::int32_t pcOffset, ScopeInfo scope, std::int32_t byteCodeIndex );

    void addIllegalProgramCounterDescriptor( std::int32_t pcOffset );

    // Dependencies
    void add_dependent( LookupKey *key );

    // Returns the size of the generated scopeDescs.
    std::int32_t size();

    // Copy the generated scopeDescs to 'addr'
    void copyTo( NativeMethod *nativeMethod );

    void verify( NativeMethodScopes *scopes );

    // Generates the the scopeDesc information.
    void generate();

private:
    // Called before storring a location to resolve the context scope index into an offset
    Location convert_location( Location loc );

    ScopeInfo addScope( ScopeInfo scope, ScopeInfo senderScope );

    NonInlinedBlockScopeNode *addNonInlinedBlockScope( NonInlinedBlockScopeNode *scope );

    void genScopeDescHeader( std::uint8_t code, bool lite, bool args, bool temps, bool context_temps, bool expr_stack, bool has_context, bool bigHeader );

    // Generate the collected dependecies
    void generateDependencies();

    void emit_termination_node();

    void emit_illegal_node( bool is_last );

    // Returns true if was possible to save exprOffset and nextOffset in the two pre-allocated bytes.
    std::int32_t updateScopeDescHeader( std::int32_t offset, std::int32_t next );

    void updateExtScopeDescHeader( std::int32_t offset, std::int32_t next );

    std::int32_t getValueIndex( std::int32_t v );

    std::int32_t getOopIndex( Oop o );

    void genIndex( std::int32_t index );

    void genValue( std::int32_t v );

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
    std::uint8_t              _byte;
    static const std::uint8_t _codeWidth;
    static const std::uint8_t _indexWidth;
    static const std::uint8_t _isLastBitNum;
    static const std::uint8_t _maxCode;


    std::uint8_t raw_index() {
        return lowerBits( _byte >> _codeWidth, _indexWidth );
    }


public:
    static const std::uint8_t _maxIndex;
    static const std::uint8_t _noIndex;
    static const std::uint8_t _terminationIndex;
    static const std::uint8_t _illegalIndex;


    std::uint8_t value() {
        return _byte;
    }


    std::uint8_t code() {
        return lowerBits( _byte, _codeWidth );
    }


    std::uint8_t index() {
        st_assert( has_index(), "must have valid index" );
        return raw_index();
    }


    bool is_illegal() {
        return raw_index() == _illegalIndex;
    }


    bool is_termination() {
        return raw_index() == _terminationIndex;
    }


    bool is_last() {
        return isBitSet( _byte, _isLastBitNum );
    }


    bool has_index() {
        return raw_index() <= _maxIndex;
    }


    void pack( std::uint8_t code, bool is_last, std::uint8_t i ) {
        st_assert( code <= _maxCode, "code too high" );
        st_assert( i <= _noIndex, "index too high" );

        _byte = addBits( i << _codeWidth, code );

        if ( is_last )
            _byte = setNthBit( _byte, _isLastBitNum );
    }


    void pack_illegal( bool is_last ) {

        _byte = addBits( _illegalIndex << _codeWidth, 0 );

        if ( is_last )
            _byte = setNthBit( _byte, _isLastBitNum );
    }


    void pack_termination( bool is_last ) {

        _byte = addBits( _terminationIndex << _codeWidth, 0 );

        if ( is_last )
            _byte = setNthBit( _byte, _isLastBitNum );
    }


    void unpack( std::uint8_t value ) {
        _byte = value;
    }
};


class ScopeDescriptorHeaderByte : public ValueObject {

private:
    std::uint8_t              _byte;
    static const std::uint8_t _codeWidth;
    static const std::uint8_t _maxCode;
    static const std::uint8_t _liteBitNum;
    static const std::uint8_t _argsBitNum;
    static const std::uint8_t _tempsBitNum;
    static const std::uint8_t _contextTempsBitNum;
    static const std::uint8_t _exprStackBitNum;
    static const std::uint8_t _contextBitNum;

public:
    std::uint8_t value() {
        return _byte;
    }


    std::uint8_t code() {
        return lowerBits( _byte, _codeWidth );
    }


    bool is_lite() {
        return isBitSet( _byte, _liteBitNum );
    }


    bool has_args() {
        return isBitSet( _byte, _argsBitNum );
    }


    bool has_temps() {
        return isBitSet( _byte, _tempsBitNum );
    }


    bool has_context_temps() {
        return isBitSet( _byte, _contextTempsBitNum );
    }


    bool has_expr_stack() {
        return isBitSet( _byte, _exprStackBitNum );
    }


    bool has_compiled_context() {
        return isBitSet( _byte, _contextBitNum );
    }


    bool has_nameDescs() {
        return has_args() or has_temps() or has_context_temps() or has_expr_stack();
    }


    void pack( std::uint8_t code, bool lite, bool args, bool temps, bool context_temps, bool expr_stack, bool has_compiled_context ) {

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


    void unpack( std::uint8_t value ) {
        _byte = value;
    }

};
