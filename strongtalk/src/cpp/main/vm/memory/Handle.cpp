
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Handle.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/utilities/OutputStream.hpp"


PersistentHandle *PersistentHandle::_first = nullptr;
std::size_t      Handles::_top  = 0;
std::size_t      Handles::_size = 20;
Oop              Handles::_array[20];


BaseHandle::BaseHandle( Oop toSave, bool_t log, const char *label ) :
        _saved( toSave ), _log( log ), _label( label ) {
}


void BaseHandle::push() {
    _next = first();
    _prev = nullptr;
    if ( _next ) {
        if ( _log ) {
            char msg[200];
            sprintf( msg, "unpopped StackHandle '%s->%s' : 0x%x->0x%x", _label, _next->_label, this, _next );
            st_assert( (const char *) this < (const char *) _next, msg );
        }
        _next->_prev = this;
    }
    if ( _log )
        _console->print_cr( "Pushing handle '%s': 0x%x", _label, this );
    setFirst( this );
}


void BaseHandle::pop() {
    if ( _log )
        _console->print_cr( "Popping handle '%s': 0x%x", _label, this );
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
    FunctionProcessClosure( void f( Oop * ) ) {
        function = f;
    }


    void do_process( DeltaProcess *p ) {
        if ( p->firstHandle() )
            p->firstHandle()->oops_do( function );
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


StackHandle::StackHandle( Oop toSave, bool_t log, const char *label ) :
        BaseHandle( toSave, log, label ) {
    push();
}


StackHandle::~StackHandle() {
    pop();
}


PersistentHandle::PersistentHandle( Oop toSave ) :
        _saved( toSave ) {
    _next = _first;
    _prev = nullptr;
    if ( _first )
        _first->_prev = this;
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


int PersistentHandle::savedOffset() {
    return (int) &( (PersistentHandle *) nullptr )->_saved;
}


Oop Handles::oop_at( int index ) {
    st_assert( index >= 0 and index < top(), "index check" );
    return _array[ index ];
}


int Handles::push_oop( Oop value ) {
    st_assert( _top < _size, "bounds check" );
    _array[ _top ] = value;
    return _top++;
}


void Handles::set_top( int t ) {
    st_assert( t >= 0 and t < top(), "index check" );
    _top = t;
}


void Handles::oops_do( void f( Oop * ) ) {
    for ( std::size_t i = 0; i < top(); i++ ) {
        f( &_array[ i ] );
    }
    PersistentHandle::oops_do( f );
    StackHandle::all_oops_do( f );
}


std::size_t Handles::top() {
    return _top;
}



// -----------------------------------------------------------------------------

HandleMark::HandleMark() {
    _top = Handles::top();
}


HandleMark::~HandleMark() {
    Handles::set_top( _top );
}


// -----------------------------------------------------------------------------

Handle::Handle( Oop value ) {
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
    _console->print_cr( "klassOop as_klass() [%s]", Handles::oop_at( _index )->print_string() );
    st_assert( Handles::oop_at( _index )->is_klass(), "as_klass() type check" );
    return KlassOop( Handles::oop_at( _index ) );
}
