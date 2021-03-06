//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/code/MethodScopeNode.hpp"
#include "vm/code/BlockScopeNode.hpp"
#include "vm/system/dll.hpp"
#include "vm/code/NativeMethodScopes.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/code/TopLevelBlockScopeNode.hpp"

extern Compiler *theCompiler;

//
// Todo list
// - Insert Logical addresses
//   If there is only on physical address they should be stored a today.
//

constexpr std::int32_t INITIAL_OOPS_SIZE                = 100;
constexpr std::int32_t INITIAL_VALUES_SIZE              = 100;
constexpr std::int32_t INITIAL_DEPENDENTS_SIZE          = 20;
constexpr std::int32_t INITIAL_CONTEXT_SCOPE_ARRAY_SIZE = 10;

const std::uint8_t nameDescHeaderByte::_codeWidth        = 2;
const std::uint8_t nameDescHeaderByte::_indexWidth       = 5;
const std::uint8_t nameDescHeaderByte::_isLastBitNum     = _codeWidth + _indexWidth;
const std::uint8_t nameDescHeaderByte::_maxCode          = nthMask( _codeWidth );
const std::uint8_t nameDescHeaderByte::_maxIndex         = nthMask( _indexWidth ) - 3;
const std::uint8_t nameDescHeaderByte::_noIndex          = nthMask( _indexWidth ) - 2;
const std::uint8_t nameDescHeaderByte::_terminationIndex = nthMask( _indexWidth ) - 1;
const std::uint8_t nameDescHeaderByte::_illegalIndex     = nthMask( _indexWidth ) - 0;

const std::uint8_t ScopeDescriptorHeaderByte::_codeWidth          = 2;                             // 2 bits: scopeDesc type
const std::uint8_t ScopeDescriptorHeaderByte::_maxCode            = nthMask( _codeWidth );
const std::uint8_t ScopeDescriptorHeaderByte::_liteBitNum         = _codeWidth;                    // 1 bit:  is lite scope desc
const std::uint8_t ScopeDescriptorHeaderByte::_argsBitNum         = _codeWidth + 1;                // 1 bit:  has arguments
const std::uint8_t ScopeDescriptorHeaderByte::_tempsBitNum        = _codeWidth + 2;                // 1 bit:  has temporaries
const std::uint8_t ScopeDescriptorHeaderByte::_contextTempsBitNum = _codeWidth + 3;                // 1 bit:  has expression stack
const std::uint8_t ScopeDescriptorHeaderByte::_exprStackBitNum    = _codeWidth + 4;                // 1 bit:  has context temporaries
const std::uint8_t ScopeDescriptorHeaderByte::_contextBitNum      = _codeWidth + 5;                // 1 bit:  has context


NameNode *newValueName( Oop value ) {
    if ( value->is_block() ) {
        st_fatal( "should never be a block" );
        return nullptr;
    } else {
        return new ValueName( value );
    }
}


bool NameNode::genHeaderByte( ScopeDescriptorRecorder *rec, std::uint8_t code, bool is_last, std::int32_t index ) {
    // Since id is most likely to be 0, the info part of the header byte indicates if is is non zero.
    // Experiments show id is zero in at least 90% of the generated nameDescs.
    // returns true if index could be inlined in headerByte.
    nameDescHeaderByte b;
    bool               can_inline  = index <= b._maxIndex;
    std::uint8_t       coded_index = can_inline ? index : b._noIndex;
    b.pack( code, is_last, coded_index );
    rec->_codes->appendByte( b.value() );

    return can_inline;
}


std::int32_t ScopeDescriptorRecorder::getValueIndex( std::int32_t v ) {
    // if v fits into 7 bits inline the value instead of creating index
    if ( 0 <= v and v <= MAX_INLINE_VALUE )
        return v;
    return MAX_INLINE_VALUE + 1 + _values->insertIfAbsent( v );
}


std::int32_t ScopeDescriptorRecorder::getOopIndex( Oop o ) {
    return o == 0 ? 0 : _oops->insertIfAbsent( (std::int32_t) o ) + 1;
}


void ScopeDescriptorRecorder::emit_illegal_node( bool is_last ) {
    nameDescHeaderByte b;
    b.pack_illegal( is_last );
    _codes->appendByte( b.value() );
}


void ScopeDescriptorRecorder::emit_termination_node() {
    nameDescHeaderByte b;
    b.pack_termination( true );
    _codes->appendByte( b.value() );
}


void IllegalName::generate( ScopeDescriptorRecorder *rec, bool is_last ) {
    rec->emit_illegal_node( is_last );
}


void ScopeDescriptorRecorder::generate() {
    st_assert( _root, "root scope must be present" );
    // Generate the bytecodes for the ScopeDescs and their NameDescs.

    generateDependencies();

    _programCounterDescriptorInfo->mark_scopes();
    (void) _root->computeVisibility();
    _root->generate( this, 0, false );
    _root->generateBody( this, 0 );

    for ( NonInlinedBlockScopeNode *p = _nonInlinedBlockScopeNode; p not_eq nullptr; p = p->_next ) {
        p->generate( this );
    }

    _codes->alignToWord();
    _hasCodeBeenGenerated = true;
}


void ScopeDescriptorRecorder::generateDependencies() {
    std::int32_t end_marker = 0;

    for ( std::size_t index = 0; index < _dependents->length(); index++ ) {

        std::int32_t i = _oops->insertIfAbsent( (std::int32_t) _dependents->at( index ) );
        if ( i > end_marker )
            end_marker = i;
    }

    _dependentsEnd = end_marker;
}


ScopeInfo ScopeDescriptorRecorder::addScope( ScopeInfo scope, ScopeInfo senderScope ) {
    if ( _root == nullptr ) {
        st_assert( senderScope == nullptr, "Root scope must be the first" );
        _root = scope;
    } else {
        st_assert( senderScope not_eq nullptr, "Sender scope must be present" );
        senderScope->addNested( scope );
    }
    return scope;
}


NonInlinedBlockScopeNode *ScopeDescriptorRecorder::addNonInlinedBlockScope( NonInlinedBlockScopeNode *scope ) {
    scope->_next = nullptr;
    if ( _nonInlinedBlockScopeNode == nullptr ) {
        _nonInlinedBlockScopeNode = _nonInlinedBlockScopesTail = scope;
    } else {
        _nonInlinedBlockScopesTail->_next = scope;
        _nonInlinedBlockScopesTail = scope;
    }

    return scope;
}


std::int32_t ScopeDescriptorRecorder::offset( ScopeInfo scope ) {
    st_assert( scope->_offset not_eq INVALID_OFFSET, "uninitialized offset" );
    return scope->_offset;
}


std::int32_t ScopeDescriptorRecorder::offset_for_noninlined_scope_node( NonInlinedBlockScopeNode *scope ) {
    st_assert( scope->_offset not_eq INVALID_OFFSET, "uninitialized offset" );
    return scope->_offset;
}


ScopeInfo ScopeDescriptorRecorder::addMethodScope( LookupKey *key, MethodOop method, LogicalAddress *receiver_location, bool allocates_compiled_context, bool lite, std::int32_t scopeID, ScopeInfo senderScope, std::int32_t senderByteCodeIndex, bool visible ) {
    return addScope( new MethodScopeNode( key, method, receiver_location, allocates_compiled_context, lite, scopeID, senderByteCodeIndex, visible ), senderScope );
}


ScopeInfo ScopeDescriptorRecorder::addBlockScope( MethodOop method, ScopeInfo parent, bool allocates_compiled_context, bool lite, std::int32_t scopeID, ScopeInfo senderScope, std::int32_t senderByteCodeIndex, bool visible ) {
    return addScope( new BlockScopeNode( method, parent, allocates_compiled_context, lite, scopeID, senderByteCodeIndex, visible ), senderScope );
}


ScopeInfo ScopeDescriptorRecorder::addTopLevelBlockScope( MethodOop method, LogicalAddress *receiver_location, KlassOop receiver_klass, bool allocates_compiled_context ) {
    return addScope( new TopLevelBlockScopeNode( method, receiver_location, receiver_klass, allocates_compiled_context ), nullptr );
}


NonInlinedBlockScopeNode *ScopeDescriptorRecorder::addNonInlinedBlockScope( MethodOop method, ScopeInfo parent ) {

    return addNonInlinedBlockScope( new NonInlinedBlockScopeNode( method, parent ) );
}


void ScopeDescriptorRecorder::addArgument( ScopeInfo scope, std::int32_t index, LogicalAddress *location ) {
    st_assert( not scope->_lite, "cannot add slot to lite scopeDesc" );
    scope->_arg_list->at_put_grow( index, location );
}


void ScopeDescriptorRecorder::addTemporary( ScopeInfo scope, std::int32_t index, LogicalAddress *location ) {
    st_assert( not scope->_lite, "cannot add slot to lite scopeDesc" );
    scope->_temp_list->at_put_grow( index, location );
}


void ScopeDescriptorRecorder::addExprStack( ScopeInfo scope, std::int32_t index, LogicalAddress *location ) {
    st_assert( not scope->_lite, "cannot add expression to lite scopeDesc" );
    scope->_expr_stack_list->at_put_grow( index, location );
}


void ScopeDescriptorRecorder::addContextTemporary( ScopeInfo scope, std::int32_t index, LogicalAddress *location ) {
    st_assert( not scope->_lite, "cannot add expression to lite scopeDesc" );
    scope->_context_temp_list->at_put_grow( index, location );
}


LogicalAddress *ScopeDescriptorRecorder::createLogicalAddress( NameNode *initial_value ) {
    return new LogicalAddress( initial_value );
}


void ScopeDescriptorRecorder::changeLogicalAddress( LogicalAddress *location, NameNode *new_value, std::int32_t pc_offset ) {
    location->append( new_value, pc_offset );
}


void ScopeDescriptorRecorder::genScopeDescHeader( std::uint8_t code, bool lite, bool args, bool temps, bool context_temps, bool expr_stack, bool has_context, bool bigHeader ) {
    ScopeDescriptorHeaderByte b;
    b.pack( code, lite, args, temps, context_temps, expr_stack, has_context );
    _codes->appendByte( b.value() );
    if ( b.has_nameDescs() ) {
        _codes->appendByte( 0 ); // placeholder for next offset
        if ( bigHeader ) {
            _codes->appendHalf( 0 );
        }
    }
}


std::int32_t ScopeDescriptorRecorder::updateScopeDescHeader( std::int32_t offset, std::int32_t next ) {
    std::int32_t nextIndex = getValueIndex( next - offset );
    if ( nextIndex < EXTENDED_INDEX ) {
        _codes->putByteAt( nextIndex, offset + 1 );
        return true;
    } else {
        return false;
    }
}


void ScopeDescriptorRecorder::updateExtScopeDescHeader( std::int32_t offset, std::int32_t next ) {
    std::int32_t nextIndex = getValueIndex( next - offset );
    _codes->putByteAt( EXTENDED_INDEX, offset + 1 );
    _codes->putHalfAt( nextIndex, offset + 2 );
}


void ScopeDescriptorRecorder::genIndex( std::int32_t index ) {
    if ( index < EXTENDED_INDEX ) {
        // generate 1 byte indexing the Oop.
        _codes->appendByte( index );
    } else {
        _codes->appendByte( EXTENDED_INDEX );
        _codes->appendHalf( index );
    }
}


void ScopeDescriptorRecorder::genValue( std::int32_t v ) {
    genIndex( getValueIndex( v ) );
}


void ScopeDescriptorRecorder::genOop( Oop o ) {
    genIndex( getOopIndex( o ) );
}


Location ScopeDescriptorRecorder::convert_location( Location loc ) {
    if ( not loc.isContextLocation() )
        return loc;
    std::int32_t scope_id = loc.scopeID();

    // Find the getScopeInfo with the right scope_id
    ScopeInfo scope = theCompiler->scopes->at( scope_id )->getScopeInfo();
    st_assert( scope, "scope must exist" );
    if ( scope->_offset == INVALID_OFFSET ) {
        SPDLOG_INFO( loc.name() );
        theCompiler->print_code( false );
        st_fatal( "compiler error: context location appears outside its scope" );    // Urs 5/96
    }
    return Location::runtimeContextLocation( loc.contextNo(), loc.tempNo(), scope->_offset );
}


std::int32_t ScopeDescriptorRecorder::size() {
    return sizeof( NativeMethodScopes )
           + _codes->size()
           + ( _oops->length() * sizeof( Oop ) )
           + ( _values->length() * sizeof( std::int32_t ) )
           + ( _programCounterDescriptorInfo->length() * sizeof( ProgramCounterDescriptor ) );
}


ScopeDescriptorRecorder::ScopeDescriptorRecorder( std::int32_t byte_size, std::int32_t pcDesc_size ) :

    _hasCodeBeenGenerated{ false },

    _oops{ new Array( INITIAL_OOPS_SIZE ) },
    _values{ new Array( INITIAL_VALUES_SIZE ) },
    _codes{ new ByteArray( byte_size ) },
    _programCounterDescriptorInfo{ new ProgramCounterDescriptorInfoClass( pcDesc_size ) },

    _dependents{ new GrowableArray<KlassOop>( INITIAL_DEPENDENTS_SIZE ) },
    _dependentsEnd{ 0 },

    _nonInlinedBlockScopeNode{ nullptr },
    _nonInlinedBlockScopesTail{ nullptr },

    _root{ nullptr } {

}


void ScopeDescriptorRecorder::copyTo( NativeMethod *nativeMethod ) {

    // destination
    NativeMethodScopes *d = (NativeMethodScopes *) nativeMethod->scopes();

    // Copy the body part of the NativeMethodScopes
    std::size_t *start = ( std::size_t * )( d + 1 );
    std::size_t *p     = start;

    d->set_nativeMethodOffset( (const char *) d - (const char *) nativeMethod );

    _codes->copy_to( p );

    d->set_oops_offset( (const char *) p - (const char *) start );
    _oops->copy_to( p );

    d->set_value_offset( (const char *) p - (const char *) start );
    _values->copy_to( p );

    d->set_pcs_offset( (const char *) p - (const char *) start );
    _programCounterDescriptorInfo->copy_to( p );

    d->set_length( (const char *) p - (const char *) start );

    d->set_dependents_end( _dependentsEnd );

    st_assert( (const char *) d + size() == (const char *) p, "wrong size of NativeMethodScopes" );
}


void ScopeDescriptorRecorder::addProgramCounterDescriptor( std::int32_t pcOffset, ScopeInfo scope, std::int32_t byteCodeIndex ) {
    st_assert( scope, "scope must be specified in addProgramCounterDescriptor" );
    st_assert( _root, "root must be present" );
    _programCounterDescriptorInfo->add( pcOffset, scope, byteCodeIndex );
}


void ScopeDescriptorRecorder::add_dependent( LookupKey *key ) {
    // make this NativeMethod dependent on the receiver klass of the lookup key.
    _dependents->append( key->klass() );
}


// Please encapsulate iterator.
ScopeDescriptor    *_sd;
NativeMethodScopes *_scopes;


ScopeDescriptor *_getNextScopeDescriptor() {
    _sd = _scopes->getNext( _sd );
    if ( _sd == nullptr ) st_fatal( "out of ScopeDescriptor instances" );
    return _sd;
}


void ScopeDescriptorRecorder::verify( NativeMethodScopes *scopes ) {
    // Initialize iterator
    _scopes = scopes;
    _sd     = nullptr;

    st_assert( _root, "root ScopeDescriptor must be present to verify" );
    _root->verify( _getNextScopeDescriptor() );
    _root->verifyBody();
}
