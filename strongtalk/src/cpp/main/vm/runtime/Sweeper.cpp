//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Sweeper.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/runtime/PeriodicTask.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/ResourceMark.hpp"


// The sweeper run at real_time ticks. We only swep if the interrupted
// Delta process is in a well-defined state (see SweeperTask).
// We might change the sweeper to sweep at preempt time like in the Self system.

Sweeper *Sweeper::_head = nullptr;

std::int32_t     Sweeper::_sweepSeconds = 0;
bool            Sweeper::_isRunning           = false;
MethodOop       Sweeper::_activeMethod        = nullptr;
NativeMethod    *Sweeper::_activeNativeMethod = nullptr;


void Sweeper::print_all() {
    for ( Sweeper *n = head(); n; n = n->next() )
        n->print();
}


bool Sweeper::register_active_frame( Frame fr ) {
    if ( fr.is_interpreted_frame() ) {
        _activeMethod = fr.method();
        if ( _activeMethod == nullptr )
            return false;
        return true;
    } else if ( fr.is_compiled_frame() ) {
        _activeNativeMethod = findNativeMethod( fr.pc() );
        return true;
    }
    return false;
}


void Sweeper::clear_active_frame() {
    _activeMethod       = nullptr;
    _activeNativeMethod = nullptr;
}


void Sweeper::step_all() {
    _isRunning       = true;
    ResourceMark  rm;
    for ( Sweeper *n = head(); n; n = n->next() )
        n->step();
    _sweepSeconds++;
    _isRunning = false;
}


Sweeper::Sweeper() {
    _is_active   = false;
    _sweep_start = _sweepSeconds;
}


void Sweeper::add( Sweeper *sweeper ) {
    sweeper->_next = head();
    _head = sweeper;
}


void Sweeper::step() {
    if ( interval() == 0 )
        return;

    if ( not is_active() and ( _sweepSeconds - _sweep_start ) >= interval() ) {
        _sweep_start = _sweepSeconds;
        activate();
    }
    if ( is_active() )
        task();
}


void Sweeper::print() const {
    spdlog::info( "%s", name() );
}


void Sweeper::activate() {
    _is_active = true;
    LOG_EVENT1( "Activating %s", name() );
}


void Sweeper::deactivate() {
    _is_active = false;
    LOG_EVENT1( "Deactivating %s", name() );
}

// ---------------- HeapSweeper -----------------

void HeapSweeper::activate() {
    _oldWaterMark = Universe::old_gen.bottom_mark();
    Sweeper::activate();
}


void HeapSweeper::task() {
}

// ---------------- CodeSweeper -----------------

void CodeSweeper::updateInterval() {
    if ( _oldHalfLifeTime not_eq CounterHalfLifeTime ) {
        _oldHalfLifeTime     = CounterHalfLifeTime;
        _codeSweeperInterval = 4;            // for now, use fixed value; could adjust if necessary
        _fractionPerTask     = 8;
        const double log2 = 0.69314718055995;   // log(2)
        _decayFactor = exp( log2 * _codeSweeperInterval * _fractionPerTask / CounterHalfLifeTime );
        if ( PrintCodeSweep )
            spdlog::info( "*method sweep: decay factor %f", _decayFactor );
    }
}


std::int32_t CodeSweeper::interval() const {
    ( (CodeSweeper *) this )->updateInterval();
    return _codeSweeperInterval;
}

// ---------------- MethodSweeper -----------------


void MethodSweeper::method_task( MethodOop method ) {
    if ( method not_eq Sweeper::active_method() ) {
        if ( method->invocation_count() > 0 ) {
            method->decay_invocation_count( _decayFactor );
        }
        method->cleanup_inline_caches();
    } else {
        // Save the NativeMethod for next round
        set_excluded_method( Sweeper::active_method() );
    }
}


std::int32_t MethodSweeper::method_dict_task( ObjectArrayOop methods ) {

    std::int32_t length = methods->length();

    for ( std::int32_t i = 1; i <= length; i++ ) {
        MethodOop method = MethodOop( methods->obj_at( i ) );
        st_assert( method->is_method(), "just checking" );
        method_task( method );
    }
    return length;
}


std::int32_t MethodSweeper::klass_task( KlassOop klass ) {
    std::int32_t result = 0;
    Klass        *k     = klass->klass_part();

    // Fix the customized methods
    result += method_dict_task( k->methods() );
    result += method_dict_task( klass->klass()->klass_part()->methods() );

    if ( k->is_named_class() ) {
        // Fix the mixin parts
        result += method_dict_task( k->mixin()->methods() );
        result += method_dict_task( klass->klass()->klass_part()->mixin()->methods() );
    }

    if ( not k->has_superKlass() )
        return result;

    if ( k->superKlass()->klass_part()->is_named_class() )
        return result;

    // super class is an unnamed class so we have to handle it
    result += klass_task( k->superKlass() );
    return result;
}


void MethodSweeper::task() {
    // Prologue: check is there is leftover from last sweep
    if ( excluded_method() ) {
        MethodOop m = excluded_method();
        set_excluded_method( nullptr );
        method_task( m );
    }

    ObjectArrayOop array             = Universe::systemDictionaryObject();
    std::int32_t   length            = array->length();
    std::int32_t   number_of_entries = length / _fractionPerTask;
    if ( PrintCodeSweep )
        spdlog::info( "*method sweep: {} entries...", number_of_entries );
    TraceTime t( "MethodSweep ", PrintCodeSweep );

    std::int32_t end = ( _index + number_of_entries );
    if ( end > length )
        end = length;

    std::int32_t begin  = _index;
    std::int32_t result = 0;

    for ( ; _index <= end; _index++ ) {
        AssociationOop assoc = AssociationOop( array->obj_at( _index ) );
        st_assert( assoc->is_association(), "just checking" );
        if ( assoc->is_constant() and assoc->value()->is_klass() ) {
            std::int32_t result = klass_task( KlassOop( assoc->value() ) );
        }
    }
    LOG_EVENT3( "MethodSweeper task [%d, %d] #%d", begin, end, result );

    if ( _index > length )
        deactivate();
}


void MethodSweeper::activate() {
    _index = 1;
    Sweeper::activate();
}

// ---------------- ZoneSweeper -----------------

void ZoneSweeper::nativeMethod_task( NativeMethod *nm ) {
    if ( nm not_eq Sweeper::active_nativeMethod() ) {
        nm->sweeper_step( _decayFactor );
    } else {
        // Save the NativeMethod for next round
        set_excluded_nativeMethod( Sweeper::active_nativeMethod() );
    }
}


void ZoneSweeper::task() {
    // Prologue: check is there is leftover from last sweep
    if ( excluded_nativeMethod() ) {
        NativeMethod *nm = excluded_nativeMethod();
        set_excluded_nativeMethod( nullptr );
        nativeMethod_task( nm );
    }

    // %fix this:
    //    we need to validate next
    std::int32_t total = Universe::code->numberOfNativeMethods();
    std::int32_t todo  = total / _fractionPerTask;
    if ( PrintCodeSweep )
        spdlog::info( "*zone sweep: {} of {} entries...", todo, total );
    TraceTime t( "ZoneSweep ", PrintCodeSweep );

    for ( std::int32_t i = 0; i < todo; i++ ) {
        if ( next == nullptr ) {
            deactivate();
            break;
        }
        nativeMethod_task( next );
        next = Universe::code->next_nm( next );
    }

    if ( UseNativeMethodAging ) {
        for ( NativeMethod *nm = Universe::code->first_nm(); nm; nm = Universe::code->next_nm( nm ) ) {
            nm->incrementAge();
        }
    }
}


void ZoneSweeper::activate() {
    if ( Universe::code->numberOfNativeMethods() > 0 ) {
        next                   = Universe::code->first_nm();
        _excluded_nativeMethod = nullptr;
        Sweeper::activate();
    } else {
        deactivate();
    }
}


// The sweeper task is activated every second (1000 milliseconds).
class SweeperTask : public PeriodicTask {
private:
    std::int32_t counter;
public:
    SweeperTask() :
            PeriodicTask( 100 ) {
        counter = 0;
    }


    void task() {
        // If we're idle forget about the tick.
        if ( DeltaProcess::is_idle() )
            return;
        if ( ++counter > 10 ) {
            if ( processSemaphore )
                return;
            if ( last_Delta_fp )
                return;

            if ( Sweeper::register_active_frame( DeltaProcess::active()->profile_top_frame() ) ) {
                Sweeper::step_all();
                Sweeper::clear_active_frame();
                counter -= 10;
            }
        }
    }
};

MethodSweeper *methodSweeper;


void sweeper_init() {
    spdlog::info( "%system-init:  sweeper_init" );

    Sweeper::add( new HeapSweeper() );
    Sweeper::add( new ZoneSweeper() );
    Sweeper::add( methodSweeper = new MethodSweeper() );

    if ( SweeperUseTimer ) {
        SweeperTask *t = new SweeperTask;
        t->enroll();
    }
}
