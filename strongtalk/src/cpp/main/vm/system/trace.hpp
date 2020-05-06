//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <iostream>


template <
typename FIRST
>
void write_debug_output( std::ostream
& out, FIRST const & f ) {
out <<
f;
}


struct tracer {

    std::ostream &
    _out;
    tracer( std::ostream
    & out,
    char const * file,
    int line
    ) :
    _out( out ) { out << file << ":" << line << ": "; }
    ~


    tracer() { _out << std::endl; }


    template <
    typename FIRST, typename
    ... REST>
    void write( FIRST const
    & f, REST const & ... rest ) {
        write_debug_output( _out, f );
        _out << " ";
        write( rest... );
    } template <
    typename FIRST
    >
    void write( FIRST const
    & f ) { write_debug_output( _out, f ); }


    void write() {
        // handle the empty params case
    }

};

#define ST_TRACE_PRIMITIVES( ... ) tracer( std::cout, __FILE__, __LINE__ ).write( __VA_ARGS__ )

#if 0
void write_debug_output(std::ostream &out, my_object const &f) {
    out << "**Mine**";
}


int main() {
    my_object a;
    ST_TRACE_PRIMITIVES(1, a, "okay");
    ST_TRACE_PRIMITIVES(5.6, my_object());
}
#endif
