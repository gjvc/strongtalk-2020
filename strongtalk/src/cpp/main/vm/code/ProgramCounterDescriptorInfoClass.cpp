//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/ProgramCounterDescriptorInfoClass.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"


ProgramCounterDescriptorInfoClass::ProgramCounterDescriptorInfoClass( std::size_t sz ) :
    _nodes{ new_resource_array<ProgramCounterDescriptorNode>( sz ) },
    _end{ 0 },
    _size{ sz } {
}


void ProgramCounterDescriptorInfoClass::extend( std::size_t newSize ) {

    ProgramCounterDescriptorNode *newNodes = new_resource_array<ProgramCounterDescriptorNode>( newSize );

    for ( std::size_t i = 0; i < _end; i++ ) {
        newNodes[ i ] = _nodes[ i ];
    }

    _nodes = newNodes;
    _size  = newSize;
}


void ProgramCounterDescriptorInfoClass::add( std::int32_t pcOffset, ScopeInfo scope, std::size_t byteCodeIndex ) {

    //
    if ( scope->_lite and not GenerateLiteScopeDescs )
        return;

    //
    if ( _end == _size )
        extend( _size * 2 );

    // After Robert's jmp elimination, instructions can be eliminated
    // We can therefore remove pc descs describing the eliminated code.
    while ( _end > 0 and pcOffset < _nodes[ _end - 1 ]._pcOffset ) {
        _end--;
    }

    if ( CompressProgramCounterDescriptors and _end > 0 ) {
        // skip if the previous had the same scope and byteCodeIndex.
        if ( scope == _nodes[ _end - 1 ]._scopeInfo and byteCodeIndex == _nodes[ _end - 1 ]._byteCodeIndex )
            return;
        // overwrite if the previous had the same pcOffset.
        if ( pcOffset == _nodes[ _end - 1 ]._pcOffset ) {
            _end--;
        }
    }

    _nodes[ _end ]._pcOffset      = pcOffset;
    _nodes[ _end ]._scopeInfo     = scope;
    _nodes[ _end ]._byteCodeIndex = byteCodeIndex;
    _end++;
}


void ProgramCounterDescriptorInfoClass::mark_scopes() {
    for ( std::size_t i = 0; i < _end; i++ ) {
        if ( _nodes[ i ]._scopeInfo ) {
            _nodes[ i ]._scopeInfo->_usedInPcs = true;
        }
    }
}


void ProgramCounterDescriptorInfoClass::copy_to( std::size_t *&addr ) {
    for ( std::size_t i = 0; i < _end; i++ ) {
        ProgramCounterDescriptor *pc = (ProgramCounterDescriptor *) addr;
        pc->_pc            = _nodes[ i ]._pcOffset;
        pc->_scope         = _nodes[ i ]._scopeInfo ? _nodes[ i ]._scopeInfo->_offset : IllegalByteCodeIndex;
        pc->_byteCodeIndex = _nodes[ i ]._byteCodeIndex;
        addr += sizeof( ProgramCounterDescriptor ) / sizeof( std::size_t );
    }
}


void LocationName::generate( ScopeDescriptorRecorder *rec, bool is_last ) {
    Location     converted_location = rec->convert_location( _location );
    std::int32_t index              = rec->getValueIndex( converted_location._loc );
    if ( not genHeaderByte( rec, LOCATION_CODE, is_last, index ) )
        rec->genIndex( index );
}


void ValueName::generate( ScopeDescriptorRecorder *rec, bool is_last ) {
    std::int32_t index = rec->getOopIndex( _value );
    if ( not genHeaderByte( rec, VALUE_CODE, is_last, index ) )
        rec->genIndex( index );
}


void MemoizedName::generate( ScopeDescriptorRecorder *rec, bool is_last ) {
    Location     converted_location = rec->convert_location( _location );
    std::int32_t index              = rec->getValueIndex( converted_location._loc );
    if ( not genHeaderByte( rec, MEMOIZEDBLOCK_CODE, is_last, index ) )
        rec->genIndex( index );
    rec->genOop( Oop( _blockMethod ) );
    rec->genValue( _parentScope == nullptr ? 0 : _parentScope->_offset ); // Lars, please check this (gri 2/2/96)
}


void BlockValueName::generate( ScopeDescriptorRecorder *rec, bool is_last ) {
    std::int32_t index = rec->getOopIndex( Oop( _blockMethod ) );
    if ( not genHeaderByte( rec, BLOCKVALUE_CODE, is_last, index ) )
        rec->genIndex( index );
    rec->genValue( _parentScope == nullptr ? 0 : _parentScope->_offset ); // Lars, please check this (gri 2/2/96)
}
