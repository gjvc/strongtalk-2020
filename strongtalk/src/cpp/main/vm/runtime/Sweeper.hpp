//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/memory/Space.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/WaterMark.hpp"

// This is where the dirty work takes place.

// A Sweeper is an incremental cleaner for Decaying invocation counters and cleanup inline caches in methodOops (MethodSweeper).

// -----------------------------------------------------------------------------

class Sweeper : public CHeapAllocatedObject {

private:
    static Sweeper *_head;


    static Sweeper *head() {
        return _head;
    }


    static std::int32_t _sweepSeconds;
    static bool         _isRunning;
    static MethodOop    _activeMethod;
    static NativeMethod *_activeNativeMethod;

public:
    static bool register_active_frame( Frame fr );

    static void clear_active_frame();

    static void print_all();

    static void step_all();

    static void add( Sweeper *sweeper );


    static MethodOop active_method() {
        return _activeMethod;
    }


    static NativeMethod *active_nativeMethod() {
        return _activeNativeMethod;
    }


    // Tells is the sweeper is running
    static bool is_running() {
        return _isRunning;
    }


protected:
    Sweeper *_next;


    Sweeper *next() const {
        return _next;
    }


    std::int32_t _sweep_start;      // time of last activation
    bool         _is_active;        // are we waiting to do something?


    bool is_active() const {
        return _is_active;
    }


    void set_active( bool value ) {
        _is_active = value;
    }


    virtual void step();

    virtual void activate();

    virtual void deactivate();

    virtual std::int32_t interval() const = 0;

    virtual const char *name() const = 0;

public:
    Sweeper();

    virtual void task() = 0;

    void print() const;
};


// -----------------------------------------------------------------------------


// Sweeps through the heap for cleaning blockOops
class HeapSweeper : public Sweeper {

private:
    OldWaterMark _oldWaterMark;

private:
    void task();

    void activate();


    std::int32_t interval() const {
        return HeapSweeperInterval;
    }


    const char *name() const {
        return "HeapSweeper";
    }


public:
    HeapSweeper() :
        _oldWaterMark{} {
    }

};


class CodeSweeper : public Sweeper {
protected:
    std::int32_t _codeSweeperInterval;    // time interval (sec) between starting zone sweep; computed from half-life time
    double       _decayFactor;            // decay factor for invocation counts
    std::int32_t _oldHalfLifeTime;        // old half-life time (to detect changes in half-life setting)
    std::int32_t _fractionPerTask;        // a task invocation does (1 / fractionPerTask) of the entire work

    void updateInterval();          // check for change in half-life setting

public:
    CodeSweeper() : _codeSweeperInterval( 1 ), _decayFactor( 1 ), _oldHalfLifeTime( -1 ), _fractionPerTask( -1 ) {

    }


    std::int32_t interval() const;
};


// -----------------------------------------------------------------------------

// Sweeps through the methods and decay counters and cleanup inline caches.
// Traverses all methodOops by traversing the system dictionary.
class MethodSweeper : public CodeSweeper {

private:
    std::int32_t _index; // next index in systemDictionary to process

private:
    MethodOop excluded_method() {
        return Universe::sweeper_method();
    }


    void set_excluded_method( MethodOop method ) {
        Universe::set_sweeper_method( method );
    }


    void task();

    void method_task( MethodOop method );

    std::int32_t method_dict_task( ObjectArrayOop methods );

    std::int32_t klass_task( KlassOop klass );

    void activate();


    const char *name() const {
        return "MethodSweeper";
    }


    friend class Recompilation;
};


// -----------------------------------------------------------------------------

extern MethodSweeper *methodSweeper;      // single instance

class ZoneSweeper : public CodeSweeper {

private:
    NativeMethod *_excluded_nativeMethod;
    NativeMethod *next;

private:
    NativeMethod *excluded_nativeMethod() {
        return _excluded_nativeMethod;
    }


    void set_excluded_nativeMethod( NativeMethod *nm ) {
        _excluded_nativeMethod = nm;
    }


    void nativeMethod_task( NativeMethod *nm );

    void task();

    void activate();


    const char *name() const {
        return "ZoneSweeper";
    }


public:
    ZoneSweeper() :
        _excluded_nativeMethod{ nullptr },
        next{ nullptr } {
    }

};
