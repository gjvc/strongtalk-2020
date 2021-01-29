
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/compiler/InliningPolicy.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/interpreter/Interpreter.hpp"


RecompilationPolicy::RecompilationPolicy( RecompilerFrame *first ) {
    _stack = new GrowableArray<RecompilerFrame *>( 50 );
    _stack->push( first );
}


Recompilee *RecompilationPolicy::findRecompilee() {
    RecompilerFrame *rf = _stack->at( 0 );
    if ( PrintRecompilation2 ) {
        for ( std::int32_t i = 0; i < 10 and rf; i++, rf = senderOf( rf ) );   // create 10 frames
        printStack();
    }
    RecompilerFrame *r  = findTopInlinableFrame();
    if ( r ) {
        if ( PrintRecompilation )
            r->print();
        return Recompilee::new_Recompilee( r );
    } else {
        return nullptr;
    }
}


void RecompilationPolicy::cleanupStaleInlineCaches() {
    std::int32_t       len = min( 20, _stack->length() );
    for ( std::int32_t i   = 0; i < len; i++ )
        _stack->at( i )->cleanupStaleInlineCaches();
}


RecompilerFrame *RecompilationPolicy::findTopInlinableFrame() {
    // go up the stack until finding a frame that (probably) won't be inlined into its caller
    RecompilerInliningPolicy p;
    RecompilerFrame          *current    = _stack->at( 0 );    // current choice for stopping
    RecompilerFrame          *prev       = nullptr;            // prev. value of current
    RecompilerFrame          *prevMethod = nullptr;        // same as prev, except always holds method frames (not blocks)
    _msg = nullptr;

    while ( 1 ) {
        if ( current == nullptr ) {
            // reached end of stack without finding a candidate
            current = prev;
            break;
        }

        if ( ( _msg = p.shouldInline( current ) ) not_eq nullptr ) {
            // current won't be inlined into caller, so stop here
            break;
        }

        // before going up the stack further, check if doing so would get us into compiled code
        RecompilerFrame *next = senderOrParentOf( current );

        if ( next ) {
            if ( next->num() > MaxRecompilationSearchLength ) {
                // don't go up too high when searching for recompilees
                _msg = "(don't go up any further: next > MaxRecompilationSearchLength)";
                break;
            }
            if ( next->distance() > MaxInterpretedSearchLength ) {
                // don't go up too high when searching for recompilees
                _msg = "(don't go up any further: next > MaxInterpretedSearchLength)";
                break;
            }
            if ( current->is_interpreted() ) {
                // Before recompiling any compiled code (next or its callers), first compile the interpreted
                // method; later, if that gets executed often, it will trigger another recompile that
                // will lead to next (or its caller) to be reoptimized.  At that point, optimization can take
                // advantage of the better type information in the compiled version of current
                LookupKey *k = next->key();
                if ( next->is_compiled() ) {
                    _msg = "(not going up into optimized code)";
                    break;
                } else if ( k not_eq nullptr and Universe::code->lookup( k ) not_eq nullptr ) {
                    _msg = "(already compiled this method)";
                    break;
                }
            }
            if ( next->is_compiled() and ( _msg = shouldNotRecompileNativeMethod( next->nm() ) ) not_eq nullptr ) {
                _msg = "NativeMethod should not be recompiled; don't go up";
                break;
            }
            if ( next->is_compiled() and next->sends() < MinSendsBeforeRecompile ) {
                _msg = "don't recompile -- hasn't sent MinSendsBeforeRecompile messages yet";
                if ( PrintRecompilation and _msg )
                    current->print();
                break;
            }
        }
        prev                  = current;

        if ( not current->is_blockMethod() )
            prevMethod = current;
        current        = next;
    }

    if ( current )
        checkCurrent( current, prev, prevMethod );

    if ( PrintRecompilation and _msg )
        spdlog::info( "%s", _msg );

    return current;
}


void RecompilationPolicy::checkCurrent( RecompilerFrame *&current, RecompilerFrame *&prev, RecompilerFrame *&prevMethod ) {
    // current is the provisional recompilation candidate; perform a few sanity checks on it
    if ( current->is_blockMethod() and current->is_interpreted() ) {
        // can't recompile blocks in isolation, and this block is too big to inline
        // thus, optimize method called by block
        if ( PrintRecompilation and _msg )
            spdlog::info( "%s", _msg );
        fixBlockParent( current );
        if ( prev and not prev->is_blockMethod() ) {
            current = prev;
            prev    = prevMethod = nullptr;
            if ( current )
                checkCurrent( current, prev, prevMethod );
            _msg = "(can't recompile block in isolation)";
        } else {
            current = nullptr;
            _msg    = "(can't recompile block in isolation, and no callee method found)";
        }
    } else if ( current->is_super() ) {
        current = prev;
        prev    = prevMethod = nullptr;
        if ( current )
            checkCurrent( current, prev, prevMethod );
        _msg = "(can't recompile super nativeMethods yet)";
    } else if ( current->is_compiled() ) {
        const char *msg2;
        if ( ( msg2 = shouldNotRecompileNativeMethod( current->nm() ) ) not_eq nullptr ) {
            current = nullptr;
            _msg    = msg2;
        } else {
            // should recompile NativeMethod
            _msg = nullptr;
        }
    }
    if ( current == nullptr )
        return;

    // check if we already recompiled this method (but old one is still on the stack)
    // Inserted after several hours of debugging by Lars
    if ( current->is_compiled() ) {
        if ( EnableOptimizedCodeRecompilation ) {
            LookupKey    *k          = current->key();
            NativeMethod *running_nm = current->nm();
            NativeMethod *newest_nm  = k ? Universe::code->lookup( k ) : nullptr;
            // NB: newest_nm is always nullptr for block nativeMethods
            if ( k and running_nm and running_nm->is_method() and running_nm not_eq newest_nm ) {
                // ideally, should determine how many stack frames nm covers, then recompile
                // the next frame
                // for now, try previous method frame
                current = prevMethod;
                prev    = prevMethod = nullptr;
                if ( current )
                    checkCurrent( current, prev, prevMethod );
            }
        } else {
            current = nullptr;
        }
    }
}


const char *RecompilationPolicy::shouldNotRecompileNativeMethod( NativeMethod *nm ) {
    // if nm should not be recompiled, return a string with the reason; otherwise, return nullptr
    if ( nm->isUncommonRecompiled() ) {
        if ( RecompilationPolicy::shouldRecompileUncommonNativeMethod( nm ) ) {
            nm->makeOld();
            return nullptr;      // ok
        } else {
            return "uncommon NativeMethod too young";
        }
    } else if ( nm->version() >= MaxVersions ) {
        return "max. version reached";
    } else if ( nm->level() == MaxRecompilationLevels - 1 ) {
        return "maximally optimized";
    } else if ( nm->isYoung() ) {
        return "NativeMethod too young";
    }
    return nullptr;
}


RecompilerFrame *RecompilationPolicy::senderOf( RecompilerFrame *rf ) {
    RecompilerFrame *sender = rf->caller();
    if ( sender and sender->num() == _stack->length() )
        _stack->push( sender );
    return sender;
}


void RecompilationPolicy::fixBlockParent( RecompilerFrame *rf ) {
    // find the parent method and increase its counter so it will be recompiled next time
    MethodOop blk = rf->top_method();
    st_assert( blk->is_blockMethod(), "must be a block" );
    MethodOop    home  = blk->home();
    std::int32_t count = home->invocation_count();
    count += Interpreter::get_invocation_counter_limit();
    count              = min( count, MethodOopDescriptor::_invocation_count_max - 1 );
    home->set_invocation_count( count );
    st_assert( home->invocation_count() >= Interpreter::get_invocation_counter_limit(), "counter increment didn't work" );
}


RecompilerFrame *RecompilationPolicy::senderOrParentOf( RecompilerFrame *rf ) {
    // "go up" on the stack to find a better recompilee;
    // for normal methods, that's the sender; for block methods, it's the home method
    // (*not* enclosing scope) unless that method is already optimized
    if ( rf->is_blockMethod() ) {
        RecompilerFrame *parent = parentOf( rf );
        if ( parent == nullptr ) {
            // can't find the parent (e.g. because it's a non-LIFO block)
            fixBlockParent( rf );
            return senderOf( rf );    // is this a good idea???
            // It may be; this way, a parent-less block doesn't prevent its callers from being
            // optimized; on the other hand, it may lead to too much compilation since what we
            // really want to do is recompile the parent.  On the third hand, if a block is non-lifo
            // = (has no parent) it may not make sense to optimize its enclosing method anyway.
            // But on the fourth hand, to optimize any block its enclosing method must be optimized.
        } else {
            if ( parent->is_compiled() == rf->is_compiled() ) {
                // try to optimize the parent
                if ( CountParentLinksAsOne )
                    parent->set_distance( rf->distance() + 1 );
                return parent;
            } else {
                return senderOf( rf );    // try sender
            }
        }
    } else if ( rf->hasBlockArgs() ) {
        // go up to highest home of any block arg
        // bug: should check how often block is created / invoked
        GrowableArray<BlockClosureOop> *blockArgs = rf->blockArgs();
        RecompilerFrame                *max       = nullptr;
        for ( std::int32_t             i          = 0; i < blockArgs->length(); i++ ) {
            BlockClosureOop blk   = blockArgs->at( i );
            //JumpTableEntry* e = blk->jump_table_entry();
            RecompilerFrame *home = parentOfBlock( blk );
            if ( home == nullptr )
                continue;
            if ( max == nullptr or home->num() > max->num() )
                max = home;
        }
        if ( max )
            return max;
    }
    // default: return sender
    return senderOf( rf );
}


RecompilerFrame *RecompilationPolicy::parentOf( RecompilerFrame *rf ) {
    st_assert( rf->is_blockMethod(), "shouldn't call" );
    // use caller VirtualFrame's receiver instead of rf's receiver because VirtualFrame may not be set up correctly
    // for most recent frame [this may not be true anymore -- Urs 10/96]
    // caller VirtualFrame must have block as the receiver because it invokes primitiveValue

    // Urs - please put in more comments here (gri):
    // b) why do you look at the receiver of the sender instead of the receiver of the send
    //    (it only happens right now that they are the same, but what if we change what we
    //    can use as first argument for primitives (primitiveValue) ?). Seems to be a lot of
    //    implicit assumptions & no checks.
    // Yes this assumes that the caller invokes primitiveValue on self; if we change that the
    // code here breaks.

    DeltaVirtualFrame *sender = rf->top_vframe()->sender_delta_frame();
    if ( sender == nullptr )
        return nullptr;

    Oop blk = sender->receiver();
    guarantee( blk->is_block(), "should be a block" );
    return parentOfBlock( BlockClosureOop( blk ) );
}


RecompilerFrame *RecompilationPolicy::parentOfBlock( BlockClosureOop blk ) {
    if ( blk->is_pure() )
        return nullptr;

    ContextOop ctx = blk->lexical_scope();
    st_assert( ctx->is_context(), "make sure we have a context" );

    std::int32_t *fp = ctx->parent_fp();
    if ( fp == nullptr ) {
        return nullptr;    // non-LIFO block
    }
    // try to find context's RecompilerFrame
    RecompilerFrame    *parent = _stack->first();
    for ( std::int32_t i       = 0; i < MaxRecompilationSearchLength; i++ ) {
        parent = senderOf( parent );
        if ( not parent )
            break;
        Frame fr = parent->fr();
        if ( fr.fp() == fp )
            return parent;    // found it!
    }
    return nullptr;      // parent not found
}


void RecompilationPolicy::printStack() {    // for debugging
    for ( std::int32_t i = 0; i < _stack->length(); i++ )
        _stack->at( i )->print();
}


bool RecompilationPolicy::needRecompileCounter( Compiler *c ) {
    if ( not UseRecompilation )
        return false;
    if ( c->version() == MaxVersions )
        return false;    // to prevent endless recompilation
    // also stop counting for "perfect" nativeMethods where nothing more can be optimized
    // NB: it is tempting to leave counters in very small methods (so that e.g. accessor functions
    // still trigger counters), but that won't work if they're invoked from megamorphic
    // call sites --> put the counters in the caller, not the callee.
    return c->level() < MaxRecompilationLevels - 1;
}


bool RecompilationPolicy::shouldRecompileAfterUncommonTrap( NativeMethod *nm ) {
    // called after nm encountered an uncommon trap; should it be recompiled into
    // less optimized form (without uncommon branches)?
    return nm->uncommon_trap_counter() >= UncommonRecompileLimit;
}


bool RecompilationPolicy::shouldRecompileUncommonNativeMethod( NativeMethod *nm ) {
    st_assert( nm->isUncommonRecompiled(), "expected an uncommon NativeMethod" );
    // nm looks like a recompilation candidate; can it really be optimized?
    // isUncommonRecompiled nativeMethods were created after the original NativeMethod encountered
    // too many "uncommon" cases.  Thus, the assumptions the compiler originally took
    // proved to be too aggressive.  In the uncommon-recompiled NativeMethod, there are
    // no uncommon branches, so it's slower.  But we don't want to reoptimize it too eagerly
    // because it needs to run std::int32_t enough to accumulate type information that's truly
    // representative of its usage.  This method determines how to make that tradeoff.
    // The main idea is to back off exponentially each time we go through the cycle
    // of optimize -- uncommon recompile -- reoptimize.
    const std::int32_t v = nm->version();
    const std::int32_t c = nm->invocation_count();
    return c >= uncommonNativeMethodInvocationLimit( v ) or ( c >= UncommonInvocationLimit and nm->age() > uncommonNativeMethodAgeLimit( v ) );
}


std::int32_t RecompilationPolicy::uncommonNativeMethodInvocationLimit( std::int32_t version ) {
    std::int32_t n = UncommonInvocationLimit;

    for ( std::int32_t i = 0; i < version; i++ )
        n *= UncommonAgeBackoffFactor;
    return n;
}


std::int32_t RecompilationPolicy::uncommonNativeMethodAgeLimit( std::int32_t version ) {
    std::int32_t n = NativeMethodAgeLimit;

    for ( std::int32_t i = 0; i < version; i++ )
        n *= UncommonAgeBackoffFactor;
    return n;
}
