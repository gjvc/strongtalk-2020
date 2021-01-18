
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

enum class Mode {
        //
        //			        3..9	10..16	17..31
        // mode/bits		3...................31	describes
        specialLoc,     //	--------id------------	sentinel values/global locations
        registerLoc,    //	--------regLoc--------	register locations
        stackLoc,       //	--------offset--------	stack locations
        contextLoc1,    //	ctxtNo	offset	scID	context locations during compilation (scope ID identifies InlinedScope)
        contextLoc2,    //	ctxtNo	offset	scOffs	context locations (scOffs is scope offset within encoded scopes)
        floatLoc,       //	0	floatNo	scopeN  float locations
};


class Location : public ResourceObject /* but usually used as ValueObj */ {
    private:
        int _loc;    // location encoding

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

        void overflow( Mode mode, int f1, int f2, int f3 );  // handle field overflow if possible

    public:

        Location() {
            specialLocation( 0 );
        }


        Location( int l ) {
            _loc = l;
        }


        Location( Mode mode, int f );

        Location( Mode mode, int f1, int f2, int f3 );


        // factory
        static Location specialLocation( int id ) {
            return Location( Mode::specialLoc, id );
        }


        static Location registerLocation( int number ) {
            return Location( Mode::registerLoc, number );
        }


        static Location stackLocation( int offset ) {
            return Location( Mode::stackLoc, offset );
        }


        static Location compiledContextLocation( int contextNo, int tempNo, int id ) {
            return Location( Mode::contextLoc1, contextNo, tempNo, id );
        }


        static Location runtimeContextLocation( int contextNo, int tempNo, int offs ) {
            return Location( Mode::contextLoc2, contextNo, tempNo, offs );
        }


        static Location floatLocation( int scopeNo, int tempNo ) {
            return Location( Mode::floatLoc, 0, tempNo, scopeNo );
        }


        // attributes
        Mode mode() const {
            return ( Mode ) ( _loc & ( ( 1 << _f1Pos ) - 1 ) );
        }


        int id() const {
            st_assert( mode() == Mode::specialLoc, "not a special location" );
            return ( _loc >> _fPos ) & _fMask;
        }


        int number() const {
            st_assert( mode() == Mode::registerLoc, "not a register location" );
            return ( _loc >> _fPos ) & _fMask;
        }


        int offset() const {
            st_assert( mode() == Mode::stackLoc, "not a stack location" );
            int t = _loc >> _fPos;
            return _loc < 0 ? ( t | ~_fMask ) : t;
        }


        int contextNo() const {
            st_assert( mode() == Mode::contextLoc1 or mode() == Mode::contextLoc2, "not a context location" );
            return ( _loc >> _f1Pos ) & _f1Mask;
        }


        int tempNo() const {
            st_assert( mode() == Mode::contextLoc1 or mode() == Mode::contextLoc2, "not a context location" );
            return ( _loc >> _f2Pos ) & _f2Mask;
        }


        int scopeID() const {
            st_assert( mode() == Mode::contextLoc1, "not a compiled context location" );
            return ( _loc >> _f3Pos ) & _f3Mask;
        }


        int scopeOffs() const {
            st_assert( mode() == Mode::contextLoc2, "not a runtime context location" );
            return ( _loc >> _f3Pos ) & _f3Mask;
        }


        int floatNo() const {
            st_assert( mode() == Mode::floatLoc, "not a float location" );
            return ( _loc >> _f2Pos ) & _f2Mask;
        }


        int scopeNo() const {
            st_assert( mode() == Mode::floatLoc, "not a float location" );
            return ( _loc >> _f3Pos ) & _f3Mask;
        }


        // helper functions
        const char * name() const;


        // predicates
        bool_t isSpecialLocation() const {
            return mode() == Mode::specialLoc;
        }


        bool_t isRegisterLocation() const {
            return mode() == Mode::registerLoc;
        }


        bool_t isStackLocation() const {
            return mode() == Mode::stackLoc;
        }


        bool_t isContextLocation() const {
            return mode() == Mode::contextLoc1 or mode() == Mode::contextLoc2;
        }


        bool_t isFloatLocation() const {
            return mode() == Mode::floatLoc;
        }


        bool_t isTopOfStack() const;


        bool_t equals( Location y ) const {
            return _loc == y._loc;
        }


        bool operator==( Location & rhs ) const {
            return rhs._loc == _loc;
        }


        bool operator==( const Location & rhs ) const {
            return rhs._loc == _loc;
        }


        bool operator!=( Location & rhs ) const {
            return rhs._loc != _loc;
        }


        bool operator!=( const Location & rhs ) const {
            return rhs._loc != _loc;
        }


        bool_t isTemporaryRegister() const;

        bool_t isTrashedRegister() const;

        bool_t isLocalRegister() const;

        // For packing and unpacking scope information
        friend class LocationName;

        friend class MemoizedName;

        friend class DebugInfoWriter;
};


// special locations
constexpr int  nofSpecialLocations    = 6;
const Location illegalLocation        = Location::specialLocation( 0 );
const Location unAllocated            = Location::specialLocation( 1 );
const Location noRegister             = Location::specialLocation( 2 );
const Location topOfStack             = Location::specialLocation( 3 );
const Location resultOfNonLocalReturn = Location::specialLocation( 4 );
const Location topOfFloatStack        = Location::specialLocation( 5 );    // only used if UseFPUStack is true


// An IntFreeList maintains a list of 'available' integers in the range [0, n[ where n is the maximum number of integers ever allocated.
// An IntFreeList may be used to allocate/release stack locations.

class IntFreeList : public PrintableResourceObject {

    protected:
        int _first;                     // the first available integer
        GrowableArray <int> * _list;    // the list
        std::vector <int> _vector;      //

        void grow();

    public:
        IntFreeList( int size );

        int allocate();         // returns a new integer, grows the list if necessary
        int allocated();        // returns the number of allocated integers
        void release( int i );  // marks the integer i as 'available' again
        int length();           // the maximum number of integers ever allocated
        void print();           //

};
