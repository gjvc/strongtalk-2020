
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Handle.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/utilities/OutputStream.hpp"


PersistentHandle *PersistentHandle::_first{ nullptr };

std::int32_t   Handles::_top{ 0 };
std::int32_t   Handles::_size{ 20 };
Oop Handles::_array[20]{};


void BaseHandle::push() {
    _next = first();
    _prev = nullptr;
    if ( _next ) {
        if ( _log ) {
            char msg[200];
//            sprintf( msg, "unpopped StackHandle '%s->%s' : 0x%0#x -> 0x%0#x", _label, _next->_label, this, _next );
            sprintf( msg, "unpopped StackHandle '%s->%s' : 0x%0#x -> 0x%0#x", _label, _next->_label, reinterpret_cast<unsigned int>(this), reinterpret_cast<unsigned int>(_next) );
            st_assert( (const char *) this < (const char *) _next, msg );
        }
        _next->_prev = this;
    }
    if ( _log ) {
        SPDLOG_INFO( "Pushing handle '%s': 0x{0:x}", _label, static_cast<const void *>(this) );
    }
    setFirst( this );
}


void BaseHandle::pop() {
    if ( _log ) {
        SPDLOG_INFO( "Popping handle '%s': 0x{0:x}", _label, static_cast<const void *>(this) );
    }
    if ( _prev ) {
        _prev->_next = _next;
    } else {
        setFirst( _next );
    }
    if ( _next )
        _next->_prev = _prev;
}


void BaseHandle::oops_do( void f( Oop * ) ) {
    for ( BaseHandle *current = this; current; current = current->_next )
        f( &current->_saved );
}


class FunctionProcessClosure : public ProcessClosure {
private:
    void (*function)( Oop * );

public:
    FunctionProcessClosure( void f( Oop * ) ) : function{ f } {
    }
    virtual ~FunctionProcessClosure() {}

    void operator delete( void *p ) {}



    void do_process( DeltaProcess *p ) {
        if ( p->firstHandle() ) {
            p->firstHandle()->oops_do( function );
        }
    }

};


KlassOop BaseHandle::as_klassOop() {
    return KlassOop( _saved );
}


Oop BaseHandle::as_oop() {
    return _saved;
}


void StackHandle::all_oops_do( void f( Oop * ) ) {
    FunctionProcessClosure closure( f );
    Processes::process_iterate( &closure );
}


BaseHandle *StackHandle::first() {
    return DeltaProcess::active()->firstHandle();
}


void StackHandle::setFirst( BaseHandle *handle ) {
    DeltaProcess::active()->setFirstHandle( handle );
}


StackHandle::StackHandle( Oop toSave, bool log, const char *label ) :
    BaseHandle( toSave, log, label ) {
    push();
}


StackHandle::~StackHandle() {
    pop();
}


PersistentHandle::PersistentHandle( Oop toSave ) :
    _next{ nullptr },
    _prev{ nullptr },
    _saved( toSave ) {
    _next = _first;
    if ( _first ) {
        _first->_prev = this;
    }
    _first = this;
}


PersistentHandle::~PersistentHandle() {
    if ( _prev ) {
        _prev->_next = _next;
    } else {
        _first = _next;
    }
    if ( _next )
        _next->_prev = _prev;
}


void PersistentHandle::oops_do( void f( Oop * ) ) {
    for ( PersistentHandle *current = _first; current; current = current->_next )
        f( &current->_saved );
}


Oop PersistentHandle::as_oop() {
    return _saved;
}


KlassOop PersistentHandle::as_klassOop() {
    return KlassOop( _saved );
}


Oop *PersistentHandle::asPointer() {
    return &_saved;
}


std::int32_t PersistentHandle::savedOffset() {
    return (std::int32_t) &( (PersistentHandle *) nullptr )->_saved;
}


Oop Handles::oop_at( std::int32_t index ) {
    st_assert( index >= 0 and index < top(), "index check" );
    return _array[ index ];
}


std::int32_t Handles::push_oop( Oop value ) {
    st_assert( _top < _size, "bounds check" );
    _array[ _top ] = value;
    return _top++;
}


void Handles::set_top( std::int32_t t ) {
    st_assert( t >= 0 and t < top(), "index check" );
    _top = t;
}


void Handles::oops_do( void f( Oop * ) ) {
    for ( std::int32_t i = 0; i < top(); i++ ) {
        f( &_array[ i ] );
    }
    PersistentHandle::oops_do( f );
    StackHandle::all_oops_do( f );
}


std::int32_t Handles::top() {
    return _top;
}



// -----------------------------------------------------------------------------

HandleMark::HandleMark() :
    _top{ Handles::top() } {
}


HandleMark::~HandleMark() {
    Handles::set_top( _top );
}


// -----------------------------------------------------------------------------

Handle::Handle( Oop value ) :
    _index{} {
    _index = Handles::push_oop( value );
}


Oop Handle::as_oop() {
    return Handles::oop_at( _index );
}


ObjectArrayOop Handle::as_objArray() {
    st_assert( Handles::oop_at( _index )->is_objArray(), "as_objArray() type check" );
    return ObjectArrayOop( Handles::oop_at( _index ) );
}


MemOop Handle::as_memOop() {
    st_assert( Handles::oop_at( _index )->is_mem(), "as_memOop() type check" );
    return MemOop( Handles::oop_at( _index ) );
}


KlassOop Handle::as_klass() {
    SPDLOG_INFO( "klassOop as_klass()[{}]", Handles::oop_at( _index )->toString() );
    st_assert( Handles::oop_at( _index )->is_klass(), "as_klass() type check" );
    return KlassOop( Handles::oop_at( _index ) );
}
