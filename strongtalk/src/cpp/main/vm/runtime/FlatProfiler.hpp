
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"


enum class TickPosition {
    in_code,        //
    in_primitive,   //
    in_compiler,    //
    in_pic,         //
    other           //
};


class TickCounter {
public:
    std::int32_t ticks_in_code;
    std::int32_t ticks_in_primitives;
    std::int32_t ticks_in_compiler;
    std::int32_t ticks_in_pics;
    std::int32_t ticks_in_other;


    TickCounter();


    std::int32_t total() const;


    void add( TickCounter *a );


    void update( TickPosition where );


    void print_code( ConsoleOutputStream *stream, std::int32_t total_ticks ) const;


    void print_other( ConsoleOutputStream *stream ) const;
};


class ProfiledNode : public CHeapAllocatedObject {

private:
    ProfiledNode *_next;

public:
    TickCounter ticks;

public:
    ProfiledNode();
    virtual ~ProfiledNode();
    ProfiledNode( const ProfiledNode & ) = default;
    ProfiledNode &operator=( const ProfiledNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    void set_next( ProfiledNode *n );


    ProfiledNode *next() const;


    void update( TickPosition where );


    std::int32_t total_ticks() const {
        return ticks.total();
    }


    virtual bool is_interpreted() const {
        return false;
    }


    virtual bool is_compiled() const {
        return false;
    }


    virtual bool match( MethodOop m, KlassOop k ) const {
        st_unused( m ); // unused
        st_unused( k ); // unused
        return false;
    }


    virtual bool match( NativeMethod *nm ) const {
        st_unused( nm ); // unused
        return false;
    }


    void print_receiver_klass_on( ConsoleOutputStream *stream ) const;

    virtual MethodOop method() const = 0;

    virtual KlassOop receiver_klass() const = 0;

    virtual void print_method_on( ConsoleOutputStream *stream ) const;

    virtual void print( ConsoleOutputStream *stream, std::int32_t total_ticks ) const;

    static std::int32_t compare( ProfiledNode **a, ProfiledNode **b );

    static void print_title( ConsoleOutputStream *stream );

    static void print_total( ConsoleOutputStream *stream, TickCounter *t, std::int32_t total, const char *msg );

};


class FlatProfilerTask;

class FlatProfiler : AllStatic {

private:
    static ProfiledNode **_table;
    static std::int32_t _tableSize;

    static DeltaProcess     *_deltaProcess;
    static FlatProfilerTask *_flatProfilerTask;
    static Timer            _timer;

    static std::int32_t _gc_ticks;           // total ticks in GC/scavenge
    static std::int32_t _semaphore_ticks;    //
    static std::int32_t _stub_ticks;         //
    static std::int32_t _compiler_ticks;     // total ticks in compilation
    static std::int32_t _unknown_ticks;      //


    static std::int32_t total_ticks() {
        return _gc_ticks + _semaphore_ticks + _stub_ticks + _unknown_ticks;
    }


    friend class FlatProfilerTask;

    static void interpreted_update( MethodOop method, KlassOop klass, TickPosition where );

    static void compiled_update( NativeMethod *nm, TickPosition where );

    static std::int32_t entry( std::int32_t value );

    static void record_tick_for_running_frame( Frame fr );

    static void record_tick_for_calling_frame( Frame fr );

public:
    static void allocate_table();

    static void reset();

    static void engage( DeltaProcess *p );

    static DeltaProcess *disengage();

    static bool is_active();

    static void print( std::int32_t cutoff );

    static void record_tick();


    static DeltaProcess *process() {
        return _deltaProcess;
    }
};


class InterpretedNode : public ProfiledNode {

private:
    MethodOop _method;
    KlassOop  _receiver_klass;

public:
    InterpretedNode( MethodOop method, KlassOop receiver_klass, TickPosition where ) :
        ProfiledNode(),
        _method{ method },
        _receiver_klass{ receiver_klass } {
        update( where );
    }

    InterpretedNode() = default;
    virtual ~InterpretedNode() = default;
    InterpretedNode( const InterpretedNode & ) = default;
    InterpretedNode &operator=( const InterpretedNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    bool is_interpreted() const {
        return true;
    }


    bool match( MethodOop m, KlassOop k ) const {
        return _method == m and _receiver_klass == k;
    }


    MethodOop method() const {
        return _method;
    }


    KlassOop receiver_klass() const {
        return _receiver_klass;
    }


    static void print_title( ConsoleOutputStream *stream ) {
        ProfiledNode::print_title( stream );
    }


    void print( ConsoleOutputStream *stream, std::int32_t total_ticks ) const {
        ProfiledNode::print( stream, total_ticks );
    }
};


class CompiledNode : public ProfiledNode {

private:
    NativeMethod *_nativeMethod;

public:
    CompiledNode( NativeMethod *nm, TickPosition where ) :
        ProfiledNode(),
        _nativeMethod{ nm } {
        update( where );
    }


    CompiledNode() = default;
    virtual ~CompiledNode() = default;
    CompiledNode( const CompiledNode & ) = default;
    CompiledNode &operator=( const CompiledNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    bool is_compiled() const {
        return true;
    }


    bool match( NativeMethod *m ) const {
        return _nativeMethod == m;
    }


    MethodOop method() const {
        return _nativeMethod->method();
    }


    KlassOop receiver_klass() const {
        return _nativeMethod->receiver_klass();
    }


    static void print_title( ConsoleOutputStream *stream ) {
        stream->print( "       Opt" );
        ProfiledNode::print_title( stream );
    }


    void print( ConsoleOutputStream *stream, std::int32_t total_ticks ) const {
        ProfiledNode::print( stream, total_ticks );
    }


    void print_method_on( ConsoleOutputStream *stream ) const {
        if ( _nativeMethod->isUncommonRecompiled() ) {
            stream->print( "Uncommom recompiled " );
        }
        ProfiledNode::print_method_on( stream );
        if ( CompilerDebug ) {
            stream->print( " 0x{0:x} ", _nativeMethod );
        }
    }

};
