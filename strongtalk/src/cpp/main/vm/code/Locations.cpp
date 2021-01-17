//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/code/Locations.hpp"


// Implementation of Locations
//
// Each entry of _freeList corresponds to a register or stack location. If the location
// is used, the _freeList entry holds the negative reference count for that location. If
// the location is not used, the _freeList entry holds a link (an index) to the next un-
// used location or a sentinel value which indicates the end of the list. There are two
// free lists, one for registers and one for stack locations; the end of a list is indi-
// cated by a sentinel entry.
//
// _freeList->at(i) >= 0: unused, entry is index to next unused entry or sentinel
// _freeList->at(i) <  0: used, entry is negative reference count

Locations::Locations( int nofArgs, int nofRegs, int nofInitialStackTmps ) {
    st_assert( 0 <= nofArgs, "illegal number of arguments" );
    st_assert( 0 <= nofRegs and nofRegs <= maxNofUsableRegisters, "too many registers required" );
    _nofArguments = nofArgs;
    _nofRegisters = nofRegs;
    _freeList     = new GrowableArray <int>( nofArgs + nofRegs + nofInitialStackTmps );
    int i = 0;
    // initialize argument reference counts
    while ( i < nofArgs ) {
        _freeList->at_put_grow( i, 0 );
        i++;
    }
    // initialize register free list
    while ( i < nofArgs + nofRegs ) {
        _freeList->at_put_grow( i, i - 1 );
        i++;
    }
    _freeList->at_put( nofArgs, sentinel ); // end of list
    _firstFreeRegister = i - 1;
    // initialize stackTmps free list
    while ( i < nofArgs + nofRegs + nofInitialStackTmps ) {
        _freeList->at_put_grow( i, i - 1 );
        i++;
    }
    _freeList->at_put( nofArgs + nofRegs, sentinel ); // end of list
    _firstFreeStackTmp = i - 1;
    verify();
}


Locations::Locations( Locations * l ) {
    l->verify();
    _nofArguments      = l->_nofArguments;
    _nofRegisters      = l->_nofRegisters;
    _freeList          = l->_freeList->copy();
    _firstFreeRegister = l->_firstFreeRegister;
    _firstFreeStackTmp = l->_firstFreeStackTmp;
    verify();
}


void Locations::extendTo( int newValue ) {
    while ( this->nofStackTmps() < newValue ) {
        // grow _freeList
        _freeList->append( _firstFreeStackTmp );
        _firstFreeStackTmp = _freeList->length() - 1;
    }
    verify();
}


int Locations::allocateRegister() {
    int i = _firstFreeRegister;
    if ( not isRegister( i ) ) st_fatal( "out of registers" );
    _firstFreeRegister = _freeList->at( i );
    _freeList->at_put( i, -1 ); // initialize reference count
    verify();
    return i;
}


int Locations::allocateStackTmp() {
    int i              = _firstFreeStackTmp;
    if ( not isStackTmp( i ) ) {
        // grow _freeList
        _freeList->append( sentinel );
        st_assert( _freeList->length() <= sentinel, "sentinel too small" );
        i = _freeList->length() - 1;
    }
    _firstFreeStackTmp = _freeList->at( i );
    _freeList->at_put( i, -1 ); // initialize reference count
    verify();
    return i;
}


void Locations::allocate( int i ) {
    st_assert( isLocation( i ), "illegal location" );
    st_assert( nofUses( i ) == 0, "already allocated" );
    if ( isRegister( i ) ) {
        int j = _firstFreeRegister;
        if ( j == i ) {
            // remove first entry from free list
            _firstFreeRegister = _freeList->at( i );
        } else {
            // find i in the free list
            while ( _freeList->at( j ) not_eq i )
                j = _freeList->at( j );
            _freeList->at_put( j, _freeList->at( i ) );
        }
        _freeList->at_put( i, -1 ); // initialize reference count
    } else if ( isStackTmp( i ) ) {
        int j = _firstFreeStackTmp;
        if ( j == i ) {
            // remove first entry from free list
            _firstFreeStackTmp = _freeList->at( i );
        } else {
            // find i in the free list
            while ( _freeList->at( j ) not_eq i )
                j = _freeList->at( j );
            _freeList->at_put( j, _freeList->at( i ) );
        }
        _freeList->at_put( i, -1 ); // initialize reference count
    } else {
        ShouldNotReachHere();
    }
    verify();
}


void Locations::use( int i ) {
    st_assert( isLocation( i ), "illegal location" );
    st_assert( isArgument( i ) or nofUses( i ) > 0, "not yet allocated" );
    _freeList->at_put( i, _freeList->at( i ) - 1 ); // adjust reference counter
    verify();
}


void Locations::release( int i ) {
    st_assert( isLocation( i ), "illegal location" );
    st_assert( nofUses( i ) > 0, "not yet allocated" );
    _freeList->at_put( i, _freeList->at( i ) + 1 ); // adjust reference counter
    if ( _freeList->at( i ) == 0 ) {
        // not used anymore => recycle
        if ( isRegister( i ) ) {
            // register location
            _freeList->at_put( i, _firstFreeRegister );
            _firstFreeRegister = i;
        } else if ( isStackTmp( i ) ) {
            // stack location
            _freeList->at_put( i, _firstFreeStackTmp );
            _firstFreeStackTmp = i;
        }
    }
    verify();
}


int Locations::nofUses( int i ) const {
    st_assert( isLocation( i ), "illegal location" );
    int n = _freeList->at( i );
    return n < 0 ? -n : 0;
}


int Locations::nofTotalUses() const {
    int       totalUses = 0;
    for ( int i         = locationsBeg(); i < locationsEnd(); i++ )
        totalUses += nofUses( i );
    return totalUses;
}


int Locations::nofFreeRegisters() const {
    int i = _firstFreeRegister;
    int n = 0;
    while ( isRegister( i ) ) {
        i = _freeList->at( i );
        n++;
    }
    return n;
}


int Locations::freeRegisterMask() const {
    int       mask = 0;
    for ( int i    = registersBeg(); i < registersEnd(); i++ ) {
        if ( nofUses( i ) == 0 )
            mask |= 1 << locationAsRegister( i ).number();
    }
    return mask;
}


int Locations::usedRegisterMask() const {
    int       mask = 0;
    for ( int i    = registersBeg(); i < registersEnd(); i++ ) {
        if ( nofUses( i ) > 0 )
            mask |= 1 << locationAsRegister( i ).number();
    }
    return mask;
}


int Locations::argumentAsLocation( int argNo ) const {
    st_assert( 0 <= argNo and argNo < nofArguments(), "illegal argument no." );
    return argumentsBeg() + argNo;
}


int Locations::registerAsLocation( Register reg ) const {
    st_assert( maxNofUsableRegisters == 6, "inconsistency - adjust this code" );
    if ( reg == eax )
        return registersBeg() + 0;
    if ( reg == ebx )
        return registersBeg() + 1;
    if ( reg == ecx )
        return registersBeg() + 2;
    if ( reg == edx )
        return registersBeg() + 3;
    if ( reg == edi )
        return registersBeg() + 4;
    if ( reg == esi )
        return registersBeg() + 5;
    ShouldNotReachHere();
    return 0;
}


int Locations::temporaryAsLocation( int tempNo ) const {
    st_assert( 0 <= tempNo, "illegal temporary no." );
    return stackTmpsBeg() + tempNo;
}


Register Locations::locationAsRegister( int loc ) const {
    st_assert( isRegister( loc ), "location is not a register" );
    switch ( loc - registersBeg() ) {
        case 0:
            return eax;
        case 1:
            return ebx;
        case 2:
            return ecx;
        case 3:
            return edx;
        case 4:
            return edi;
        case 5:
            return esi;
        default: st_fatal( "inconsistency - adjust this code" );
    }
    ShouldNotReachHere();
    return eax;
}


// Stack frame
//
// |    ^   |
// |1st temp| <-- ebp - 1*oopSize (firstTemporaryOffset)
// |ebp save| <-- ebp
// |ret addr|
// |last arg| <-- ebp + 2*oopSize (lastParameterOffset)
// |        |

int Locations::locationAsWordOffset( int loc ) const {
    st_assert( isLocation( loc ), "illegal location" );
    if ( isArgument( loc ) ) {
        int lastParameterOffset = 2;
        return lastParameterOffset + ( nofArguments() - 1 - ( loc - argumentsBeg() ) );
    } else if ( isStackTmp( loc ) ) {
        constexpr int firstTemporaryOffset = -1;
        return firstTemporaryOffset - ( loc - stackTmpsBeg() );
    }
    ShouldNotReachHere();
    return 0;
}


void Locations::print() {
    int len = _freeList->length();
    int i;
    // print used locations
    _console->print_cr( "Locations:" );
    for ( int i = 0; i < len; i++ ) {
        if ( nofUses( i ) > 0 ) {
            _console->print_cr( "%d: %d uses", i, nofUses( i ) );
        }
    }
    _console->cr();
    // print whole free list
    _console->print_cr( "Free List:" );
    _console->print_cr( "no. of arguments    : %d", _nofArguments );
    _console->print_cr( "no. of registers    : %d", _nofRegisters );
    _console->print_cr( "first free register : %d", _firstFreeRegister );
    _console->print_cr( "first free stack loc: %d", _firstFreeStackTmp );
    for ( int i = 0; i < len; i++ ) {
        _console->print_cr( "%d: %d", i, _freeList->at( i ) );
    }
    _console->cr();
}


void Locations::verify() {
    if ( not CompilerDebug )
        return;
    int nofFreeRegisters = 0;
    int nofFreeStackTmps = 0;
    int nofUsedLocations = 0;
    int i;
    // verify arguments reference counts
    i = 0;
    while ( i < _nofArguments ) {
        if ( _freeList->at( i ) > 0 ) st_fatal( "bug in argument reference counts" );
        i++;
    }
    // verify register free list
    i = _firstFreeRegister;
    while ( i not_eq sentinel ) {
        if ( not isRegister( i ) ) st_fatal( "bug in registers free list" );
        nofFreeRegisters++;
        i = _freeList->at( i );
    }
    if ( nofFreeRegisters > _nofRegisters ) st_fatal( "too many free registers" );
    // verify stack locs free list
    i = _firstFreeStackTmp;
    while ( i not_eq sentinel ) {
        if ( not isStackTmp( i ) ) st_fatal( "bug in stack locs free list" );
        nofFreeStackTmps++;
        i = _freeList->at( i );
    }
    if ( nofFreeStackTmps > _freeList->length() - _nofRegisters - _nofArguments ) st_fatal( "too many free stack locs" );
    // verify used locations
    i = _freeList->length();
    while ( i-- > _nofArguments ) {
        if ( _freeList->at( i ) < 0 )
            nofUsedLocations++;
    }
    // verify total number
    if ( _nofArguments + nofFreeRegisters + nofFreeStackTmps + nofUsedLocations not_eq _freeList->length() ) st_fatal( "locations data structure is leaking" );
}
