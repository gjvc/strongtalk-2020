//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/utilities/GrowableArray.hpp"



// UnpackClosure is used for iteration over a string of nameDescs with different pc values.

class UnpackClosure : public StackAllocatedObject {
public:
    virtual void nameDescAt( NameDescriptor *nameDesc, const char *pc ) = 0;
};


//
// NativeMethodScopes represent the compressed NativeMethod source-level debugging information generated by the ScopeDescriptorRecorder.
// Whenever debugging information is needed, it is uncompressed into ScopeDescriptor instances.
//
// Compression works mainly by putting all "large" data (e.g., oops) into an array and
// using small (8-bit) indices to refer to this array from the actual scope data structures.
//

#define FOR_EACH_SCOPE( SCOPES, VAR ) \
   for( ScopeDescriptor* VAR = SCOPES->getNext( nullptr ); VAR not_eq nullptr; VAR = SCOPES->getNext( VAR ) )


// Implements the compact version of NativeMethod scopes with byte codes, shared Oop array, and value array.

class NativeMethodScopes : public ValueObject {

private:
    std::size_t           _nativeMethodOffset;   //
    std::uint16_t _length;               //
    std::uint16_t _oopsOffset;           // word offset to the oops array
    std::uint16_t _valueOffset;          // word offset to the value array
    std::uint16_t _pcsOffset;            // word offset to ProgramCounterDescriptor array
    std::size_t           _dependentsEnd;        // size of dependents

private:
    static std::uint16_t pack_word_aligned( std::size_t value ) {
        st_assert( value % BytesPerWord == 0, "value should be word aligned" );
        st_assert( value >> BytesPerWord <= nthMask( BitsPerByte * sizeof( std::uint16_t ) ), "value exceeds limit" );
        return value >> LogBytesPerWord;
    }


    static std::size_t unpack_word_aligned( std::uint16_t v ) {
        return v << LogBytesPerWord;
    }


    std::size_t oops_offset() const {
        return unpack_word_aligned( _oopsOffset );
    }


    std::size_t value_offset() const {
        return unpack_word_aligned( _valueOffset );
    }


    std::size_t pcs_offset() const {
        return unpack_word_aligned( _pcsOffset );
    }


    // Return the address after the struct header
    std::uint8_t *start() const {
        return (std::uint8_t *) ( this + 1 );
    }


public: // for debugging
    Oop *oops() const {
        return (Oop *) ( start() + oops_offset() );
    }


    std::size_t oops_size() const {
        return ( value_offset() - oops_offset() ) / sizeof( Oop );
    }


    Oop oop_at( std::size_t index ) const {
        st_assert( index < oops_size(), "oops index out of range" );
        return oops()[ index ];
    }


private:
    std::size_t *values() const {
        return (std::size_t *) ( start() + value_offset() );
    }


    std::size_t value_size() const {
        return ( pcs_offset() - value_offset() ) / sizeof( std::size_t );
    }


    std::size_t value_at( std::size_t index ) const {
        st_assert( index < value_size(), "oops index out of range" );
        return values()[ index ];
    }


    inline std::uint8_t getIndexAt( std::size_t &offset ) const;

    inline Oop unpackOopFromIndex( std::uint8_t index, std::size_t &offset ) const;

    inline std::size_t unpackValueFromIndex( std::uint8_t index, std::size_t &offset ) const;

private:
    friend class ScopeDescriptorRecorder;


    void set_nativeMethodOffset( std::size_t v ) {
        _nativeMethodOffset = v;
    }


    void set_length( std::size_t v ) {
        _length = pack_word_aligned( v );
    }


    void set_oops_offset( std::size_t v ) {
        _oopsOffset = pack_word_aligned( v );
    }


    void set_value_offset( std::size_t v ) {
        _valueOffset = pack_word_aligned( v );
    }


    void set_pcs_offset( std::size_t v ) {
        _pcsOffset = pack_word_aligned( v );
    }


    void set_dependents_end( std::size_t v ) {
        _dependentsEnd = v;
    }


public:
    KlassOop dependent_at( std::size_t index ) const {
        st_assert( index >= 0 and index < dependent_length(), "must be within bounds" );
        Oop result = oop_at( index );
        st_assert( result->is_klass(), "must be klass" );
        return KlassOop( result );
    }


    std::size_t dependent_length() const {
        return _dependentsEnd;
    }


    void *pcs() const {
        return (void *) ( start() + pcs_offset() );
    }


    void *pcsEnd() const {
        return (void *) end();
    }


    std::size_t length() const {
        return unpack_word_aligned( _length );
    }


    NativeMethod *my_nativeMethod() const {
        return (NativeMethod *) ( ( (const char *) this ) - _nativeMethodOffset );
    };


    // returns the address following this NativeMethodScopes.
    ScopeDescriptor *end() const {
        return (ScopeDescriptor *) ( start() + length() );
    }


    bool_t includes( ScopeDescriptor *d ) const {
        return this == d->_scopes;
    }


    // Returns the root scope without pc specific information.
    // The returned scope cannot be used for retrieving name desc information.
    ScopeDescriptor *root() const {
        return at( 0, ScopeDescriptor::invalid_pc );
    }


    std::size_t size() const {
        return sizeof( NativeMethodScopes ) + length();
    }


    // Returns a scope located at offset.
    ScopeDescriptor *at( std::size_t offset, const char *pc ) const;

    NonInlinedBlockScopeDescriptor *noninlined_block_scope_at( std::size_t offset ) const;


    // used in iterator macro FOR_EACH_SCOPE
    ScopeDescriptor *getNext( ScopeDescriptor *s ) const {
        if ( not s )
            return root();
        std::size_t offset = s->next_offset();

        if ( offset + ( sizeof( std::size_t ) - ( offset % sizeof( std::size_t ) ) ) % sizeof( std::size_t ) >= ( _oopsOffset ) * sizeof( Oop ) )
            return nullptr;

        return at( offset, ScopeDescriptor::invalid_pc );
    }


    std::uint8_t get_next_char( std::size_t &offset ) const {
        return *( start() + offset++ );
    }


    std::int16_t get_next_half( std::size_t &offset ) const;


    std::uint8_t peek_next_char( std::size_t offset ) const {
        return *( start() + offset );
    }


    Oop unpackOopAt( std::size_t &offset ) const;

    std::size_t unpackValueAt( std::size_t &offset ) const;

    void iterate( std::size_t &offset, UnpackClosure *closure ) const;    // iterates over a string of NameDescs (iterator is not called at termination)
    NameDescriptor *unpackNameDescAt( std::size_t &offset, const char *pc ) const;    // Unpacks a string of name descs and returns one matching the pc

private:
    NameDescriptor *unpackNameDescAt( std::size_t &offset, bool_t &is_last, const char *pc ) const;    // Unpacks a single name desc at offset

public:

    // Support for garbage collection.
    void oops_do( void f( Oop * ) );

    void scavenge_contents();

    void switch_pointers( Oop from, Oop to, GrowableArray<NativeMethod *> *nativeMethods_to_invalidate );

    bool_t is_new() const;

    void relocate();

    void verify();

    void print();

    // Prints (dep d% oops %d bytes %d pcs %d)
    void print_partition();
};
