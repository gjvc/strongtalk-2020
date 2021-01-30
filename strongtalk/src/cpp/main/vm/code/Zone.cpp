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


constexpr std::int32_t LRU_RESOLUTION     = 16;    /* resolution (in secs) of LRU timer */
constexpr std::int32_t LRU_RETIREMENT_AGE = 10;   /* min. age (# sweeps) for retirement */
#define LRU_MAX_VISITED    min(numberOfNativeMethods(), LRUMaxVisited)

/* max. # nativeMethods visited per sweep */
constexpr std::int32_t LRU_MAX_RECLAIMED = 250000; /* stop after having found this amount of */

/* Space occupied by replacement candidates */
constexpr std::int32_t LRU_CUTOFF      = 32;  /* max. length (bytes) of "small" methods */
constexpr std::int32_t LRU_SMALL_BOOST = 3;  /* small methods live this many times longer */
// (because access methods don't clear their unused bit)

constexpr float COMPACT_OVERHEAD = 0.05; /* desired max. overhead for zone compaction */


// Trade-offs for blocksizes/queue length below:
// - larger blocks mean larger internal fragmentation but less Space
//   overhead for the heap maps and increased effectiveness of the free lists
// - longer free lists cover a wider range of allocations but can slow down
//   allocation (when most lists are empty but still have to be scanned).
constexpr std::int32_t CODE_BLOCK_SIZE                = 64;  /* block size for nativeMethods */
constexpr std::int32_t POLYMORPHIC_INLINE_CACHE_BLOCK = 32;    /* block size for PICs */

constexpr std::int32_t FREE       = 20;        /* # of free lists */
constexpr std::int32_t StubBlock  = 16;        /* block size for PolymorphicInlineCache zone */
constexpr std::int32_t StubFree   = 20;        /* # of free lists for PolymorphicInlineCache zone */
constexpr float        MaxExtFrag = 0.05;    /* max. tolerated ext. fragmentation */

#define FOR_ALL_NMETHODS( var )                              \
    for (NativeMethod *var = first_nm(); var; var = next_nm(var))

#define FOR_ALL_PICS( var )                              \
    for (PolymorphicInlineCache *var = first_pic(); var; var = next_pic(var))


std::int32_t roundSize( std::int32_t s, std::int32_t blockSize ) {
    return ( s + blockSize - 1 ) / blockSize * blockSize;
}


LRUcount     *LRUtable;    // for optimized methods
std::int32_t *LRUflag;        // == LRUtable, just different type for convenience

static std::int32_t LRUtime;        // virtual time; incremented after every full sweep

// We could directly run the sweeper from the interrupt handler, but
// this is tricky since the stack is in a strange state.
void sweepTrigger() {
    if ( UseLRUInterrupts ) {
        Universe::code->_needsSweep = true;
        // %note: should be:
        // currentProcess->setupPreemption();
    };
}


static void idOverflowError( std::int32_t delta ) {
    // fix this - maybe eliminate NativeMethod IDs altogether?
    st_fatal( "zone: NativeMethod ID table overflowed" );
}


Zone::Zone( std::int32_t &size ) {
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


std::int32_t Zone::nextNativeMethodID() {
    return jump_table()->peekID();
}


NativeMethod *Zone::allocate( std::int32_t size ) {
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


std::int32_t Zone::used() {
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


std::int32_t Zone::findReplCandidates( std::int32_t needed ) {
    // find replacement candidates; stop if > needed bytes found or if
    // there seem to be no more candidates
# ifdef NOT_IMPLEMENTED
    std::int32_t reclaimed = 0, iter = 0;
    while (iter++ < LRU_RETIREMENT_AGE and reclaimed < needed) {
      std::int32_t vis, recl;
      std::int32_t limit = numberOfNativeMethods();		// because usedIDs may change
      std::int32_t newTime = sweeper(limit, needed, &vis, &recl);
      reclaimed += recl;
      if (recl < needed and newTime > LRUtime + 1) {
        // next sweep wouldn't reclaim anything
        assert(vis == limit, "should have visited them all");
        LRUtime = newTime - 1;
        if (PrintLRUSweep) spdlog::info("\n*forced new LRU time: %ld", LRUtime);
      }
    }
    return reclaimed;
#endif
    return 0;
}


// flush next replacement candidate; return # bytes freed
std::int32_t Zone::flushNextMethod( std::int32_t needed ) {
    std::int32_t freed = 0;
    Unimplemented();
    return freed;
}


void moveInsts( const char *from, char *to, std::int32_t size ) {
    NativeMethod *n   = (NativeMethod *) from;
    NativeMethod *nTo = (NativeMethod *) to;

//    char *n1 = n->instructionsStart();
//    char *n2 = n->instructionsEnd();

    n->moveTo( to, (const char *) n->locsEnd() - (const char *) n );
    if ( Universe::code->LRUhand == n ) {
        Universe::code->LRUhand = nTo;
    }
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


void Zone::flushZombies( bool deoptimize ) {
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
            if ( deoptimize ) {
                Processes::deoptimize_wrt( p );
            }
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
        spdlog::info("(compact overhead %3.1f%%: increasing minFreeFrac to %3.1f%%) ",
           overhead *100, minFreeFrac * 100);
      }
    } else if (overhead < COMPACT_OVERHEAD / 2) {
      minFreeFrac = max(float(minFreeFrac / 1.5), float(minFreeFrac - 0.05));
      if (PrintCodeReclamation) {
        spdlog::info("(compact overhead %3.1f%%: decreasing minFreeFrac to %3.1f%%) ",
           overhead *100, minFreeFrac * 100);
      }
    }
    std::int32_t minFree = std::int32_t(iZone->capacity() * minFreeFrac);
    while (1) {
      std::int32_t toFlush = minFree - (iZone->capacity() - iZone->usedBytes());
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


void Zone::compact( bool forced ) {
    // BlockProfilerTicks bpt(exclude_NativeMethod_compact);
    // CSect cs(profilerSemaphore); // for profiler

    TraceTime   t( "*compacting NativeMethod cache...", PrintCodeReclamation );
    EventMarker em( "compacting zone" );

    // chainFrames();
    flushZombies();
//    const char *firstFree = nullptr;
    if ( not forced )
        adjustPolicy();

    if ( needsCompaction() ) {
        if ( PrintCodeReclamation ) {
            _console->print( "I" );
        }
        const char *firstFree = _methodHeap->compact( moveInsts );
    }
    // unchainFrames();
    //    flushICache();

    verify_if_often();
    _needsCompaction = false;
    // compactDuration = cpuTimer->tickNo - compactTime;
}


void Zone::free( NativeMethod *nm ) {
    verify_if_often();
    if ( LRUhand == nm ) {
        LRUhand = next_nm( nm );
    }
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


std::int32_t retirementAge( NativeMethod *n ) {
    std::int32_t delta = LRU_RETIREMENT_AGE;
    if ( n->instructionsLength() <= LRU_CUTOFF )
        delta = max( delta, LRU_RETIREMENT_AGE * LRU_SMALL_BOOST );
    return n->lastUsed() + delta;
}


void Zone::verify() {
    _methodTable->verify();
    std::int32_t n = 0;
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
    std::int32_t n, insts, locs, scopes;
public:
    nmsizes() {
        n = insts = locs = scopes = 0;
    }


    std::int32_t total() {
        return n * sizeof( NativeMethod ) + insts + locs + scopes;
    }


    bool isEmpty() {
        return n == 0;
    }


    void print( const char *name, nmsizes &tot ) {
        std::int32_t bigTotal = tot.total();
        std::int32_t myTotal  = total();
        if ( not isEmpty() ) {
            _console->print( "%-13s (%ld methods): ", name, n );
            NMLINE( "headers = %ld (%2.0f%%/%2.0f%%); ", n * sizeof( NativeMethod ), myTotal, bigTotal );
            NMLINE( "instructions = %ld (%2.0f%%/%2.0f%%);\n", insts, myTotal, bigTotal );
            NMLINE( "\tlocs = %ld (%2.0f%%/%2.0f%%); ", locs, myTotal, bigTotal );
            NMLINE( "scopes = %ld (%2.0f%%/%2.0f%%);\n", scopes, myTotal, bigTotal );
            NMLINE( "\ttotal = %ld (%2.0f%%/%2.0f%%)\n", myTotal, myTotal, bigTotal );
        }
    }


    void print( const char *title, std::int32_t t ) {
        spdlog::info( "   n [%3d] title[{}] nativeMethods %2d%% = [%4dK], hdr [%2d%%], inst [%2d%%], locs [%2d%%], debug [%2d%%]", n, title, total() * 100 / t, total() / 1024, n * sizeof( NativeMethod ) * 100 / total(), insts * 100 / total(), locs * 100 / total(), scopes * 100 / total() );
    }


    void add( NativeMethod *nm ) {
        n++;
        insts += nm->instructionsLength();
        locs += nm->locsLen();
        scopes += nm->scopes()->length();
    }
};


void Zone::print() {
    nmsizes      nms;
    nmsizes      zombies;
    std::int32_t uncommon = 0;

    FOR_ALL_NMETHODS( p ) {
        if ( p->isZombie() ) {
            zombies.add( p );
        } else {
            nms.add( p );
            if ( p->isUncommonRecompiled() )
                uncommon++;
        }
    }

    spdlog::info( "Zone:" );

    if ( not nms.isEmpty() ) {
        spdlog::info( "  Code ({}K, {}%% used)", _methodHeap->capacity() / 1024, ( _methodHeap->usedBytes() * 100 ) / _methodHeap->capacity() );
        nms.print( "live", _methodHeap->capacity() );
    }
    if ( uncommon ) {
        spdlog::info( "({} live uncommon nativeMethods)", uncommon );
    }
    if ( not zombies.isEmpty() ) {
        zombies.print( "dead", _methodHeap->capacity() );
        spdlog::info( "  PICs ({}K, {}%% used)", _picHeap->capacity() / 1024, ( _picHeap->usedBytes() * 100 ) / _picHeap->capacity() );
    }

    std::int32_t n     = 0;
    std::int32_t insts = 0;
    FOR_ALL_PICS( pic ) {
        n++;
        insts += pic->code_size();
    }
    std::int32_t total = insts + n * sizeof( PolymorphicInlineCache );

    if ( n > 0 ) {
        spdlog::info( "   %3d entries = {}K (hdr %2d%%, inst %2d%%)", n, total / 1024, n * sizeof( PolymorphicInlineCache ) * 100 / total, insts * 100 / total );
    }
}


struct nm_hist_elem {
    NativeMethod *nm;
    std::int32_t count;
    std::int32_t size;
    std::int32_t sic_count;
    std::int32_t sic_size;
};


static std::int32_t compareOop( const void *m1, const void *m2 ) {
    ResourceMark rm;
    const auto   *nativeMethod1 = reinterpret_cast<const struct nm_hist_elem *>( m1 );
    const auto   *nativeMethod2 = reinterpret_cast<const struct nm_hist_elem *>( m2 );
    return nativeMethod2->nm->method() - nativeMethod1->nm->method();
}


static std::int32_t compareCount( const void *m1, const void *m2 ) {
    const auto *nativeMethod1 = reinterpret_cast<const struct nm_hist_elem *>( m1 );
    const auto *nativeMethod2 = reinterpret_cast<const struct nm_hist_elem *>( m2 );
    return nativeMethod2->count - nativeMethod1->count;
}


void Zone::print_NativeMethod_histogram( std::int32_t size ) {
#ifdef NOT_IMPLEMENTED
    ResourceMark resourceMark;
    nm_hist_elem* hist_array = NEW_RESOURCE_ARRAY(nm_hist_elem, numberOfNativeMethods());

    std::int32_t*  compiled_nativeMethods = NEW_RESOURCE_ARRAY(std::int32_t, numberOfNativeMethods());

    std::int32_t n = 0;
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

    for (std::int32_t i = 0; i < n; i++) compiled_nativeMethods[i] = 0;

    std::int32_t i = 0;
    std::int32_t out = 0;
    while (i < n) {
      std::int32_t counter     = 1;
      std::int32_t all_size    = 0;
      std::int32_t sic_counter = 0;
      std::int32_t sic_size    = 0;

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

    spdlog::info("\n# nm \t # methods \t%% acc.\n");
    std::int32_t nm_count = 0;
    for (std::int32_t i = 0; i < n; i++) {
      if (compiled_nativeMethods[i] > 0) {
        nm_count += i * compiled_nativeMethods[i];
        spdlog::info("%5ld \t%5ld \t\t%3ld %%\n", i, compiled_nativeMethods[i],
           (nm_count*100)/n);

      }
    }

    spdlog::info( "\nList of methods with more than %d nativeMethods compiled.\n", size);
    spdlog::info( " ALL(#,Kb)  Compiler(#,Kb) Method:\n");
    for (std::int32_t i = 0; i < out; i++) {
      spdlog::info("%4d,%-4d   %4d,%-4d ",
         hist_array[i].count,     hist_array[i].size / 1024,
         hist_array[i].sic_count, hist_array[i].sic_size / 1024);
      printName((MethodKlass*) hist_array[i].nm->method()->klass(),
             hist_array[i].nm->key.selector);
      spdlog::info("\n");
    }

    fflush(stdout);
#endif
}


bool Zone::isDeltaPC( void *p ) const {
    return _methodHeap->contains( p ) or _picHeap->contains( p );
}


NativeMethod *Zone::findNativeMethod( const void *start ) const {
    NativeMethod *n{ nullptr};
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

            for ( std::int32_t index = 0; index < nms->length(); index++ ) {
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


std::int32_t Zone::instsSize() {
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
std::int32_t Zone::sweeper( std::int32_t maxVisit, std::int32_t maxReclaim, std::int32_t *nvisited, std::int32_t *nbytesReclaimed ) {
#ifdef NOT_IMPLEMENTED
    EventMarker  em( "LRU sweep" );
    ResourceMark resourceMark;

    timer tmr;
    std::int32_t   visited  = 0;
    std::int32_t   nused    = 0;
    std::int32_t   nbytes   = 0;
    std::int32_t   nextTime = LRUtime + LRU_RETIREMENT_AGE;
    NativeMethod * first = first_nm();
    if ( not first ) return -1;

    NativeMethod * p = LRUhand ? LRUhand : first;
    if ( PrintLRUSweep or PrintLRUSweep2 ) {
        spdlog::info( "*starting LRU sweep..." );
        fflush( stdout );
        tmr.start();
    }

    do {
        if ( PrintLRUSweep2 ) spdlog::info( "\n*inspecting 0x{0:x} (id %ld): ", p, p->id );

        if ( ( p->isZombie() or
               p->isDebug() or
               p->codeTableLink.isEmpty() and not p->isUncommon() ) and
             p->frame_chain == nullptr ) {
            // can be flushed - nobody will ever use it again
            if ( PrintLRUSweep2 )
                spdlog::info( " %s; flushed",
                        p->isZombie() ? "zombie":
                        ( p->isDebug() ? "debug" : "unreachable" ) );
            nbytes += iZone->sizeOfBlock( p );
            p->flush();
        } else if ( p->isUsed() ) {
            // has been used
            nused++;
            if ( PrintLRUSweep2 ) {
                spdlog::info( "used" );
                std::int32_t diff = useCount[ p->id ] - p->oldCount;
                if ( diff ) spdlog::info( " %ld times", diff );
            }
            if ( LRUDecayFactor > 1 ) {
                useCount[ p->id ] = std::int32_t( useCount[ p->id ] / LRUDecayFactor );
            }
            p->oldCount                = useCount[ p->id ];
            LRUtable[ p->id ].unused   = true;
            LRUtable[ p->id ].lastUsed = LRUtime;
            if ( p->zoneLink.notEmpty() ) {
                if ( PrintLRUSweep2 ) spdlog::info( "; removed from replCandidates" );
                p->zoneLink.remove();        // no longer a replacement candidate
                nbytes -= iZone->sizeOfBlock( p );
            }
        } else if ( retirementAge( p ) < LRUtime and p->zoneLink.isEmpty() ) {
            if ( PrintLRUSweep2 ) spdlog::info( "added to replCandidates" );
            replCandidates.add( &p->zoneLink );
            nbytes += iZone->sizeOfBlock( p );
        } else {
            std::int32_t age = retirementAge( p );
            if ( age < nextTime ) {
                nextTime = age;
            }
            if ( PrintLRUSweep2 ) {
                spdlog::info( "unused (age %ld)", LRUtime - p->lastUsed() );
                if ( p->zoneLink.notEmpty() )
                    spdlog::info( " already scheduled for replacement" );
            }
        }

        NativeMethod * oldp = p;
        p = next_circular_nm( p );
        if ( p < oldp ) {                // wrap around
            LRUtime++;
            // The LRU scheme will actually fail if LRUtime > 2^16, but that
            // won't happen very often (every 20*LRU_RESOLUTION CPU hours).
            if ( PrintLRUSweep2 ) spdlog::info( "\n*new LRU time: %ld", LRUtime );
        }
    } while ( ++visited < maxVisit and nbytes < maxReclaim and p );

    if ( needsSweep() and LRUDecayFactor > 1 ) {
        // called from timer; decay count stubs
        stubs->decay( LRUDecayFactor );
    }

    LRUhand     = p;
    _needsSweep = false;
    if ( PrintLRUSweep or PrintLRUSweep2 ) {
        spdlog::info( " done: %ld/%ld visits, %ld bytes, %ld ms.\n",
                nused, visited, nbytes, tmr.time() );
        fflush( stdout );
    }
    if ( nvisited ) *nvisited               = visited;
    if ( nbytesReclaimed ) *nbytesReclaimed = nbytes;
    return nextTime;
#endif
    return 0;
}


std::int32_t Zone::LRU_time() {
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
