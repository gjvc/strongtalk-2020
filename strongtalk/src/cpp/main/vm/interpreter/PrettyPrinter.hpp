//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"


// This file contains the interface to the vm pretty printer.
//    prettyPrintStream describes the output media.
//    PrettyPrinter     defined the interface for printing.

// WARNING WARNING WARNING WARNING WARNING WARNING
// The implementation is NOT production quality due
// to time constraints but since it is only intended
// for vm debugging I hope we can live  with it!!!!!
// -- Lars 3/24-95

class prettyPrintStream : public PrintableResourceObject {
protected:
    int    _indentation;
    int    pos;
    bool_t in_hl;


    void set_highlight( bool_t value ) {
        in_hl = value;
    }


public:
    bool_t in_highlight() {
        return in_hl;
    }


    // creation
    prettyPrintStream() {
        _indentation = pos = 0;
        set_highlight( false );
    }


    // Indentation
    virtual void indent() = 0;


    void inc_indent() {
        _indentation++;
    }


    void dec_indent() {
        _indentation--;
    }


    int position() {
        return pos;
    }


    // Printing
    virtual void print( const char *str ) = 0;

    virtual void print_char( char c ) = 0;

    virtual void newline() = 0;


    void dec_newline() {
        dec_indent();
        newline();
    }


    void inc_newline() {
        inc_indent();
        newline();
    }


    virtual void space() = 0;

    virtual void begin_highlight() = 0;

    virtual void end_highlight() = 0;

    // Layout
    virtual int width() = 0;

    virtual int remaining() = 0;

    virtual int width_of_string( const char *str ) = 0;

    virtual int width_of_char( char c ) = 0;

    virtual int width_of_space() = 0;

    virtual int infinity() = 0;

    // VM printing
    void print();
};

// Default pretty-printer stream
class defaultPrettyPrintStream : public prettyPrintStream {
public:
    defaultPrettyPrintStream() :
            prettyPrintStream() {
    }


    void indent();

    void print( const char *str );

    void print_char( char c );

    int width_of_string( const char *str );

    void space();

    void newline();

    void begin_highlight();

    void end_highlight();


    int width() {
        return 100;
    }


    int remaining() {
        return width() - pos;
    }


    int width_of_char( char c ) {
        return 1;
    }


    int width_of_space() {
        return 1;
    }


    int infinity() {
        return width();
    }
};

class byteArrayPrettyPrintStream : public defaultPrettyPrintStream {
private:
    GrowableArray<int> *_buffer;

public:
    byteArrayPrettyPrintStream();

    void newline();

    void print_char( char c );

    ByteArrayOop asByteArray();
};

// Pretty printing is done by first constructing a pseudo abstract syntax tree
// based on the byte codes of the method and then printing the AST.
// A simple ad-hoc strategy is used for splitting!

class DeltaVirtualFrame;

class PrettyPrinter : AllStatic {

public:
    // Pretty prints the method with the byteCodeIndex highlighted.
    // If output is nullptr a default stream is used.
    static void print( int index, DeltaVirtualFrame *fr, prettyPrintStream *output = nullptr );

    static void print_body( DeltaVirtualFrame *fr, prettyPrintStream *output = nullptr );

    static void print( MethodOop method, KlassOop klass = nullptr, int byteCodeIndex = -1, prettyPrintStream *output = nullptr );

    // Pretty prints the method with the byteCodeIndex highlighted into a byteArray.
    static ByteArrayOop print_in_byteArray( MethodOop method, KlassOop klass = nullptr, int byteCodeIndex = -1 );
};
