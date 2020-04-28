
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/NativeMethod.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"
#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/Timer.hpp"

NativeMethod  * recompilee = nullptr;            // method being recompiled
Recompilation * theRecompilation;


const char * Recompilation::methodOop_invocation_counter_overflow( Oop receiver, MethodOop method ) {

    // called by the interpreter whenever a method's invocation counter reaches the limit
    // returns continuation pc or nullptr if continuing in interpreter
    //
    // Note: For block methods the receiver is the context associated with the
    //       block closure from which the method is coming.

    // It seems that sometimes the method is screwed up, which causes a crash in consequence.
    // The following tests are for debugging only and should be removed at some point - gri 7/11/96.
    // If the method is illegal, recompilation is simply aborted.
    bool_t ok = false;
    if ( Universe::is_heap( ( Oop * ) method ) ) {
        MemOop obj = as_memOop( Universe::object_start( ( Oop * ) method ) );
        if ( obj->is_method() ) {
            ok = true;
        }
    }

    if ( not ok ) {
        // Possibly caused by a method sweeper bug: inline cache has been modified during the send.
        // To check: method is a JumpTable entry to an NativeMethod instead of a methodOop.
        const char * msg = Oop( method )->is_smi() ? "(method might be jump table entry)" : "";
        LOG_EVENT3( "invocation counter overflow with broken methodOop 0x%x (recv = 0x%x) %s", method, receiver, msg );
        fatal( "invocation counter overflow with illegal method - internal error" );
        // fatal("invocation counter overflow with illegal method - tell Robert");
        // continuing here is probably catastrophal because the invocation counter
        // increment might have modified the jump table entries anyway.
        return nullptr;
    }

    if ( UseRecompilation ) {
        Recompilation r( receiver, method );
        VMProcess::execute( &r );
        if ( r.recompiledTrigger() ) {
            return r.result();
        } else {
            return nullptr;
        }
    } else {
        method->set_invocation_count( 0 );
        return nullptr;
    }
}


const char * Recompilation::nativeMethod_invocation_counter_overflow( Oop receiver, char * retpc ) {
    // called by an NativeMethod whenever the NativeMethod's invocation counter reaches its limit
    // retpc is the recompilee's return PC (i.e., the pc of the NativeMethod which triggered
    // the invocation counter overflow).
    ResourceMark resourceMark;
    NativeMethod * trigger = findNativeMethod( retpc );
    LOG_EVENT3( "nativeMethod_invocation_counter_overflow: receiver = %#x, pc = %#x (NativeMethod %#x)", receiver, retpc, trigger );
    const char        * continuationAddr = trigger->verifiedEntryPoint();   // where to continue
    DeltaVirtualFrame * vf               = DeltaProcess::active()->last_delta_vframe();
    st_assert( vf->is_compiled_frame() and ( ( CompiledVirtualFrame * ) vf )->code() == trigger, "stack isn't set up right" );
    DeltaProcess::active()->trace_stack();
    if ( UseRecompilation ) {
        Recompilation recomp( receiver, trigger );
        VMProcess::execute( &recomp );
        if ( recomp.recompiledTrigger() ) {
            continuationAddr = recomp.result();
        }
    } else {
        //compiler_warning("if UseRecompilation is off, why does NativeMethod %#x have a counter?", trigger);
        trigger->set_invocation_count( 1 );
    }
    return continuationAddr;
}


NativeMethod * compile_method( LookupKey * key, MethodOop m ) {

    if ( UseInliningDatabase and not m->is_blockMethod() ) {
        // Find entry in inlining database matching the key.
        // If entry found we use the stored RecompilationScope a basis for the compile.
        RecompilationScope * rscope = InliningDatabase::lookup_and_remove( key );
        if ( rscope ) {
            VM_OptimizeRScope op( rscope );
            VMProcess::execute( &op );
            if ( TraceInliningDatabase ) {
                _console->print_cr( "Inlining database compile " );
                key->print_on( _console );
                _console->cr();
            }
            return op.result();
        }
    }

    VM_OptimizeMethod op( key, m );
    VMProcess::execute( &op );
    return op.result();
}


void Recompilation::init() {

    _newNativeMethod   = nullptr;
    _deltaVirtualFrame = nullptr;
    _recompiledTrigger = false;

    st_assert( not theRecompilation, "already set" );
    theRecompilation = this;
    if ( _method->is_blockMethod() and not isCompiled() ) {
        // interpreter passes in parent context, not receiver
        // should compute receiver via context (although receiver isn't actually used
        // for block RFrames right now) -- fix this
        if ( _receiver->is_context() ) {
            ContextOop ctx = ( ContextOop ) _receiver;
        } else {
            st_assert( _receiver == nilObj, "expected nil" );
        }
    }
}


void Recompilation::doit() {
    ResourceMark resourceMark;
    if ( PrintRecompilation ) {
        lprintf( "recompilation trigger: %s (%#x)\n", _method->selector()->as_string(), isCompiled() ? ( const char * ) _nativeMethod : ( const char * ) _method );
    }

    _deltaVirtualFrame = calling_process()->last_delta_vframe();
    // trigger is the frame that triggered its counter
    // caution: that frame may be in a bad state, so don't access data within it
    RecompilerFrame * first;
    if ( isCompiled() ) {
        first = new CompiledRecompilerFrame( _deltaVirtualFrame->fr() );
    } else {
        first = new InterpretedRecompilerFrame( _deltaVirtualFrame->fr(), _method, _receiver->klass() );
    }

    RecompilationPolicy policy( first );
    if ( handleStaleInlineCache( first ) ) {
        // called obsolete method/NativeMethod -- no need to recompile
    } else {
        Recompilee * r;
        if ( _isUncommonBranch ) {
            st_assert( isCompiled(), "must be compiled" );
            r = Recompilee::new_Recompilee( first );
        } else {
            r = policy.findRecompilee();
        }
        _recompiledTrigger = r not_eq nullptr and r->rframe() == first;
        if ( r ) {
            recompile( r );
        }

        //slr debugging
        if ( false and _nativeMethod ) {
            _console->cr();
            _nativeMethod->print_value_on( _console );
            _console->cr();
            _method->print_value_on( _console );
            _console->cr();
            _console->print_cr( "uncommon? %d", _nativeMethod->isUncommonRecompiled() );
        }
        //slr debugging

        if ( true or not _recompiledTrigger ) {      // fix this
            // reset the trigger's counter
            if ( isCompiled() /*and not _nm->isUncommonRecompiled()*/) {
                // don't
                _nativeMethod->set_invocation_count( 1 );
            } else {
                _method->set_invocation_count( 1 );
            }
        }
    }
    policy.cleanupStaleInlineCaches();
    recompilee = nullptr;
}


bool_t Recompilation::handleStaleInlineCache( RecompilerFrame * first ) {

    // check if the trigger was an interpreted method that has already been compiled; return true if so
    LookupKey * key = first->key();
    if ( not key )
        return false;
    NativeMethod * nm = Universe::code->lookup( key );
    if ( nm == nullptr )
        return false;

    // yes, we already have a compiled method; see if the sending inline cache points to that NativeMethod
    InlineCacheIterator * it = first->caller()->fr().current_ic_iterator();
    if ( not it )
        return false;    // no inline cache (perform)
    st_assert( it->selector() == key->selector(), "selector mismatch" );
    while ( not it->at_end() and it->klass() not_eq key->klass() )
        it->advance();
    if ( it->at_end() ) {
        // NB: this is possible -- inline cache could have been modified after the call, so now the called method is no longer in it
    } else {
        NativeMethod * target = it->compiled_method();
        st_assert( not target or target == nm or target->_lookupKey.equal( &nm->_lookupKey ), "inconsistent target" );
        if ( not target or target not_eq nm ) {
            // yes, the inline cache still calls the interpreted method rather than the compiled one,
            // or calls an obsolete NativeMethod which has been recompiled
            // replace it with the compiled one; no need to recompile anything now
            if ( PrintRecompilation ) {
                if ( it->is_interpreted_ic() ) {
                    lprintf( "replacing nm %#x in InterpretedInlineCache %#x\n", nm, it->interpreted_ic() );
                } else {
                    lprintf( "replacing nm %#x in CompiledInlineCache %#x\n", nm, it->compiled_ic() );
                }
            }

            // Replace the element in the inline cache
            if ( it->is_interpreted_ic() ) {
                it->interpreted_ic()->replace( nm );
            } else {
                it->compiled_ic()->replace( nm );
            }

            _newNativeMethod = nm;
            return true;
        }
    }

    return false;
}


Oop Recompilation::receiverOf( DeltaVirtualFrame * vf ) const {
    return _deltaVirtualFrame->equal( vf ) ? _receiver : vf->receiver();
}


//#ifdef HEAVY_CLEANUP
class CleanupInlineCaches : public ObjectClosure {
        void do_object( MemOop obj ) {
            if ( obj->is_method() )
                MethodOop( obj )->cleanup_inline_caches();
        }
};
//#endif


void Recompilation::recompile( Recompilee * r ) {

    // recompile r
    recompilee = r->is_compiled() ? r->code() : nullptr;    // previous version (if any)
    if ( r->rframe()->is_blockMethod() ) {
        recompile_block( r );
    } else {
        recompile_method( r );
    }

    if ( _newNativeMethod == nullptr )
        return;              // possible -- fix this later

    if ( recompilee and not recompilee->isFree() ) {
        // discard old NativeMethod (*after* compiling newNM)
        recompilee->clear_inline_caches();
    }

    // because compilation uses oldNM's PICs)
    recompilee = nullptr;

    // now install _newNM in calling inline cache
    VirtualFrame        * vf = r->rframe()->top_vframe();
    InlineCacheIterator * it = vf->fr().sender_ic_iterator();
    if ( it ) {
        // Replace the element in the inline cache
        if ( it->is_interpreted_ic() ) {
            InterpretedInlineCache * ic = it->interpreted_ic();
            if ( not ic->is_empty() )
                ic->replace( _newNativeMethod );
        } else {
            CompiledInlineCache * ic = it->compiled_ic();
            if ( not ic->is_empty() )
                ic->replace( _newNativeMethod );
        }
    } else if ( not _newNativeMethod->is_method() ) {
        // recompiled a block: block call stub has already been updated
    } else {
        // called from C (incl. performs)
    }

    // update lookup caches
    LookupCache::flush( &_newNativeMethod->_lookupKey );
    DeltaCallCache::clearAll();
}


void Recompilation::recompile_method( Recompilee * r ) {
    // recompile method r
    LookupKey * key = r->key();
    MethodOop m = r->method();

    LookupResult res = LookupCache::lookup( key );
    st_assert( res.method() == m, "mismatched method" );
    if ( res.method() not_eq m ) {
        res.method()->print();
        m->print();
    }
    _newNativeMethod = Universe::code->lookup( key );          // see if we've already compiled it

    if ( _newNativeMethod == nullptr or _newNativeMethod == recompilee ) {
        if ( recompilee and not recompilee->isZombie() )
            recompilee->unlink(); // remove it from the code table
        _newNativeMethod = compile_method( key, m );
        if ( recompilee and not recompilee->isZombie() )
            recompilee->makeZombie( true );
//#ifdef HEAVY_CLEANUP
        static int count;
        if ( count++ > 10 ) {
            count = 0;
            TraceTime           t( "*cleaning inline caches...", PrintRecompilation2 );
            CleanupInlineCaches blk;
            Universe::object_iterate( &blk );
            Universe::code->cleanup_inline_caches();
        }
//#endif
    } else {
        recompilee = nullptr;                  // no recompilation necessary
    }
}


void Recompilation::recompile_block( Recompilee * r ) {
    st_assert( r->rframe()->is_blockMethod(), "must be block recompilation" );
    if ( recompilee == nullptr ) {
        // Urs please fix this.
        // Sometimes recompilee is nullptr when this is called
        // Lars, 6-18-96
        compiler_warning( "recompilee == nullptr when recompile_block is called, possibly internal error" );
        //compiler_warning("recompilee == nullptr when recompile_block is called -- Urs please look at this");
        _newNativeMethod = nullptr;
        return;
    }
    st_assert( recompilee not_eq nullptr, "must have block recompilee" );

    DeltaVirtualFrame * vf = r->rframe()->top_vframe();
    Oop               block;
    if ( recompilee and recompilee->is_block() ) {
        DeltaVirtualFrame * sender = vf->sender_delta_frame();
        if ( sender == nullptr )
            return;              // pathological case (not sure it can happen)
        GrowableArray <Oop> * exprs = sender->expression_stack();
        // primitiveValue takes block as first argument
        int                 nargs   = recompilee->method()->nofArgs();
        block = exprs->at( nargs );
    } else {
        block = receiverOf( vf );
    }

    st_assert( block->is_block(), "must be a block" );
    _newNativeMethod = JumpTable::compile_block( BlockClosureOop( block ) );
    if ( recompilee not_eq nullptr )
        recompilee->makeZombie( true );      // do this last (after recompilation)
}


Recompilee * Recompilee::new_Recompilee( RecompilerFrame * rf ) {
    if ( rf->is_compiled() ) {
        return new CompiledRecompilee( rf, ( ( CompiledRecompilerFrame * ) rf )->nm() );
    } else {
        InterpretedRecompilerFrame * irf = ( InterpretedRecompilerFrame * ) rf;
        // Urs, please check this!
        st_assert( irf->key() not_eq nullptr, "must have a key" );
        return new InterpretedRecompilee( irf, irf->key(), irf->top_method() );
    }
}


LookupKey * CompiledRecompilee::key() const {
    return &_nativeMethod->_lookupKey;
}


MethodOop CompiledRecompilee::method() const {
    return _nativeMethod->method();
}
