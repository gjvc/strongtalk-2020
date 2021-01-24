//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/code/Zone.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/ResourceMark.hpp"


constexpr std::size_t LRU_RESOLUTION     = 16;    /* resolution (in secs) of LRU timer */
constexpr std::size_t LRU_RETIREMENT_AGE = 10;   /* min. age (# sweeps) for retirement */
#define LRU_MAX_VISITED    min(numberOfNativeMethods(), LRUMaxVisited)

/* max. # nativeMethods visited per sweep */
constexpr std::size_t LRU_MAX_RECLAIMED = 250000; /* stop after having found this amount of */

/* Space occupied by replacement candidates */
constexpr std::size_t LRU_CUTOFF      = 32;  /* max. length (bytes) of "small" methods */
constexpr std::size_t LRU_SMALL_BOOST = 3;  /* small methods live this many times longer */
// (because access methods don't clear their unused bit)

constexpr float COMPACT_OVERHEAD = 0.05; /* desired max. overhead for zone compaction */


// Trade-offs for blocksizes/queue length below:
// - larger blocks mean larger internal fragmentation but less Space
//   overhead for the heap maps and increased effectiveness of the free lists
// - longer free lists cover a wider range of allocations but can slow down
//   allocation (when most lists are empty but still have to be scanned).
constexpr std::size_t CODE_BLOCK_SIZE                = 64;  /* block size for nativeMethods */
constexpr std::size_t POLYMORPHIC_INLINE_CACHE_BLOCK = 32;    /* block size for PICs */

constexpr std::size_t   FREE       = 20;        /* # of free lists */
constexpr std::size_t   StubBlock  = 16;        /* block size for PolymorphicInlineCache zone */
constexpr std::size_t   StubFree   = 20;        /* # of free lists for PolymorphicInlineCache zone */
constexpr float MaxExtFrag = 0.05;    /* max. tolerated ext. fragmentation */

#define FOR_ALL_NMETHODS( var )                              \
    for (NativeMethod *var = first_nm(); var; var = next_nm(var))

#define FOR_ALL_PICS( var )                              \
    for (PolymorphicInlineCache *var = first_pic(); var; var = next_pic(var))


std::size_t roundSize( std::size_t s, std::size_t blockSize ) {
    return ( s + blockSize - 1 ) / blockSize * blockSize;
}


LRUcount *LRUtable;    // for optimized methods
std::size_t      *LRUflag;        // == LRUtable, just different type for convenience

static std::size_t LRUtime;        // virtual time; incremented after every full sweep

// We could directly run the sweeper from the interrupt handler, but
// this is tricky since the stack is in a strange state.
void sweepTrigger() {
    if ( UseLRUInterrupts ) {
        Universe::code->_needsSweep = true;
        // %note: should be:
        // currentProcess->setupPreemption();
    };
}


static void idOverflowError( std::size_t delta ) {
    // fix this - maybe eliminate NativeMethod IDs altogether?
    st_fatal( "zone: NativeMethod ID table overflowed" );
}


Zone::Zone( std::size_t &size ) {
    _methodHeap = new ZoneHeap( Universe::current_sizes._code_size, CODE_BLOCK_SIZE );
    _picHeap    = new ZoneHeap( Universe::current_sizes._pic_heap_size, POLYMORPHIC_INLINE_CACHE_BLOCK );

    _methodTable = new CodeTable( codeTableSize );

    // LRUflag = idManager->data;
    LRUtable = (LRUcount *) LRUflag;
    clear();

    // start sweeper
    // cpuTimer->enroll(sweepTrigger, 1.0 / LRU_RESOLUTION);
}


void Zone::clear() {
    _methodHeap->clear();
    _picHeap->clear();
    _methodTable->clear();

    LRUhand = nullptr;
    LRUtime = 0;
    jump_table()->init();
    _needsCompaction = _needsSweep = false;
    compactTime      = 0;
    compactDuration  = 1;
    minFreeFrac      = 0.05;
}


std::size_t Zone::nextNativeMethodID() {
    return jump_table()->peekID();
}


NativeMethod *Zone::allocate( std::size_t size ) {
    // CSect cs(profilerSemaphore); // for profiler
    // must get method ID here! (because reclaim might change firstFree)
    // (compiler may have used peekID to get ID of new NativeMethod for LRU stuff)
    if ( needsSweep() )
        doSweep();

    NativeMethod *n = (NativeMethod *) _methodHeap->allocate( size );

    if ( n == nullptr ) {
        // allocation failed so flush zombies and retry
        flushZombies();
        n = (NativeMethod *) _methodHeap->allocate( size );
        if ( n == nullptr ) {
            print();
            st_fatal( "cannot allocate enough Space for NativeMethod" );
        }
    }

    verify_if_often();
    return n;
}


std::size_t Zone::used() {
    return _methodHeap->usedBytes();
}


void Zone::verify_if_often() {
    if ( VerifyZoneOften ) {
        _methodHeap->verify();
    }
}


void Zone::flush() {
    ResourceMark resourceMark;
    TraceTime    t( "Flushing method cache...", PrintCodeReclamation );
    EventMarker  em( "flushing method cache" );

    // Deoptimize all stack frames
    Processes::deoptimize_all();

    FOR_ALL_NMETHODS( p ) {
        p->makeZombie( true );
    }
    flushZombies( false );

    verify_if_often();
}


std::size_t Zone::findReplCandidates( std::size_t needed ) {
    // find replacement candidates; stop if > needed bytes found or if
    // there seem to be no more candidates
# ifdef NOT_IMPLEMENTED
    std::size_t reclaimed = 0, iter = 0;
    while (iter++ < LRU_RETIREMENT_AGE and reclaimed < needed) {
      std::size_t vis, recl;
      std::size_t limit = numberOfNativeMethods();		// because usedIDs may change
      std::size_t newTime = sweeper(limit, needed, &vis, &recl);
      reclaimed += recl;
      if (recl < needed and newTime > LRUtime + 1) {
        // next sweep wouldn't reclaim anything
        assert(vis == limit, "should have visited them all");
        LRUtime = newTime - 1;
        if (PrintLRUSweep) lprintf("\n*forced new LRU time: %ld", LRUtime);
      }
    }
    return reclaimed;
#endif
    return 0;
}


// flush next replacement candidate; return # bytes freed
std::size_t Zone::flushNextMethod( std::size_t needed ) {
    std::size_t freed = 0;
    Unimplemented();
    return freed;
}


void moveInsts( const char *from, char *to, std::size_t size ) {
    NativeMethod *n   = (NativeMethod *) from;
    NativeMethod *nTo = (NativeMethod *) to;

    char *n1 = n->instructionsStart();
    char *n2 = n->instructionsEnd();
    n->moveTo( to, (const char *) n->locsEnd() - (const char *) n );
    if ( Universe::code->LRUhand == n )
        Universe::code->LRUhand = nTo;
}


class ConvertBlockClosure : public ObjectClosure {
public:
    void do_object( MemOop obj ) {
        if ( obj->is_block() and BlockClosureOop( obj )->isCompiledBlock() ) {
            BlockClosureOop( obj )->deoptimize();
        }
    }
};

static NativeMethod *debug_nm = nullptr;
extern NativeMethod *recompilee;


void Zone::flushZombies( bool_t deoptimize ) {
    // 1. cleanup all methodOop inline caches
    // 2. cleanup all NativeMethod inline caches
    // 3..deoptimized blocks with compiled code.

    // Clear caches
    LookupCache::flush();
    // Universe::cleanup_all_inline_caches();

    Universe::flush_inline_caches_in_methods();
    clear_inline_caches();

    // Convert all blocks with compiled code
    ConvertBlockClosure bl;
    Universe::object_iterate( &bl );

    FOR_ALL_NMETHODS( p ) {
        debug_nm = p;
        if ( p->isZombie() ) {
            if ( deoptimize )
                Processes::deoptimize_wrt( p );
            p->flush();
        }
    }
}


void Zone::flushUnused() {
    // flush all nativeMethods marked as unused
    // NB: access methods are always unused since they don't do the LRU thing
    // chainFrames();
    FOR_ALL_NMETHODS( p ) {
        // use makeZombie() for efficiency, not flush()
        // (see comment in zone::flush())
        p->makeZombie( true );
    }
    flushZombies();
//    flushICache();
    // unchainFrames();
}


void Zone::adjustPolicy() {
    // This routine does the policy decisions about flushing, using feedback
    // from previous flushes.  If the heap is too full, then we'll soon have
    // to compact again, leading to frequent pauses and hight overhead.  On
    // the other hand, if we flush too many nativeMethods before compacting, we'll
    // have high compilation overhead to recreate the flushed code.
#ifdef NOT_IMPLEMENTED
    if (DontUseAnyTimer) return;		// for reproducible results & debugging
    float timeSinceLast = cpuTimer->tickNo - compactTime;
    float overhead = compactDuration / timeSinceLast;
    if (overhead > COMPACT_OVERHEAD) {
      float currentFreeFrac = 1 - float(iZone->usedBytes()) / iZone->capacity();
      minFreeFrac = min(float(minFreeFrac * 1.5), float(minFreeFrac + 0.05));
      if (PrintCodeReclamation) {
        printf("(compact overhead %3.1f%%: increasing minFreeFrac to %3.1f%%) ",
           overhead *100, minFreeFrac * 100);
      }
    } else if (overhead < COMPACT_OVERHEAD / 2) {
      minFreeFrac = max(float(minFreeFrac / 1.5), float(minFreeFrac - 0.05));
      if (PrintCodeReclamation) {
        printf("(compact overhead %3.1f%%: decreasing minFreeFrac to %3.1f%%) ",
           overhead *100, minFreeFrac * 100);
      }
    }
    std::size_t minFree = std::size_t(iZone->capacity() * minFreeFrac);
    while (1) {
      std::size_t toFlush = minFree - (iZone->capacity() - iZone->usedBytes());
      if (toFlush <= 0) break;
      flushNextMethod(LRU_MAX_RECLAIMED);
    }
    compactTime = cpuTimer->tickNo;
#endif
}


#define LRU_MAX_RECLAIMED 250000 /* stop after having found this amount of */
#define LRUMaxVisited     150


void Zone::doSweep() {
    sweeper( LRUMaxVisited, LRU_MAX_RECLAIMED );
}


void Zone::doWork() {
    if ( needsSweep() )
        doSweep();
    if ( needsCompaction() )
        compact();
}


void Zone::compact( bool_t forced ) {
    // BlockProfilerTicks bpt(exclude_NativeMethod_compact);
    // CSect cs(profilerSemaphore); // for profiler

    TraceTime   t( "*compacting NativeMethod cache...", PrintCodeReclamation );
    EventMarker em( "compacting zone" );

    // chainFrames();
    flushZombies();
    const char *firstFree = nullptr;
    if ( not forced )
        adjustPolicy();
    if ( needsCompaction() ) {
        if ( PrintCodeReclamation )
            _console->print( "I" );
        firstFree = _methodHeap->compact( moveInsts );
    }
    // unchainFrames();
//    flushICache();

    verify_if_often();
    _needsCompaction = false;
    // compactDuration = cpuTimer->tickNo - compactTime;
}


void Zone::free( NativeMethod *nm ) {
    verify_if_often();
    if ( LRUhand == nm )
        LRUhand = next_nm( nm );
    _methodHeap->deallocate( nm, nm->size() );
    verify_if_often();
}


void Zone::addToCodeTable( NativeMethod *nm ) {
    _methodTable->add( nm );
}


void Zone::clear_inline_caches() {
    TraceTime   t( "*flushing inline caches...", PrintInlineCacheInvalidation );
    EventMarker em( "flushing inline caches" );

    FOR_ALL_NMETHODS( p ) {
        p->clear_inline_caches();
    }

//    flushICache();
}


void Zone::cleanup_inline_caches() {
    TraceTime   t( "*cleaning inline caches...", PrintInlineCacheInvalidation );
    EventMarker em( "cleaning inline caches" );

    FOR_ALL_NMETHODS( p ) {
        p->cleanup_inline_caches();
    }

//    flushICache();
}


std::size_t retirementAge( NativeMethod *n ) {
    std::size_t delta = LRU_RETIREMENT_AGE;
    if ( n->instructionsLength() <= LRU_CUTOFF )
        delta = max( delta, LRU_RETIREMENT_AGE * LRU_SMALL_BOOST );
    return n->lastUsed() + delta;
}


void Zone::verify() {
    _methodTable->verify();
    std::size_t n = 0;
    FOR_ALL_NMETHODS( p ) {
        n++;
        p->verify();
    }
    if ( n not_eq numberOfNativeMethods() )
        error( "zone: inconsistent usedIDs value - should be %ld, is %ld", n, numberOfNativeMethods() );
    _methodHeap->verify();
    jump_table()->verify();
}


void Zone::switch_pointers( Oop from, Oop to ) {
    Unimplemented();
}


void Zone::oops_do( void f( Oop * ) ) {
    FOR_ALL_NMETHODS( nm ) {
        nm->oops_do( f );
    }

    FOR_ALL_PICS( pic ) {
        pic->oops_do( f );
    }
}


void Zone::nativeMethods_do( void f( NativeMethod *nm ) ) {
    FOR_ALL_NMETHODS( p ) {
        f( p );
    }
}


void Zone::PICs_do( void f( PolymorphicInlineCache *pic ) ) {
    FOR_ALL_PICS( p ) {
        f( p );
    }
}


#define NMLINE( format, n, ntot, ntot2 )                      \
  _console->print(format, (n), 100.0 * (n) / (ntot), 100.0 * (n) / (ntot2))

class nmsizes {
    std::size_t n, insts, locs, scopes;
public:
    nmsizes() {
        n = insts = locs = scopes = 0;
    }


    std::size_t total() {
        return n * sizeof( NativeMethod ) + insts + locs + scopes;
    }


    bool_t isEmpty() {
        return n == 0;
    }


    void print( const char *name, nmsizes &tot ) {
        std::size_t bigTotal = tot.total();
        std::size_t myTotal  = total();
        if ( not isEmpty() ) {
            _console->print( "%-13s (%ld methods): ", name, n );
            NMLINE( "headers = %ld (%2.0f%%/%2.0f%%); ", n * sizeof( NativeMethod ), myTotal, bigTotal );
            NMLINE( "instructions = %ld (%2.0f%%/%2.0f%%);\n", insts, myTotal, bigTotal );
            NMLINE( "\tlocs = %ld (%2.0f%%/%2.0f%%); ", locs, myTotal, bigTotal );
            NMLINE( "scopes = %ld (%2.0f%%/%2.0f%%);\n", scopes, myTotal, bigTotal );
            NMLINE( "\ttotal = %ld (%2.0f%%/%2.0f%%)\n", myTotal, myTotal, bigTotal );
        }
    }


    void print( const char *title, std::size_t t ) {
        _console->print_cr( "   n [%3d] title [%s] nativeMethods %2d%% = [%4dK], hdr [%2d%%], inst [%2d%%], locs [%2d%%], debug [%2d%%]", n, title, total() * 100 / t, total() / 1024, n * sizeof( NativeMethod ) * 100 / total(), insts * 100 / total(), locs * 100 / total(), scopes * 100 / total() );
    }


    void add( NativeMethod *nm ) {
        n++;
        insts += nm->instructionsLength();
        locs += nm->locsLen();
        scopes += nm->scopes()->length();
    }
};


void Zone::print() {
    nmsizes nms;
    nmsizes zombies;
    std::size_t     uncommon = 0;

    FOR_ALL_NMETHODS( p ) {
        if ( p->isZombie() ) {
            zombies.add( p );
        } else {
            nms.add( p );
            if ( p->isUncommonRecompiled() )
                uncommon++;
        }
    }

    _console->print_cr( "Zone:" );

    if ( not nms.isEmpty() ) {
        _console->print_cr( "  Code (%dK, %d%% used)", _methodHeap->capacity() / 1024, ( _methodHeap->usedBytes() * 100 ) / _methodHeap->capacity() );
        nms.print( "live", _methodHeap->capacity() );
    }
    if ( uncommon ) {
        _console->print_cr( "(%d live uncommon nativeMethods)", uncommon );
    }
    if ( not zombies.isEmpty() ) {
        zombies.print( "dead", _methodHeap->capacity() );
        _console->print_cr( "  PICs (%dK, %d%% used)", _picHeap->capacity() / 1024, ( _picHeap->usedBytes() * 100 ) / _picHeap->capacity() );
    }

    std::size_t n     = 0;
    std::size_t insts = 0;
    FOR_ALL_PICS( pic ) {
        n++;
        insts += pic->code_size();
    }
    std::size_t total = insts + n * sizeof( PolymorphicInlineCache );

    if ( n > 0 ) {
        _console->print_cr( "   %3d entries = %dK (hdr %2d%%, inst %2d%%)", n, total / 1024, n * sizeof( PolymorphicInlineCache ) * 100 / total, insts * 100 / total );
    }
}


struct nm_hist_elem {
    NativeMethod *nm;
    std::size_t count;
    std::size_t size;
    std::size_t sic_count;
    std::size_t sic_size;
};


static std::size_t compareOop( const void *m1, const void *m2 ) {
    ResourceMark rm;
    const auto *nativeMethod1 = reinterpret_cast<const struct nm_hist_elem *>( m1 );
    const auto *nativeMethod2 = reinterpret_cast<const struct nm_hist_elem *>( m2 );
    return nativeMethod2->nm->method() - nativeMethod1->nm->method();
}


static std::size_t compareCount( const void *m1, const void *m2 ) {
    const auto *nativeMethod1 = reinterpret_cast<const struct nm_hist_elem *>( m1 );
    const auto *nativeMethod2 = reinterpret_cast<const struct nm_hist_elem *>( m2 );
    return nativeMethod2->count - nativeMethod1->count;
}


void Zone::print_NativeMethod_histogram( std::size_t size ) {
#ifdef NOT_IMPLEMENTED
    ResourceMark resourceMark;
    nm_hist_elem* hist_array = NEW_RESOURCE_ARRAY(nm_hist_elem, numberOfNativeMethods());

    std::size_t*  compiled_nativeMethods = NEW_RESOURCE_ARRAY(std::size_t, numberOfNativeMethods());

    std::size_t n = 0;
    FOR_ALL_NMETHODS(p) {
      if (not p->isAccess() and not p->isZombie()) {
        hist_array[n].nm     = p;
        hist_array[n].size   = p->instsLen() +
                               p->locsLen() +
                               p->depsLen +
                               p->scopes->length();
        n++;
      }
    }

    qsort(hist_array, n, sizeof(nm_hist_elem), compareOop);

    for (std::size_t i = 0; i < n; i++) compiled_nativeMethods[i] = 0;

    std::size_t i = 0;
    std::size_t out = 0;
    while (i < n) {
      std::size_t counter     = 1;
      std::size_t all_size    = 0;
      std::size_t sic_counter = 0;
      std::size_t sic_size    = 0;

      Oop method = hist_array[i].nm->method();
      if (hist_array[i].nm->flags.compiler == nm_new) {
        sic_counter++;
        sic_size += hist_array[i].size;
      }
      all_size = hist_array[i].size;

      i++;

      while (i < n and method == hist_array[i].nm->method()) {
        if (hist_array[i].nm->flags.compiler == nm_sic) {
      sic_counter++;
          sic_size += hist_array[i].size;
        }
        all_size += hist_array[i].size;
        counter++; i++;
      }
      compiled_nativeMethods[counter]++;
      if (counter > size) {
        hist_array[out]           = hist_array[i-1];
        hist_array[out].count     = counter;
        hist_array[out].size      = all_size;
        hist_array[out].sic_count = sic_counter;
        hist_array[out].sic_size  = sic_size;
        out++;
      }
    }

    qsort(hist_array, out, sizeof(nm_hist_elem), compareCount);

    printf("\n# nm \t # methods \t%% acc.\n");
    std::size_t nm_count = 0;
    for (std::size_t i = 0; i < n; i++) {
      if (compiled_nativeMethods[i] > 0) {
        nm_count += i * compiled_nativeMethods[i];
        printf("%5ld \t%5ld \t\t%3ld %%\n", i, compiled_nativeMethods[i],
           (nm_count*100)/n);

      }
    }

    printf( "\nList of methods with more than %d nativeMethods compiled.\n", size);
    printf( " ALL(#,Kb)  Compiler(#,Kb) Method:\n");
    for (std::size_t i = 0; i < out; i++) {
      printf("%4d,%-4d   %4d,%-4d ",
         hist_array[i].count,     hist_array[i].size / 1024,
         hist_array[i].sic_count, hist_array[i].sic_size / 1024);
      printName((MethodKlass*) hist_array[i].nm->method()->klass(),
             hist_array[i].nm->key.selector);
      printf("\n");
    }

    fflush(stdout);
#endif
}


bool_t Zone::isDeltaPC( void *p ) const {
    return _methodHeap->contains( p ) or _picHeap->contains( p );
}


NativeMethod *Zone::findNativeMethod( const void *start ) const {
    NativeMethod *n;
    if ( _methodHeap->contains( start ) ) {
        n = (NativeMethod *) _methodHeap->findStartOfBlock( start );
        st_assert( (const char *) start < (const char *) n->locsEnd(), "found wrong NativeMethod" );
    }
    st_assert( _methodHeap->contains( n ), "not in zone" );
    st_assert( n->isNativeMethod(), "findNativeMethod didn't find NativeMethod" );
    st_assert( n->encompasses( start ), "doesn't encompass start" );
    return n;
}


NativeMethod *Zone::findNativeMethod_maybe( void *start ) const {
    // start *may* point into the instructions part of a NativeMethod; find it
    if ( not _methodHeap->contains( start ) )
        return nullptr;
    // relies on FOR_ALL_NMETHODS to enumerate in ascending order
    FOR_ALL_NMETHODS( p ) {
        if ( p->instructionsStart() > (const char *) start )
            return nullptr;
        if ( p->instructionsEnd() > (const char *) start )
            return p;
    }
    return nullptr;
}


void Zone::mark_dependents_for_deoptimization() {
    ResourceMark resourceMark;
    FOR_ALL_NMETHODS( nm ) {
        if ( nm->depends_on_invalid_klass() ) {
            GrowableArray<NativeMethod *> *nms = nm->invalidation_family();

            for ( std::size_t index = 0; index < nms->length(); index++ ) {
                NativeMethod *elem = nms->at( index );
                if ( TraceApplyChange ) {
                    _console->print( "invalidating " );
                    elem->print_value_on( _console );
                    _console->cr();
                }
                elem->mark_for_deoptimization();
            }
        }
    }
}


void Zone::mark_all_for_deoptimization() {
    FOR_ALL_NMETHODS( nm ) {
        nm->mark_for_deoptimization();
    }
}


void Zone::unmark_all_for_deoptimization() {
    FOR_ALL_NMETHODS( nm )nm->unmark_for_deoptimization();
}


void Zone::make_marked_nativeMethods_zombies() {
    FOR_ALL_NMETHODS( nm ) {
        if ( nm->is_marked_for_deoptimization() ) {
            nm->makeZombie( true );
        }
    }
}


const char *Zone::instsStart() {
    return _methodHeap->startAddr();
}


std::size_t Zone::instsSize() {
    return _methodHeap->capacity();
}


NativeMethod *Zone::next_circular_nm( NativeMethod *nm ) {
    nm     = next_nm( nm );
    if ( nm == nullptr )
        nm = first_nm();
    return nm;
}


// called every LRU_RESOLUTION seconds or by reclaimNativeMethods if needed
// returns time at which oldest non-reclaimed NativeMethod will be reclaimed
std::size_t Zone::sweeper( std::size_t maxVisit, std::size_t maxReclaim, std::size_t *nvisited, std::size_t *nbytesReclaimed ) {
#ifdef NOT_IMPLEMENTED
    EventMarker  em( "LRU sweep" );
    ResourceMark resourceMark;

    timer tmr;
    std::size_t   visited  = 0;
    std::size_t   nused    = 0;
    std::size_t   nbytes   = 0;
    std::size_t   nextTime = LRUtime + LRU_RETIREMENT_AGE;
    NativeMethod * first = first_nm();
    if ( not first ) return -1;

    NativeMethod * p = LRUhand ? LRUhand : first;
    if ( PrintLRUSweep or PrintLRUSweep2 ) {
        printf( "*starting LRU sweep..." );
        fflush( stdout );
        tmr.start();
    }

    do {
        if ( PrintLRUSweep2 ) printf( "\n*inspecting %#lx (id %ld): ", p, p->id );

        if ( ( p->isZombie() or
               p->isDebug() or
               p->codeTableLink.isEmpty() and not p->isUncommon() ) and
             p->frame_chain == nullptr ) {
            // can be flushed - nobody will ever use it again
            if ( PrintLRUSweep2 )
                printf( " %s; flushed",
                        p->isZombie() ? "zombie":
                        ( p->isDebug() ? "debug" : "unreachable" ) );
            nbytes += iZone->sizeOfBlock( p );
            p->flush();
        } else if ( p->isUsed() ) {
            // has been used
            nused++;
            if ( PrintLRUSweep2 ) {
                printf( "used" );
                std::size_t diff = useCount[ p->id ] - p->oldCount;
                if ( diff ) printf( " %ld times", diff );
            }
            if ( LRUDecayFactor > 1 ) {
                useCount[ p->id ] = std::size_t( useCount[ p->id ] / LRUDecayFactor );
            }
            p->oldCount                = useCount[ p->id ];
            LRUtable[ p->id ].unused   = true;
            LRUtable[ p->id ].lastUsed = LRUtime;
            if ( p->zoneLink.notEmpty() ) {
                if ( PrintLRUSweep2 ) printf( "; removed from replCandidates" );
                p->zoneLink.remove();        // no longer a replacement candidate
                nbytes -= iZone->sizeOfBlock( p );
            }
        } else if ( retirementAge( p ) < LRUtime and p->zoneLink.isEmpty() ) {
            if ( PrintLRUSweep2 ) printf( "added to replCandidates" );
            replCandidates.add( &p->zoneLink );
            nbytes += iZone->sizeOfBlock( p );
        } else {
            std::size_t age = retirementAge( p );
            if ( age < nextTime ) {
                nextTime = age;
            }
            if ( PrintLRUSweep2 ) {
                printf( "unused (age %ld)", LRUtime - p->lastUsed() );
                if ( p->zoneLink.notEmpty() )
                    printf( " already scheduled for replacement" );
            }
        }

        NativeMethod * oldp = p;
        p = next_circular_nm( p );
        if ( p < oldp ) {                // wrap around
            LRUtime++;
            // The LRU scheme will actually fail if LRUtime > 2^16, but that
            // won't happen very often (every 20*LRU_RESOLUTION CPU hours).
            if ( PrintLRUSweep2 ) printf( "\n*new LRU time: %ld", LRUtime );
        }
    } while ( ++visited < maxVisit and nbytes < maxReclaim and p );

    if ( needsSweep() and LRUDecayFactor > 1 ) {
        // called from timer; decay count stubs
        stubs->decay( LRUDecayFactor );
    }

    LRUhand     = p;
    _needsSweep = false;
    if ( PrintLRUSweep or PrintLRUSweep2 ) {
        printf( " done: %ld/%ld visits, %ld bytes, %ld ms.\n",
                nused, visited, nbytes, tmr.time() );
        fflush( stdout );
    }
    if ( nvisited ) *nvisited               = visited;
    if ( nbytesReclaimed ) *nbytesReclaimed = nbytes;
    return nextTime;
#endif
    return 0;
}


std::size_t Zone::LRU_time() {
    return LRUtime;
}


void printAllNativeMethods() {
    for ( NativeMethod *m = Universe::code->first_nm(); m not_eq nullptr; m = Universe::code->next_nm( m ) ) {
        if ( not m->isZombie() ) {
            m->scopes()->print_partition();
        }
    }
}




// On the x86, the I cache is consistent after the next branch or call, so don't need to do any flushing.

void flushICacheWord( void *addr ) {
}


void flushICacheRange( void *start, void *end ) {
}
