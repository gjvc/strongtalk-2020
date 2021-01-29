//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/OopDescriptor.hpp"

// Interface for preserving memOops across scavenge/gc in primitives.
// ex.
//    HandleMark hm;
//    Handle saved_oop(obj);
//    // ..... call that might cause scavenge/gc .....
//    saved_oop.as_klass()->foo();

//class HandleMark;               //
//class FunctionProcessClosure;   //

extern "C" volatile void *handleCallBack( std::int32_t index, std::int32_t params );

class BaseHandle {

private:
    const char *_label;
    bool     _log;
    Oop        _saved;
    BaseHandle *_next;
    BaseHandle *_prev;

protected:
    void push();

    virtual BaseHandle *first() = 0;

    virtual void setFirst( BaseHandle * ) = 0;

    BaseHandle( Oop toSave, bool log, const char *label );

    void oops_do( void f( Oop * ) );

public:
    void pop();


    Oop *asPointer() {
        return &_saved;
    }


    Oop as_oop();


    KlassOop as_klassOop();


    friend class FunctionProcessClosure;

    friend volatile void *handleCallBack( std::int32_t index, std::int32_t params );
};


class StackHandle : public BaseHandle, StackAllocatedObject {
protected:
    BaseHandle *first();

    void setFirst( BaseHandle *handle );

public:
    StackHandle( Oop toSave, bool log = false, const char *label = "" );

    ~StackHandle();

    static void all_oops_do( void f( Oop * ) );
};


// PersistentHandles can preserve a MemOop without occupying Space in the Handles array and do not require a HandleMark.
// This means they can be used in contexts where a thread switch may occur (eg. surrounding a delta call).
class PersistentHandle : public CHeapAllocatedObject {

private:
    Oop                     _saved;
    PersistentHandle        *_next;
    PersistentHandle        *_prev;
    static PersistentHandle *_first;

public:
    static std::int32_t savedOffset();

    PersistentHandle( Oop toSave );

    ~PersistentHandle();

    Oop as_oop();

    KlassOop as_klassOop();

    Oop *asPointer();

    static void oops_do( void f( Oop * ) );
};


// -----------------------------------------------------------------------------

class HandleMark {

private:
    std::int32_t _top;

public:
    HandleMark();
    ~HandleMark();
};


class Handle : StackAllocatedObject {

private:
    std::int32_t _index;

public:
    Handle( Oop value );

    Oop as_oop();

    ObjectArrayOop as_objArray();

    MemOop as_memOop();

    KlassOop as_klass();
};


// -----------------------------------------------------------------------------

class Handles : AllStatic {

private:
    static std::int32_t _top;
    static std::int32_t _size;
    static Oop         _array[];

    static Oop oop_at( std::int32_t index );

    static std::int32_t push_oop( Oop value );

    static std::int32_t top();

    static void set_top( std::int32_t t );

    friend class HandleMark;

    friend class Handle;

public:
    // Memory management
    static void oops_do( void f( Oop * ) );
};
