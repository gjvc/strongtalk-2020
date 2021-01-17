//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/memory/Array.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/code/PseudoRegisterMapping.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/utilities/disassembler.hpp"
#include "vm/code/MapConformance.hpp"

extern Compiler * theCompiler;


//
// Implementation of PRegMapping
//
// The mapping is done via a simple dictionary implemented via the _pregs array holding
// the keys (the pregs), and the _reg_map & _stack_map arrays holding the values (the
// location indices). Note: A preg may be mapped to a register (via _reg_map & and a
// stack location at the same time). An unused dictionary entry is marked by a nullptr
// _pregs entry. An unused map entry is marked by a number < 0.
//
// Invariant: A particular preg exists only once in the _pregs array; it has at least
//            one and at most two locations (a register and a stack location).
//
// Note: A future implementation might use little "mapping objects" instead if these
//       three arrays that are used right now. Likely to speed up the implementation
//       (at() and at_put() are not cheap compared to indirection ->. See class Entry).
//
//

int PseudoRegisterMapping::index( PseudoRegister * preg ) {
    st_assert( preg not_eq nullptr, "no preg specified" );
    // try cashed index first
    int i = preg->_map_index_cache;
    st_assert( 0 <= i, "_map_index_cache must always be > 0" );
    if ( i < size() and _pseudoRegisters->at( i ) == preg )
        return i;
    // otherwise search for it
    i = size();
    while ( i-- > 0 and _pseudoRegisters->at( i ) not_eq preg );
    // (-1 <= i < size() and (i >= 0 => _pregs->at(i) == preg)
    // if found, set cash for next time
    if ( i >= 0 )
        preg->_map_index_cache = i;
    return i;
}


void PseudoRegisterMapping::set_entry( int i, PseudoRegister * preg, int rloc, int sloc ) {
    st_assert( preg not_eq nullptr, "no preg specified" );
    st_assert( not _locations->isLocation( rloc ) or _locations->isRegister( rloc ), "should be a register location if at all" );
    st_assert( not _locations->isLocation( sloc ) or _locations->isArgument( sloc ) or _locations->isStackTmp( sloc ), "should be a stack location if at all" );
    st_assert( _locations->isLocation( rloc ) or _locations->isLocation( sloc ), "at least one location expected" );
    _pseudoRegisters->at_put( i, preg );
    _registerLocations->at_put( i, rloc );
    _stackLocations->at_put( i, sloc );
    // set cash for next time
    preg->_map_index_cache = i;
}


int PseudoRegisterMapping::freeSlot() {
    // search for an unused slot
    int i = size();
    while ( i-- > 0 and used( i ) );
    // (-1 <= i < size()) and (i >= 0 => not used(i))
    if ( i < 0 ) {
        // no free slot => grow arrays
        _pseudoRegisters->append( nullptr );
        _registerLocations->append( -1 );
        _stackLocations->append( -1 );
        i = size() - 1;
    }
    st_assert( not used( i ), "just checkin'..." );
    return i;
}


int PseudoRegisterMapping::spillablePRegIndex() {
    // Finds a PseudoRegister that is mapped to a register location and that is not locked.
    // Returns a value < 0 if there's no such PseudoRegister.
    int uses = 999999999;
    int i0   = -1;
    int i    = size();
    while ( i-- > 0 ) {
        PseudoRegister * preg = _pseudoRegisters->at( i );
        int rloc = regLoc( i );
        if ( preg not_eq nullptr and _locations->isRegister( rloc ) and not PseudoRegisterLocker::locks( preg ) and _locations->nofUses( rloc ) < uses ) {
            uses = _locations->nofUses( rloc );
            i0   = i;
        }
    }
    return i0;
}


void PseudoRegisterMapping::ensureOneFreeRegister() {
    if ( not _locations->freeRegisters() ) {
        // no free registers available => find a register to spill
        int i = spillablePRegIndex();
        if ( i < 0 ) st_fatal( "too many temporaries or locked pregs: out of spillable registers" );
        // _console->print("WARNING: Register spilling - check if this works\n");
        spillRegister( regLoc( i ) );
        st_assert( _locations->freeRegisters(), "at least one register should be available now" );
        verify();
    }
}


void PseudoRegisterMapping::spillRegister( int loc ) {
    st_assert( _locations->isRegister( loc ), "must be a register" );
    int spillLoc = _locations->allocateStackTmp();
    // remap pregs
    int i        = size();
    while ( i-- > 0 ) {
        if ( used( i ) and regLoc( i ) == loc ) {
            _locations->release( loc );
            _registerLocations->at_put( i, _locations->noLocation );
            if ( not hasStkLoc( i ) ) {
                _locations->use( spillLoc );
                _stackLocations->at_put( i, spillLoc );
            }
        }
    }
    _locations->release( spillLoc );
    st_assert( _locations->nofUses( loc ) == 0, "location should not be used anymore" );
    // generate spill code
    Register reg = _locations->locationAsRegister( loc );
    _macroAssembler->movl( _locations->locationAsAddress( spillLoc ), reg );
    verify();
}


int PseudoRegisterMapping::allocateTemporary( Register hint ) {
    ensureOneFreeRegister();
    int regLoc = _locations->noLocation;
    if ( hint not_eq noreg ) {
        // try to use hint register
        int hintLoc = _locations->registerAsLocation( hint );
        if ( _locations->nofUses( hintLoc ) == 0 ) {
            // hintLoc is available
            _locations->allocate( hintLoc );
            regLoc = hintLoc;
        }
    }
    if ( not _locations->isLocation( regLoc ) )
        regLoc = _locations->allocateRegister();
    st_assert( _locations->isLocation( regLoc ) and _locations->nofUses( regLoc ) == 1, "should be allocated exactly once" );
    _temporaryLocations->push( regLoc );
    return regLoc;
}


void PseudoRegisterMapping::releaseTemporary( int regLoc ) {
    if ( _temporaryLocations->pop() == regLoc ) {
        _locations->release( regLoc );
    } else {
        // temporary not freed in stack-fashioned manner
        // (can only happen if there's more than one temporary per function)
        Unimplemented();
    }
}


void PseudoRegisterMapping::releaseAllTemporaries() {
    while ( _temporaryLocations->nonEmpty() )
        _locations->release( _temporaryLocations->pop() );
}


void PseudoRegisterMapping::destroy() {
    _pseudoRegisters = nullptr;
}


PseudoRegisterMapping::PseudoRegisterMapping( MacroAssembler * assm, int nofArgs, int nofRegs, int nofTemps ) {
    constexpr int initialSize = 8;
    _macroAssembler           = assm;
    _nonLocalReturnInProgress = false;
    _locations                = new Locations( nofArgs, nofRegs, nofTemps );
    _pseudoRegisters          = new GrowableArray <PseudoRegister *>( initialSize );
    _registerLocations        = new GrowableArray <int>( initialSize );
    _stackLocations           = new GrowableArray <int>( initialSize );
    _temporaryLocations       = new GrowableArray <int>( 2 );
    verify();
}


PseudoRegisterMapping::PseudoRegisterMapping( PseudoRegisterMapping * m ) {
    _macroAssembler           = m->_macroAssembler;
    _nonLocalReturnInProgress = m->_nonLocalReturnInProgress;
    _locations                = new Locations( m->_locations );
    _pseudoRegisters          = m->_pseudoRegisters->copy();
    _registerLocations        = m->_registerLocations->copy();
    _stackLocations           = m->_stackLocations->copy();
    _temporaryLocations       = m->_temporaryLocations->copy();
    releaseAllTemporaries();
    verify();
}


bool_t PseudoRegisterMapping::isInjective() {
    int i = size();
    while ( i-- > 0 ) {
        if ( used( i ) ) {
            if ( hasRegLoc( i ) and _locations->nofUses( regLoc( i ) ) > 1 )
                return false;
            if ( hasStkLoc( i ) and _locations->nofUses( stkLoc( i ) ) > 1 )
                return false;
        }
    }
    return true;
}


bool_t PseudoRegisterMapping::isConformant( PseudoRegisterMapping * with ) {
    // checks conformity on the intersection of this and with
    if ( NonLocalReturninProgress() not_eq with->NonLocalReturninProgress() )
        return false;
    int j = with->size();
    while ( j-- > 0 ) {
        if ( with->used( j ) ) {
            int i = index( with->_pseudoRegisters->at( j ) );
            if ( i >= 0 ) {
                if ( regLoc( i ) not_eq with->regLoc( j ) or stkLoc( i ) not_eq with->stkLoc( j ) )
                    return false;
            }
        }
    }
    return true;
}


void PseudoRegisterMapping::mapToArgument( PseudoRegister * preg, int argNo ) {
    st_assert( index( preg ) < 0, "preg for argument defined twice" );
    int loc = _locations->argumentAsLocation( argNo );
    _locations->use( loc );
    set_entry( freeSlot(), preg, _locations->noLocation, loc );
    verify();
}


void PseudoRegisterMapping::mapToTemporary( PseudoRegister * preg, int tempNo ) {
    st_assert( index( preg ) < 0, "preg for argument defined twice" );
    int loc = _locations->temporaryAsLocation( tempNo );
    _locations->allocate( loc );
    set_entry( freeSlot(), preg, _locations->noLocation, loc );
    verify();
}


void PseudoRegisterMapping::mapToRegister( PseudoRegister * preg, Register reg ) {
    st_assert( index( preg ) < 0, "preg for register defined twice" );
    int loc = _locations->registerAsLocation( reg );
    _locations->allocate( loc );
    set_entry( freeSlot(), preg, loc, _locations->noLocation );
    verify();
}


void PseudoRegisterMapping::kill( PseudoRegister * preg ) {
    int i = index( preg );
    if ( i >= 0 ) {
        if ( PrintPRegMapping ) {
            _console->print( "kill " );
            print( i );
        }
        int rloc = regLoc( i );
        int sloc = stkLoc( i );
        if ( _locations->isLocation( rloc ) )
            _locations->release( rloc );
        if ( _locations->isLocation( sloc ) )
            _locations->release( sloc );
        _pseudoRegisters->at_put( i, nullptr );
        verify();
    }
}


void PseudoRegisterMapping::killAll( PseudoRegister * exception ) {
    int i = size();
    while ( i-- > 0 ) {
        if ( used( i ) and _pseudoRegisters->at( i ) not_eq exception ) {
            st_assert( not PseudoRegisterLocker::locks( _pseudoRegisters->at( i ) ), "PseudoRegister is locked" );
            if ( PrintPRegMapping ) {
                _console->print( "kill " );
                print( i );
            }
            int rloc = regLoc( i );
            int sloc = stkLoc( i );
            if ( _locations->isLocation( rloc ) )
                _locations->release( rloc );
            if ( _locations->isLocation( sloc ) )
                _locations->release( sloc );
            _pseudoRegisters->at_put( i, nullptr );
        }
    }
    verify();
}


void PseudoRegisterMapping::killDeadsAt( Node * node, PseudoRegister * exception ) {
    while ( node->isTrivial() or node->isMergeNode() )
        node = node->next();
    // In case of a ReturnNode resultPR & scope context are needed
    // -> maybe use a better solution?
    //
    // if (node->isReturnNode()) {
    //   // end of method, only result needed
    //   killAll(node->scope()->resultPR);
    // }
    st_assert( node->id() >= 0, "should not be a comment" );
    int i = size();
    while ( i-- > 0 ) {
        PseudoRegister * preg = _pseudoRegisters->at( i );
        if ( preg not_eq nullptr and preg not_eq exception and ( not preg->isLiveAt( node ) or preg->isConstPseudoRegister() ) )
            kill( preg );
    }
}


void PseudoRegisterMapping::cleanupContextReferences() {
    int i = size();
    while ( i-- > 0 ) {
        PseudoRegister * preg = _pseudoRegisters->at( i );
        if ( preg not_eq nullptr and preg->_location.isContextLocation() ) {
            // refers to a context temporary -> kill it
            kill( preg );
        }
    }
}


// Note: Right now, if there's a hint register given, def() and use() make sure that the
// preg will be mapped to the hint register; and hint must be unallocated. This is not the
// same behaviour as in Temporary, where the hint register is only used if it is actually
// available. Furthermore, this behaviour seems inconsistent somehow, because if the preg
// is already in the hint register it will be used even though it is not actually available
// (kind of academic subtlety).

Register PseudoRegisterMapping::def( PseudoRegister * preg, Register hint ) {
    st_assert( not preg->isSinglyAssignedPseudoRegister() or index( preg ) < 0, "SinglyAssignedPseudoRegisters can be defined only once" );
    int i = index( preg );
    st_assert( i < 0 or not hasStkLoc( i ) or not _locations->isArgument( stkLoc( i ) ), "cannot assign to parameters" );
    kill( preg );
    int loc;
    if ( hint == noreg ) {
        ensureOneFreeRegister();
        loc  = _locations->allocateRegister();
        hint = _locations->locationAsRegister( loc );
    } else {
        loc = _locations->registerAsLocation( hint );
        _locations->allocate( loc );
    }
    set_entry( freeSlot(), preg, loc, _locations->noLocation );
    verify();
    return hint;
}


Register PseudoRegisterMapping::use( PseudoRegister * preg, Register hint ) {
    int i = index( preg );
    if ( i < 0 and preg->_location.isContextLocation() ) {
        // preg refers to context temporary
        // determine context temporary address
        PseudoRegister * context  = theCompiler->contextList->at( preg->_location.contextNo() )->context();
        PseudoRegisterLocker lock( context );
        Address              addr = Address( use( context ), Mapping::contextOffset( preg->_location.tempNo() ) );
        // determine a target register
        int                  loc;
        if ( hint == noreg ) {
            ensureOneFreeRegister();
            loc  = _locations->allocateRegister();
            hint = _locations->locationAsRegister( loc );
        } else {
            loc = _locations->registerAsLocation( hint );
            _locations->allocate( loc );
        }
        _macroAssembler->movl( hint, addr );
        set_entry( freeSlot(), preg, loc, _locations->noLocation );
    }
    i     = index( preg );
    st_assert( i >= 0, "preg must have been defined" )
    if ( hasRegLoc( i ) ) {
        Register old = _locations->locationAsRegister( regLoc( i ) );
        if ( hint == noreg or hint == old ) {
            hint = old;
        } else {
            int loc = _locations->registerAsLocation( hint );
            _locations->release( regLoc( i ) );
            _locations->allocate( loc );
            _registerLocations->at_put( i, loc );
            _macroAssembler->movl( hint, old );
        }
    } else {
        // copy into register
        st_assert( hasStkLoc( i ), "must have at least one location" );
        int loc;
        if ( hint == noreg ) {
            ensureOneFreeRegister();
            loc  = _locations->allocateRegister();
            hint = _locations->locationAsRegister( loc );
        } else {
            loc = _locations->registerAsLocation( hint );
            _locations->allocate( loc );
        }
        _registerLocations->at_put( i, loc );
        _macroAssembler->movl( hint, _locations->locationAsAddress( stkLoc( i ) ) );
    }
    verify();
    return hint;
}


Register PseudoRegisterMapping::use( PseudoRegister * preg ) {
    Register reg;
    if ( preg->isConstPseudoRegister() and not isDefined( preg ) ) {
        reg = def( preg );
        _macroAssembler->movl( reg, ( ( ConstPseudoRegister * ) preg )->constant );
    } else {
        reg = use( preg, noreg );
    }
    return reg;
}


void PseudoRegisterMapping::move( PseudoRegister * dst, PseudoRegister * src ) {
    st_assert( dst->_location not_eq topOfStack, "parameter passing cannot be handled here" );
    kill( dst ); // remove any previous definition
    int i = index( src );
    st_assert( i >= 0, "src must be defined" );
    int rloc = regLoc( i );
    int sloc = stkLoc( i );
    if ( _locations->isLocation( rloc ) )
        _locations->use( rloc );
    if ( _locations->isLocation( sloc ) )
        _locations->use( sloc );
    set_entry( freeSlot(), dst, rloc, sloc );
    verify();
}


void PseudoRegisterMapping::saveRegister( int loc ) {
    st_assert( _locations->isRegister( loc ), "must be a register" );
    int spillLoc = _locations->allocateStackTmp();
    // remap pregs
    int i        = size();
    while ( i-- > 0 ) {
        if ( used( i ) and regLoc( i ) == loc ) {
            if ( not hasStkLoc( i ) ) {
                _locations->use( spillLoc );
                _stackLocations->at_put( i, spillLoc );
            }
        }
    }
    _locations->release( spillLoc );
    // generate spill code
    _macroAssembler->movl( _locations->locationAsAddress( spillLoc ), _locations->locationAsRegister( loc ) );
    verify();
}


void PseudoRegisterMapping::saveRegisters( PseudoRegister * exception ) {
    int i = size();
    while ( i-- > 0 ) {
        if ( used( i ) and hasRegLoc( i ) and not hasStkLoc( i ) and _pseudoRegisters->at( i ) not_eq exception ) {
            saveRegister( regLoc( i ) );
        }
    }
    verify();
}


void PseudoRegisterMapping::killRegisters( PseudoRegister * exception ) {
    int i = size();
    while ( i-- > 0 ) {
        if ( used( i ) and hasRegLoc( i ) and _pseudoRegisters->at( i ) not_eq exception ) {
            // remove that register from mapping & release it
            _locations->release( regLoc( i ) );
            _registerLocations->at_put( i, _locations->noLocation );
            if ( not hasStkLoc( i ) ) {
                // remove entry for preg alltogether
                _pseudoRegisters->at_put( i, nullptr );
            }
        }
    }
    verify();
}


void PseudoRegisterMapping::killRegister( PseudoRegister * preg ) {
    int i = index( preg );
    st_assert( i >= 0, "preg must have been defined" )
    if ( hasRegLoc( i ) ) {
        // remove that register from mapping & release it
        _locations->release( regLoc( i ) );
        _registerLocations->at_put( i, _locations->noLocation );
        if ( not hasStkLoc( i ) ) {
            // remove entry for preg alltogether
            _pseudoRegisters->at_put( i, nullptr );
        }
    }
    verify();
}


void PseudoRegisterMapping::acquireNonLocalReturnRegisters() {
    guarantee( not NonLocalReturninProgress(), "no NonLocalReturn must be in progress" );
    _nonLocalReturnInProgress = true;
    _locations->allocate( _locations->registerAsLocation( NonLocalReturn_result_reg ) );
    _locations->allocate( _locations->registerAsLocation( NonLocalReturn_home_reg ) );
    _locations->allocate( _locations->registerAsLocation( NonLocalReturn_homeId_reg ) );
}


void PseudoRegisterMapping::releaseNonLocalReturnRegisters() {
    guarantee( NonLocalReturninProgress(), "NonLocalReturn must be in progress" );
    _nonLocalReturnInProgress = false;
    _locations->release( _locations->registerAsLocation( NonLocalReturn_result_reg ) );
    _locations->release( _locations->registerAsLocation( NonLocalReturn_home_reg ) );
    _locations->release( _locations->registerAsLocation( NonLocalReturn_homeId_reg ) );
}


void PseudoRegisterMapping::makeInjective() {
    // Note: This routine must not generate any code that modifies CPU flags!
    int i = size();
    while ( i-- > 0 ) {
        if ( used( i ) ) {
            int rloc = regLoc( i );
            int sloc = stkLoc( i );
            st_assert( _locations->isLocation( rloc ) or _locations->isLocation( sloc ), "must have at least one location" );
            if ( _locations->isLocation( rloc ) and _locations->isLocation( sloc ) ) {
                if ( _locations->nofUses( rloc ) > 1 and _locations->nofUses( sloc ) > 1 ) {
                    // preg is mapped to both register and stack location that are shared with other pregs
                    // => map to a new stack location
                    int newLoc = _locations->allocateStackTmp();
                    _macroAssembler->movl( _locations->locationAsAddress( newLoc ), _locations->locationAsRegister( rloc ) );
                    _locations->release( rloc );
                    _locations->release( sloc );
                    _registerLocations->at_put( i, _locations->noLocation );
                    _stackLocations->at_put( i, newLoc );
                } else if ( _locations->nofUses( rloc ) > 1 ) {
                    // preg mapped to a register that is shared with other pregs => use stack location only
                    _locations->release( rloc );
                    _registerLocations->at_put( i, _locations->noLocation );
                } else if ( _locations->nofUses( sloc ) > 1 ) {
                    // preg mapped to a stack location that is shared with other pregs => use register location only
                    _locations->release( sloc );
                    _stackLocations->at_put( i, _locations->noLocation );
                }
            } else if ( _locations->isLocation( rloc ) ) {
                if ( _locations->nofUses( rloc ) > 1 ) {
                    // preg is mapped to a register that shared with other pregs => map to a new stack location
                    int newLoc = _locations->allocateStackTmp();
                    _macroAssembler->movl( _locations->locationAsAddress( newLoc ), _locations->locationAsRegister( rloc ) );
                    _locations->release( rloc );
                    _registerLocations->at_put( i, _locations->noLocation );
                    _stackLocations->at_put( i, newLoc );
                }
            } else if ( _locations->isLocation( sloc ) ) {
                if ( _locations->nofUses( sloc ) > 1 ) {
                    // preg is mapped to a stack location that is shared with other pregs => map to a new stack location
                    ensureOneFreeRegister();
                    int      tmpLoc = _locations->allocateRegister();
                    int      newLoc = _locations->allocateStackTmp();
                    Register t      = _locations->locationAsRegister( tmpLoc );
                    _macroAssembler->movl( t, _locations->locationAsAddress( sloc ) );
                    _macroAssembler->movl( _locations->locationAsAddress( newLoc ), t );
                    _locations->release( tmpLoc );
                    _locations->release( sloc );
                    _registerLocations->at_put( i, _locations->noLocation );
                    _stackLocations->at_put( i, newLoc );
                }
            } else {
                ShouldNotReachHere();
            }
        }
    }
    verify();
    st_assert( isInjective(), "mapping not injective" );
}


void PseudoRegisterMapping::old_makeConformant( PseudoRegisterMapping * with ) {

    int j = with->size();

    // determine which entries have to be adjusted (save values on the stack)

    const char * begin_of_code = _macroAssembler->pc();

    GrowableArray <int> src( 4 );
    GrowableArray <int> dst( 4 );


    while ( j-- > 0 ) {
        if ( with->used( j ) ) {
            int i = index( with->_pseudoRegisters->at( j ) );
            if ( i >= 0 ) {
                st_assert( _pseudoRegisters->at( i ) == with->_pseudoRegisters->at( j ), "should be the same" );
                if ( regLoc( i ) not_eq with->regLoc( j ) or stkLoc( i ) not_eq with->stkLoc( j ) ) {
                    // push value if necessary
                    // (not necessary if one of the locations is conformant)
                    if ( hasRegLoc( i ) and regLoc( i ) == with->regLoc( j ) ) {
                        // register locations conform => not necessary to save a value
                        if ( hasStkLoc( i ) ) {
                            st_assert( stkLoc( i ) not_eq with->stkLoc( j ), "error in program logic" );
                            _locations->release( stkLoc( i ) );
                        }
                    } else if ( hasStkLoc( i ) and stkLoc( i ) == with->stkLoc( j ) ) {
                        // stack locations conform => not necessary to save a value
                        if ( hasRegLoc( i ) ) {
                            st_assert( regLoc( i ) not_eq with->regLoc( j ), "error in program logic" );
                            _locations->release( regLoc( i ) );
                        }
                    } else {
                        // none of the locations conform => push value
                        if ( hasRegLoc( i ) ) {
                            _macroAssembler->pushl( _locations->locationAsRegister( regLoc( i ) ) );
                        } else {
                            st_assert( hasStkLoc( i ), "must have at least one location" );
                            _macroAssembler->pushl( _locations->locationAsAddress( stkLoc( i ) ) );
                        }
                        // free allocated locations
                        if ( hasRegLoc( i ) )
                            _locations->release( regLoc( i ) );
                        if ( hasStkLoc( i ) )
                            _locations->release( stkLoc( i ) );
                    }
                    // mapping differs for this preg => remember entries
                    src.push( i );
                    dst.push( j );
                }
            }
        }
    }

    // pop values from stack and adjust entries
    while ( src.nonEmpty() ) {
        int i = src.pop();
        int j = dst.pop();
        if ( hasRegLoc( i ) and regLoc( i ) == with->regLoc( j ) ) {
            // register locations conform => must have non-conformant stack location
            st_assert( stkLoc( i ) not_eq with->stkLoc( j ), "error in program logic" );
            if ( with->hasStkLoc( j ) ) {
                _macroAssembler->movl( with->_locations->locationAsAddress( with->stkLoc( j ) ), _locations->locationAsRegister( regLoc( i ) ) );
                _locations->allocate( with->stkLoc( j ) );
            }
        } else if ( hasStkLoc( i ) and stkLoc( i ) == with->stkLoc( j ) ) {
            // stack locations conform => must have non-conformant register location
            st_assert( regLoc( i ) not_eq with->regLoc( j ), "error in program logic" );
            if ( with->hasRegLoc( j ) ) {
                _macroAssembler->movl( with->_locations->locationAsRegister( with->regLoc( j ) ), _locations->locationAsAddress( stkLoc( i ) ) );
                _locations->allocate( with->regLoc( j ) );
            }
        } else {
            // none of the locations conform => pop value
            if ( with->hasRegLoc( j ) ) {
                Register reg = with->_locations->locationAsRegister( with->regLoc( j ) );
                _macroAssembler->popl( reg );
                if ( with->hasStkLoc( j ) ) {
                    // copy register on stack
                    _macroAssembler->movl( with->_locations->locationAsAddress( with->stkLoc( j ) ), reg );
                }
            } else {
                st_assert( with->hasStkLoc( j ), "must have at least one location" );
                _macroAssembler->popl( with->_locations->locationAsAddress( with->stkLoc( j ) ) );
            }
            // allocate locations
            if ( with->hasRegLoc( j ) )
                _locations->allocate( with->regLoc( j ) );
            if ( with->hasStkLoc( j ) )
                _locations->allocate( with->stkLoc( j ) );
        }
        // adjust mapping
        _registerLocations->at_put( i, with->regLoc( j ) );
        _stackLocations->at_put( i, with->stkLoc( j ) );
    }


    const char * end_of_code = _macroAssembler->pc();

    if ( PrintMakeConformantCode and begin_of_code < end_of_code ) {
        _console->print_cr( "MakeConformant:" );
        Disassembler::decode( begin_of_code, end_of_code );
        _console->cr();
    }

    verify();
    st_assert( isConformant( with ), "mapping not conformant" );
}


// Helper class to make mappings conformant

class ConformanceHelper : public MapConformance {
    private:
        MacroAssembler * _masm;

    public:
        void generate( MacroAssembler * masm, Variable temp1, Variable temp2 );

        void move( Variable src, Variable dst );

        void push( Variable src );

        void pop( Variable dst );
};


void ConformanceHelper::generate( MacroAssembler * masm, Variable temp1, Variable temp2 ) {
    _masm = masm;
    MapConformance::generate( temp1, temp2 );
    _masm = nullptr;
}


void ConformanceHelper::move( Variable src, Variable dst ) {
    Register src_reg = src.in_register() ? Register( src.register_number(), ' ' ) : noreg;
    Register dst_reg = dst.in_register() ? Register( dst.register_number(), ' ' ) : noreg;
    Address  src_adr = src.on_stack() ? Address( ebp, src.stack_offset() ) : Address();
    Address  dst_adr = dst.on_stack() ? Address( ebp, dst.stack_offset() ) : Address();

    if ( src.in_register() ) {
        if ( dst.in_register() ) {
            _masm->movl( dst_reg, src_reg );
        } else if ( dst.on_stack() ) {
            _masm->movl( dst_adr, src_reg );
        } else {
            ShouldNotReachHere();
        }
    } else if ( src.on_stack() ) {
        if ( dst.in_register() ) {
            _masm->movl( dst_reg, src_adr );
        } else {
            ShouldNotReachHere();
        }
    } else {
        ShouldNotReachHere();
    }
}


void ConformanceHelper::push( Variable src ) {
    if ( src.in_register() ) {
        _masm->pushl( Register( src.register_number(), ' ' ) );
    } else if ( src.on_stack() ) {
        _masm->pushl( Address( ebp, src.stack_offset() ) );
    } else {
        ShouldNotReachHere();
    }
}


void ConformanceHelper::pop( Variable dst ) {
    if ( dst.in_register() ) {
        _masm->popl( Register( dst.register_number(), ' ' ) );
    } else if ( dst.on_stack() ) {
        _masm->popl( Address( ebp, dst.stack_offset() ) );
    } else {
        ShouldNotReachHere();
    }
}


void PseudoRegisterMapping::new_makeConformant( PseudoRegisterMapping * with ) {
    // set up ConformanceHelper
    bool_t            makeConformant = false;
    Variable          unused         = Variable::unused();
    ConformanceHelper chelper;
    int               j              = with->size();
    while ( j-- > 0 ) {
        if ( with->used( j ) ) {
            int i = index( with->_pseudoRegisters->at( j ) );
            if ( i >= 0 ) {
                st_assert( _pseudoRegisters->at( i ) == with->_pseudoRegisters->at( j ), "should be the same" );
                if ( regLoc( i ) not_eq with->regLoc( j ) or stkLoc( i ) not_eq with->stkLoc( j ) ) {
                    Variable src_reg = hasRegLoc( i ) ? Variable::new_register( _locations->locationAsRegisterNo( regLoc( i ) ) ) : unused;
                    Variable src_stk = hasStkLoc( i ) ? Variable::new_stack( _locations->locationAsByteOffset( stkLoc( i ) ) ) : unused;
                    Variable dst_reg = with->hasRegLoc( j ) ? Variable::new_register( with->_locations->locationAsRegisterNo( with->regLoc( j ) ) ) : unused;
                    Variable dst_stk = with->hasStkLoc( j ) ? Variable::new_stack( with->_locations->locationAsByteOffset( with->stkLoc( j ) ) ) : unused;
                    chelper.append_mapping( src_reg, src_stk, dst_reg, dst_stk );
                    makeConformant = true;
                }
            }
        }
    }

    if ( makeConformant ) {
        // mappings differ -> generate code to make them conformant
        // first: find free registers that can be used for "make conformance" code
        int      this_mask = _locations->freeRegisterMask();
        int      with_mask = with->_locations->freeRegisterMask();
        int      both_mask = this_mask & with_mask;
        Variable temp1     = unused;
        Variable temp2     = unused;
        if ( both_mask not_eq 0 ) {
            // free registers in common
            int i = 0;
            while ( ( both_mask & ( 1 << i ) ) == 0 )
                i++;
            // make sure register can actually be used in both mappings
            {
                Register reg       = Register( i, ' ' );
                int      this_rloc = _locations->registerAsLocation( reg );
                int      with_rloc = with->_locations->registerAsLocation( reg );
                // allocate & deallocate them - will fail if registers are already in use
                _locations->allocate( this_rloc );
                _locations->release( this_rloc );
                with->_locations->allocate( with_rloc );
                with->_locations->release( with_rloc );
            }
            temp1 = Variable::new_register( i );
            clearNthBit( both_mask, i );
        }

        if ( both_mask not_eq 0 ) {
            // free registers in common
            int i = 0;
            while ( ( both_mask & ( 1 << i ) ) == 0 )
                i++;
            // make sure register can actually be used in both mappings
            {
                Register reg       = Register( i, ' ' );
                int      this_rloc = _locations->registerAsLocation( reg );
                int      with_rloc = with->_locations->registerAsLocation( reg );
                // allocate & deallocate them - will fail if registers are already in use
                _locations->allocate( this_rloc );
                _locations->release( this_rloc );
                with->_locations->allocate( with_rloc );
                with->_locations->release( with_rloc );
            }
            temp2 = Variable::new_register( i );
            clearNthBit( both_mask, i );
        }
        guarantee( temp1 not_eq temp2 or temp1 == unused, "should not be the same" );

        // make conformant
        const char * begin_of_code = _macroAssembler->pc();
        chelper.generate( _macroAssembler, temp1, temp2 );

        const char * end_of_code = _macroAssembler->pc();
        if ( PrintMakeConformantCode ) {
            chelper.print();
            _console->print_cr( "(using R%d & R%d as temporary registers)", temp1.register_number(), temp2.register_number() );
            Disassembler::decode( begin_of_code, end_of_code );
            _console->cr();
        }

        // Mapping should not be used anymore now, since it is not representing the
        // current situation (one should use the 'with' mapping instead). Destroy it
        // to make sure it is not accidentally used.
        destroy();
    }
}


void PseudoRegisterMapping::makeConformant( PseudoRegisterMapping * with ) {
    //guarantee(NonLocalReturninProgress() == with->NonLocalReturninProgress(), "cannot be made conformant");

    if ( PrintPRegMapping and WizardMode ) {
        _console->print_cr( "make conformant:" );
        print();
        _console->print( "with " );
        with->print();
    }

    _locations->extendTo( with->_locations->nofStackTmps() );
    st_assert( _locations->nofArguments() == with->_locations->nofArguments(), "must be the same" );
    st_assert( _locations->nofRegisters() == with->_locations->nofRegisters(), "must be the same" );
    st_assert( _locations->nofStackTmps() >= with->_locations->nofStackTmps(), "must be greater or equal" );
    st_assert( with->isInjective(), "'with' mapping not injective" );

    // strip mapping so that it contains the same pregs as 'with' mapping
    int i = size();
    while ( i-- > 0 ) {
        if ( used( i ) ) {
            int j = with->index( _pseudoRegisters->at( i ) );
            if ( j < 0 ) {
                // entry not found in 'with' mapping -> remove it from this mapping
                kill( _pseudoRegisters->at( i ) );
            }
        }
    }

    if ( UseNewMakeConformant ) {
        new_makeConformant( with );
    } else {
        old_makeConformant( with );
    }
}


void PseudoRegisterMapping::iterate( PseudoRegisterClosure * closure ) {
    for ( int i = size(); i-- > 0; ) {
        PseudoRegister * preg = _pseudoRegisters->at( i );
        if ( preg not_eq nullptr ) {
            preg->_map_index_cache = i;
            closure->preg_do( preg );
        }
    }
}


Location PseudoRegisterMapping::locationFor( PseudoRegister * preg ) {
    int i = index( preg );
    st_assert( i >= 0, "preg must be defined" );
    Location loc = illegalLocation;
    if ( hasStkLoc( i ) ) {
        loc = Location::stackLocation( _locations->locationAsWordOffset( stkLoc( i ) ) );
    } else if ( hasRegLoc( i ) ) {
        loc = Location::registerLocation( _locations->locationAsRegister( regLoc( i ) ).number() );
    } else {
        ShouldNotReachHere();
    }
    return loc;
}


int PseudoRegisterMapping::nofPRegs() {
    int i = size();
    int n = 0;
    while ( i-- > 0 ) {
        if ( used( i ) )
            n++;
    };
    return n;
}


int PseudoRegisterMapping::maxNofStackTmps() {
    return _locations->nofStackTmps();
}


void PseudoRegisterMapping::my_print() {
    // This is only here for debugging because print() cannot
    // be called from within the debugger for some strange reason...
    print();
}


void PseudoRegisterMapping::print( int i ) {
    st_assert( used( i ), "unused slot" );
    int rloc = regLoc( i );
    int sloc = stkLoc( i );
    _console->print( "%s -> ", _pseudoRegisters->at( i )->name() );
    if ( rloc >= 0 ) {
        _console->print( _locations->locationAsRegister( rloc ).name() );
    }
    if ( sloc >= 0 ) {
        if ( rloc >= 0 )
            _console->print( ", " );
        int offs = _locations->locationAsByteOffset( sloc );
        _console->print( "[ebp%s%d]", ( offs < 0 ? "" : "+" ), offs );
    }
    _console->cr();
}


void PseudoRegisterMapping::print() {
    if ( WizardMode )
        _locations->print();
    if ( nofPRegs() > 0 ) {
        _console->print_cr( "PseudoRegister mapping:" );
        for ( int i = 0; i < size(); i++ ) {
            if ( used( i ) )
                print( i );
        }
    } else {
        _console->print_cr( "PseudoRegister mapping is empty" );
    }
    _console->cr();
    if ( _temporaryLocations->length() > 0 ) {
        _console->print_cr( "Temporaries in use:" );
        for ( int i = 0; i < _temporaryLocations->length(); i++ ) {
            int loc = _temporaryLocations->at( i );
            st_assert( _locations->isRegister( loc ), "temporaries must be in registers" );
            _console->print_cr( "temp 0x%08x -> 0x%08x %s", i, loc, _locations->locationAsRegister( loc ).name() );
        }
        _console->cr();
    }
    if ( NonLocalReturninProgress() ) {
        _console->print_cr( "NonLocalReturn in progress" );
        _console->cr();
    }
}


void PseudoRegisterMapping::verify() {
    if ( not CompilerDebug )
        return;
    _locations->verify();
    int totalUses = 0;
    int i         = size();
    while ( i-- > 0 ) {
        if ( used( i ) ) {
            // verify mapping for entry i
            int rloc = regLoc( i );
            int sloc = stkLoc( i );
            if ( rloc < 0 and sloc < 0 ) st_fatal( "no location associated with preg" );
            int rlocUses = 0;
            int slocUses = 0;
            int j        = size();
            while ( j-- > 0 ) {
                if ( used( j ) ) {
                    if ( i not_eq j and _pseudoRegisters->at( i ) == _pseudoRegisters->at( j ) ) st_fatal( "preg found twice in mapping" );
                    if ( rloc >= 0 and regLoc( j ) == rloc )
                        rlocUses++;
                    if ( sloc >= 0 and stkLoc( j ) == sloc )
                        slocUses++;
                }
            }
            // check locations usage counter
            if ( rloc >= 0 and _locations->nofUses( rloc ) not_eq rlocUses ) st_fatal( "inconsistent nofUses (register locations)" );
            if ( sloc >= 0 and _locations->nofUses( sloc ) not_eq slocUses ) st_fatal( "inconsistent nofUses (stack locations)" );
            // compute total usage
            if ( rloc >= 0 )
                totalUses++;
            if ( sloc >= 0 )
                totalUses++;
        }
    }
    // check allocated temporaries
    i = _temporaryLocations->length();
    while ( i-- > 0 ) {
        int rloc = _temporaryLocations->at( i );
        if ( _locations->nofUses( rloc ) not_eq 1 ) st_fatal( "inconsistent nofUses (temporaries)" );
        totalUses++;
    }
    // check NonLocalReturn registers if in use
    if ( NonLocalReturninProgress() ) {
        if ( _locations->nofUses( _locations->registerAsLocation( NonLocalReturn_result_reg ) ) not_eq 1 ) st_fatal( "inconsistent nofUses (NonLocalReturn_result_reg)" );
        if ( _locations->nofUses( _locations->registerAsLocation( NonLocalReturn_home_reg ) ) not_eq 1 ) st_fatal( "inconsistent nofUses (NonLocalReturn_home_reg  )" );
        if ( _locations->nofUses( _locations->registerAsLocation( NonLocalReturn_homeId_reg ) ) not_eq 1 ) st_fatal( "inconsistent nofUses (NonLocalReturn_homeId_reg)" );
        totalUses += 3;
    }
    // check total uses
    if ( _locations->nofTotalUses() not_eq totalUses ) st_fatal( "inconsistent totalUses" );
}


// Implementation of PseudoRegisterLocker

PseudoRegisterLocker * PseudoRegisterLocker::_top;


PseudoRegisterLocker::PseudoRegisterLocker( PseudoRegister * r0 ) {
    st_assert( r0 not_eq nullptr, "PseudoRegister must be defined" );
    lock( r0, nullptr, nullptr );
}


PseudoRegisterLocker::PseudoRegisterLocker( PseudoRegister * r0, PseudoRegister * r1 ) {
    st_assert( r0 not_eq nullptr and r1 not_eq nullptr, "PRegs must be defined" );
    lock( r0, r1, nullptr );
}


PseudoRegisterLocker::PseudoRegisterLocker( PseudoRegister * r0, PseudoRegister * r1, PseudoRegister * r2 ) {
    st_assert( r0 not_eq nullptr and r1 not_eq nullptr and r2 not_eq nullptr, "PRegs must be defined" );
    lock( r0, r1, r2 );
}


bool_t PseudoRegisterLocker::holds( PseudoRegister * preg ) const {
    st_assert( preg not_eq nullptr, "undefined preg" );
    int i = sizeof( _pregs ) / sizeof( PseudoRegister * );
    while ( i-- > 0 ) {
        if ( preg == _pregs[ i ] )
            return true;
    }
    return false;
}


bool_t PseudoRegisterLocker::locks( PseudoRegister * preg ) {
    st_assert( preg not_eq nullptr, "undefined preg" );
    PseudoRegisterLocker * p = _top;
    while ( p not_eq nullptr and not p->holds( preg ) )
        p = p->_prev;
    // p == nullptr or p->holds(preg)
    return p not_eq nullptr;
}


// Implementation of Temporary

Temporary::Temporary( PseudoRegisterMapping * mapping, Register hint ) {
    _mapping = mapping;
    _regLoc  = mapping->allocateTemporary( hint );
}


Temporary::Temporary( PseudoRegisterMapping * mapping, PseudoRegister * preg ) {
    // old code - keep around for time comparison purposes
    const bool_t old_code = false;
    if ( old_code ) {
        _mapping = mapping;
        _regLoc  = mapping->allocateTemporary( noreg );
        mapping->assembler()->movl( Temporary::reg(), mapping->use( preg ) );
        return;
    }

    _mapping = mapping;
    Register reg = mapping->use( preg );
    // preg is guaranteed to be in a register
    if ( mapping->onStack( preg ) ) {
        // preg is also on stack -> release register location from mapping and use it as copy
        mapping->killRegister( preg );
        _regLoc = mapping->allocateTemporary( reg );
    } else {
        // preg is only in register -> need to allocate a new register & copy it explicitly
        _regLoc = mapping->allocateTemporary( noreg );
        mapping->assembler()->movl( Temporary::reg(), reg );
    }
}


Temporary::~Temporary() {
    _mapping->releaseTemporary( _regLoc );
}
