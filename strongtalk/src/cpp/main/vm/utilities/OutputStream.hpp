//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/ResourceObject.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/utilities/OutputStream.hpp"

#include <fstream>


// -----------------------------------------------------------------------------

class OutputStream : public ResourceObject {

public:
    OutputStream() : _output{ std::cout } {}


protected:
    const std::ostream &_output;
};


// -----------------------------------------------------------------------------

class ConsoleOutputStream : public OutputStream {

protected:

    int _indentation;   // current indentation
    int _width;         // width of the page
    int _position;      // position on the current line


public:
    ConsoleOutputStream( int width = 80 );
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


    int indentation() const {
        return _indentation;
    }


    void set_indentation( int i ) {
        _indentation = i;
    }


    void vprint_cr( const char *format, va_list argptr );


    void fill_to( int col );


    // sizing
    int width() const {
        return _width;
    }


    int position() const {
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


class StringOutputStream : public ConsoleOutputStream {

protected:
    std::string _string;
    char *buffer;
    int buffer_pos;
    int buffer_length;

public:
    StringOutputStream( const int initial_size = 1 * 1024 );

    void put( char c );

    char *as_string();

    // Conversion into Delta object
    ByteArrayOop as_byteArray();
};


class FileOutputStream : public ConsoleOutputStream {

protected:
    std::ofstream _file;

public:
    FileOutputStream( const char *file_name );
    ~FileOutputStream();


    int is_open() const {
        return _file.good();
    }


    void put( char c );
};


// Standard output
extern ConsoleOutputStream *_console;
