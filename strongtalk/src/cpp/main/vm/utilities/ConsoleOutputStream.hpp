//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <fstream>

#include "vm/runtime/ResourceObject.hpp"
#include "vm/utilities/OutputStream.hpp"


constexpr std::int32_t BUFLEN{ 64 * 1024 };  // max size of output of individual print() methods

extern class ConsoleOutputStream *_console;


class ConsoleOutputStream : public OutputStream {

protected:

    std::int32_t _indentation;   // current indentation
    std::int32_t _width;         // width of the page
    std::int32_t _position;      // position on the current line


public:
    ConsoleOutputStream( std::int32_t width = 80 );
    
//    ConsoleOutputStream() = default;
    virtual ~ConsoleOutputStream() = default;
    ConsoleOutputStream( const ConsoleOutputStream & ) = default;
    ConsoleOutputStream &operator=( const ConsoleOutputStream & ) = default;
    void operator delete( void *ptr ) { (void)ptr; }


    void print( const char *format, ... );

    void print_cr( const char *format, ... );

    void vprint( const char *format, va_list argptr );

    void basic_print( const char *str );


    void print_raw( const char *str ) {
        basic_print( str );
    }


    // indentation
    void indent();


    void dec_cr() {
        dec();
        cr();
    }


    void inc_cr() {
        inc();
        cr();
    }


    void inc() {
        _indentation++;
    };


    void dec() {
        _indentation--;
    };


    std::int32_t indentation() const {
        return _indentation;
    }


    void set_indentation( std::int32_t i ) {
        _indentation = i;
    }


    void vprint_cr( const char *format, va_list argptr );


    void fill_to( std::int32_t col );


    // sizing
    std::int32_t width() const {
        return _width;
    }


    std::int32_t position() const {
        return _position;
    }


    virtual void put( char c );

    virtual void sp();

    virtual void cr();


//        std::ostream & operator << ( std::ostream & out, const std::string & s ) {
//            out << s;
//            return out;
//        }

};
