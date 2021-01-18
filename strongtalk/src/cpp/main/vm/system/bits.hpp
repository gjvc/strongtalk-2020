
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/system/platform.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>


// -----------------------------------------------------------------------------

constexpr int AllBitsSet = ~0;
constexpr int NoBitsSet  = 0;
constexpr int OneBitSet  = 1;


// -----------------------------------------------------------------------------

constexpr auto nthBit( const auto n ) { return OneBitSet << ( n ); }


constexpr auto maskBits( const auto x, const auto m ) { return x & m; }


constexpr auto addBits( const auto x, const auto m ) { return x | m; }


constexpr auto subBits( const auto x, const auto m ) { return x & ~m; }


constexpr auto setBits( auto &x, const auto m ) { return x |= m; }


constexpr auto clearBits( auto &x, const auto m ) { return x &= ~m; }


constexpr auto addNthBit( const auto x, const auto n ) { return addBits( x, nthBit( n ) ); }


constexpr auto subNthBit( const auto x, const auto n ) { return subBits( x, nthBit( n ) ); }


constexpr auto setNthBit( auto x, const auto n ) { return setBits( x, nthBit( n ) ); }


constexpr auto clearNthBit( auto &x, const auto n ) { return clearBits( x, nthBit( n ) ); }


// -----------------------------------------------------------------------------

constexpr auto anyBitSet( auto x, const auto m ) { return maskBits( x, m ) not_eq NoBitsSet; }


constexpr auto isBitSet( auto x, const auto n ) { return anyBitSet( x, nthBit( n ) ); }


constexpr auto nthMask( const auto n ) { return ( n == BitsPerWord ? AllBitsSet : ( nthBit( n ) - OneBitSet ) ); }


// -----------------------------------------------------------------------------

constexpr auto lowerBits( auto x, const auto n ) { return maskBits( x, nthMask( n ) ); }


constexpr auto roundMask( auto x, const auto m ) { return x + m & ~m; }


constexpr auto roundTo( auto x, const auto v ) { return roundMask( x, v - OneBitSet ); }


constexpr auto roundBits( auto x, const auto n ) { return roundMask( x, nthMask( n ) ); }


// -----------------------------------------------------------------------------

constexpr std::size_t INTEGER_TAG  = 0;
constexpr std::size_t MEMOOP_TAG   = 1;
constexpr std::size_t MARK_TAG     = 3;
constexpr std::size_t MARK_TAG_BIT = 2;    // if ( (Oop & MARK_TAG_BIT) not_eq 0 )  then Oop is a markOop
constexpr std::size_t TAG_SIZE     = 2;
constexpr std::size_t TAG_MASK     = nthMask( TAG_SIZE );
constexpr std::size_t Num_Tags     = nthBit( TAG_SIZE );


// -----------------------------------------------------------------------------

//#define clearTag( Oop )  (int(Oop) & ~Tag_Mask)
constexpr auto clearTag( auto Oop ) { return reinterpret_cast<int>( Oop ) & ~TAG_MASK; }


// -----------------------------------------------------------------------------

constexpr int BYTE_WIDTH       = 8;
constexpr int EXTENDED_INDEX   = nthMask( BYTE_WIDTH );
constexpr int MAX_INLINE_VALUE = nthMask( BYTE_WIDTH - 1 );


// -----------------------------------------------------------------------------

inline int arithmetic_shift_right( int value, int shift ) {
    return value >> shift;
}


inline int logic_shift_right( int value, int shift ) {
    return value << shift;
}


inline int get_unsigned_bitfield( int value, int start_bit_no, int field_length ) {
    return (int) lowerBits( value >> start_bit_no, field_length );
}


inline int get_signed_bitfield( int value, int start_bit_no, int field_length ) {
    int result = get_unsigned_bitfield( value, start_bit_no, field_length );
    return isBitSet( result, start_bit_no + field_length - 1 ) ? addBits( result, ~nthMask( field_length ) ) : result;
}


inline int set_unsigned_bitfield( int value, int start_bit_no, int field_length, std::uint32_t new_field_value ) {
    st_assert( addBits( new_field_value, ~nthMask( field_length ) ) == 0, "range check" );
    int mask = nthMask( field_length ) << start_bit_no;
    return addBits( subBits( value, mask ), maskBits( new_field_value << start_bit_no, mask ) );
}


// -----------------------------------------------------------------------------

enum class oop_tags {
    oop_tag_int,    //
    oop_tag_mem,    //
    oop_tag_mark,   //
    num_tags,       //
};


// -----------------------------------------------------------------------------

template<std::uint32_t arg, std::uint32_t result = 0>
constexpr std::uint32_t mylog2 = mylog2<( arg >> 1 ), result + 1>;

template<std::uint32_t result>
constexpr std::uint32_t mylog2<1, result> = result;


// -----------------------------------------------------------------------------

template<std::uint32_t arg, std::uint32_t result = 1>
constexpr std::uint32_t myexp2 = myexp2<arg - 1, result << 1>;

template<std::uint32_t result>
constexpr std::uint32_t myexp2<0, result> = result;


// -----------------------------------------------------------------------------

template<typename TAG_TYPE, class T, std::uint32_t num_tags = mylog2<std::alignment_of<T>::value>>
class tagged_ptr_t {
public:
    tagged_ptr_t() noexcept
            : tagged_ptr_t( nullptr ) {
    }


    explicit tagged_ptr_t( T *p ) noexcept: ptr{ reinterpret_cast<std::uintptr_t>(p) } {
        static_assert( num_tags >= TAG_TYPE::num_tags, "enum defines too many flags" );
        assert( ( ptr & bitmask() ) == 0 );
    }


    tagged_ptr_t( tagged_ptr_t const & ) noexcept = default;
    tagged_ptr_t( tagged_ptr_t && ) noexcept = default;

    tagged_ptr_t &operator=( tagged_ptr_t const & ) = default;
    tagged_ptr_t &operator=( tagged_ptr_t && ) = default;

    ~tagged_ptr_t() noexcept = default;


    T &operator*() noexcept {
        return *reinterpret_cast<T *>(ptr & ~bitmask());
    }


    T const &operator*() const noexcept {
        return *reinterpret_cast<T const *>(ptr & ~bitmask());
    }


    T *operator->() noexcept {
        return reinterpret_cast<T *>(ptr & ~bitmask());
    }


    T const *operator->() const noexcept {
        return reinterpret_cast<T const *>(ptr & ~bitmask());
    }


    static constexpr std::uintptr_t bitmask() noexcept {
        return ( 1 << num_tags ) - 1;
    }


    template<TAG_TYPE t>
    bool check() const noexcept {
        return ptr & myexp2<static_cast<std::uint32_t>(t)>;
    }


    template<TAG_TYPE t>
    void set( bool v ) noexcept {
        constexpr auto i = static_cast<std::uint32_t>(t);

        if ( v ) {
            ptr |= ( 1 << i );
        } else {
            ptr &= ~( 1 << i );
        }
    }


private:
    std::uintptr_t ptr;
};


template<typename tag_type, typename T>
tagged_ptr_t<tag_type, T> make_tagged( T *ptr ) noexcept {
    return tagged_ptr_t<tag_type, T>{ ptr };
}


template<typename tag_type, typename T>
tagged_ptr_t<tag_type, T const> const make_tagged( T const *ptr ) noexcept {
    return tagged_ptr_t<tag_type, T const>{ ptr };
}
