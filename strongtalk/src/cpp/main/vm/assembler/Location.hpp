
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"

// Locations serve as abstractions for physical addresses.
// For each physical location (register, stack position or context temporary), there is a corresponding location and vice versa.

enum class LocationMode {

    //                                    1         2         3
    //                          01234567890123456789012345678901
    //
    //			                3..9	10..16	17..31
    // mode/bits		        3...................31	describes
    SPECIAL_LOCATION,     //	--------id------------	sentinel values/global locations
    REGISTER_LOCATION,    //	--------regLoc--------	register locations
    STACK_LOCATION,       //	--------offset--------	stack locations
    CONTEXT_LOCATIION_1,  //	ctxtNo	offset	scID	context locations during compilation (scope ID identifies InlinedScope)
    CONTEXT_LOCATIION_2,  //	ctxtNo	offset	scOffs	context locations (scOffs is scope offset within encoded scopes)
    FLOAT_LOCATION,       //	0	floatNo	scopeN  float locations

};


class Location : public ResourceObject /* but usually used as ValueObject */ {
private:
    std::int32_t _loc;    // location encoding

    // field layout of _loc (no local const definitions allowed in C++)
    enum {
        _f1Pos  = 3, //
        _f1Len  = 7, //
        _f1Mask = ( 1 << _f1Len ) - 1, //

        _f2Pos  = _f1Pos + _f1Len, //
        _f2Len  = 7, //
        _f2Mask = ( 1 << _f2Len ) - 1, //

        _f3Pos  = _f2Pos + _f2Len, //
        _f3Len  = 15, //
        _f3Mask = ( 1 << _f3Len ) - 1, //

        _fPos  = _f1Pos, //
        _fLen  = 29, //
        _fMask = ( 1 << _fLen ) - 1,
    };

    void overflow( LocationMode mode, std::int32_t f1, std::int32_t f2, std::int32_t f3 );  // handle field overflow if possible

public:

    Location() :
        _loc{ 0 } {
        specialLocation( 0 );
    }


    Location( std::int32_t l ) :
        _loc{ l } {
    }


    Location( LocationMode mode, std::int32_t f );

    Location( LocationMode mode, std::int32_t f1, std::int32_t f2, std::int32_t f3 );


    // factory
    static Location specialLocation( std::int32_t id ) {
        return Location( LocationMode::SPECIAL_LOCATION, id );
    }


    static Location registerLocation( std::int32_t number ) {
        return Location( LocationMode::REGISTER_LOCATION, number );
    }


    static Location stackLocation( std::int32_t offset ) {
        return Location( LocationMode::STACK_LOCATION, offset );
    }


    static Location compiledContextLocation( std::int32_t contextNo, std::int32_t tempNo, std::int32_t id ) {
        return Location( LocationMode::CONTEXT_LOCATIION_1, contextNo, tempNo, id );
    }


    static Location runtimeContextLocation( std::int32_t contextNo, std::int32_t tempNo, std::int32_t offs ) {
        return Location( LocationMode::CONTEXT_LOCATIION_2, contextNo, tempNo, offs );
    }


    static Location floatLocation( std::int32_t scopeNo, std::int32_t tempNo ) {
        return Location( LocationMode::FLOAT_LOCATION, 0, tempNo, scopeNo );
    }


    // attributes
    LocationMode mode() const {
        return (LocationMode) ( _loc & ( ( 1 << _f1Pos ) - 1 ) );
    }


    std::int32_t id() const {
        st_assert( mode() == LocationMode::SPECIAL_LOCATION, "not a special location" );
        return ( _loc >> _fPos ) & _fMask;
    }


    std::int32_t number() const {
        st_assert( mode() == LocationMode::REGISTER_LOCATION, "not a register location" );
        return ( _loc >> _fPos ) & _fMask;
    }


    std::int32_t offset() const {
        st_assert( mode() == LocationMode::STACK_LOCATION, "not a stack location" );
        std::int32_t t = _loc >> _fPos;
        return _loc < 0 ? ( t | ~_fMask ) : t;
    }


    std::int32_t contextNo() const {
        st_assert( mode() == LocationMode::CONTEXT_LOCATIION_1 or mode() == LocationMode::CONTEXT_LOCATIION_2, "not a context location" );
        return ( _loc >> _f1Pos ) & _f1Mask;
    }


    std::int32_t tempNo() const {
        st_assert( mode() == LocationMode::CONTEXT_LOCATIION_1 or mode() == LocationMode::CONTEXT_LOCATIION_2, "not a context location" );
        return ( _loc >> _f2Pos ) & _f2Mask;
    }


    std::int32_t scopeID() const {
        st_assert( mode() == LocationMode::CONTEXT_LOCATIION_1, "not a compiled context location" );
        return ( _loc >> _f3Pos ) & _f3Mask;
    }


    std::int32_t scopeOffs() const {
        st_assert( mode() == LocationMode::CONTEXT_LOCATIION_2, "not a runtime context location" );
        return ( _loc >> _f3Pos ) & _f3Mask;
    }


    std::int32_t floatNo() const {
        st_assert( mode() == LocationMode::FLOAT_LOCATION, "not a float location" );
        return ( _loc >> _f2Pos ) & _f2Mask;
    }


    std::int32_t scopeNo() const {
        st_assert( mode() == LocationMode::FLOAT_LOCATION, "not a float location" );
        return ( _loc >> _f3Pos ) & _f3Mask;
    }


    // helper functions
    const char *name() const;


    // predicates
    bool isSpecialLocation() const {
        return mode() == LocationMode::SPECIAL_LOCATION;
    }


    bool isRegisterLocation() const {
        return mode() == LocationMode::REGISTER_LOCATION;
    }


    bool isStackLocation() const {
        return mode() == LocationMode::STACK_LOCATION;
    }


    bool isContextLocation() const {
        return mode() == LocationMode::CONTEXT_LOCATIION_1 or mode() == LocationMode::CONTEXT_LOCATIION_2;
    }


    bool isFloatLocation() const {
        return mode() == LocationMode::FLOAT_LOCATION;
    }


    bool isTopOfStack() const;


    bool equals( Location y ) const {
        return _loc == y._loc;
    }


    bool operator==( Location &rhs ) const {
        return rhs._loc == _loc;
    }


    bool operator==( const Location &rhs ) const {
        return rhs._loc == _loc;
    }


    bool operator!=( Location &rhs ) const {
        return rhs._loc != _loc;
    }


    bool operator!=( const Location &rhs ) const {
        return rhs._loc != _loc;
    }


    bool isTemporaryRegister() const;

    bool isTrashedRegister() const;

    bool isLocalRegister() const;

    // For packing and unpacking scope information
    friend class LocationName;

    friend class MemoizedName;

    friend class DebugInfoWriter;

    //
    static Location ILLEGAL_LOCATION;
    static Location UNALLOCATED_LOCATION;
    static Location NO_REGISTER;
    static Location TOP_OF_STACK;
    static Location RESULT_OF_NON_LOCAL_RETURN;
    static Location TOP_OF_FLOAT_STACK;

};


// An IntegerFreeList maintains a list of 'available' integers in the range [0, n[ where n is the maximum number of integers ever allocated.
// An IntegerFreeList may be used to allocate/release stack locations.

class IntegerFreeList : public PrintableResourceObject {

protected:
    std::int32_t                _first;     // the first available integer
    GrowableArray<std::int32_t> *_list;     // the list
    std::vector<std::int32_t>   _vector;    //

    void grow();

public:
    IntegerFreeList( std::int32_t size );

    std::int32_t allocate();        // returns a new integer, grows the list if necessary
    std::int32_t allocated();       // returns the number of allocated integers
    void release( std::int32_t i ); // marks the integer i as 'available' again
    std::int32_t length();          // the maximum number of integers ever allocated
    void print();                   //

};
