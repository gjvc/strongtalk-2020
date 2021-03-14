//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/platform/platform.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/runtime/VirtualFrame.hpp"


// This file contains the interface to the vm pretty printer.
//    PrettyPrintStream describes the output media.
//    PrettyPrinter     defined the interface for printing.

// WARNING WARNING WARNING WARNING WARNING WARNING
// The implementation is NOT production quality due
// to time constraints but since it is only intended
// for vm debugging I hope we can live  with it!!!!!
// -- Lars 3/24-95


class PrettyPrintStream : public PrintableResourceObject {
protected:
    std::int32_t _indentation;
    std::int32_t _position;
    bool         _inHighlight;


    void set_highlight( bool value ) {
        _inHighlight = value;
    }


public:
    bool in_highlight() {
        return _inHighlight;
    }


    // creation
    PrettyPrintStream() :
        _indentation{ 0 },
        _position{ 0 },
        _inHighlight{ false } {
    }


    // Indentation
    virtual void indent() = 0;


    void inc_indent() {
        _indentation++;
    }


    void dec_indent() {
        _indentation--;
    }


    std::int32_t position() {
        return _position;
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
    virtual std::int32_t width() = 0;

    virtual std::int32_t remaining() = 0;

    virtual std::int32_t width_of_string( const char *str ) = 0;

    virtual std::int32_t width_of_char( char c ) = 0;

    virtual std::int32_t width_of_space() = 0;

    virtual std::int32_t infinity() = 0;

    // VM printing
    void print();
};


// Default pretty-printer stream
class DefaultPrettyPrintStream : public PrettyPrintStream {
public:
    DefaultPrettyPrintStream() :
        PrettyPrintStream() {
    }


    void indent();

    void print( const char *str );

    void print_char( char c );

    std::int32_t width_of_string( const char *str );

    void space();

    void newline();

    void begin_highlight();

    void end_highlight();


    std::int32_t width() {
        return 100;
    }


    std::int32_t remaining() {
        return width() - _position;
    }


    std::int32_t width_of_char( char c ) {
        st_unused( c ); // unused
        return 1;
    }


    std::int32_t width_of_space() {
        return 1;
    }


    std::int32_t infinity() {
        return width();
    }
};


class ByteArrayPrettyPrintStream : public DefaultPrettyPrintStream {
private:
    GrowableArray<std::int32_t> *_buffer;

public:
    ByteArrayPrettyPrintStream();
    virtual ~ByteArrayPrettyPrintStream() = default;
    ByteArrayPrettyPrintStream( const ByteArrayPrettyPrintStream & ) = default;
    ByteArrayPrettyPrintStream &operator=( const ByteArrayPrettyPrintStream & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void newline();

    void print_char( char c );

    ByteArrayOop asByteArray();
};

// Pretty printing is done by first constructing a pseudo abstract syntax tree
// based on the byte codes of the method and then printing the AST.
// A simple ad-hoc strategy is used for splitting!


class PrettyPrinter : AllStatic {

public:
    // Pretty prints the method with the byteCodeIndex highlighted.
    // If output is nullptr a default stream is used.
    static void print( std::int32_t index, DeltaVirtualFrame *fr, PrettyPrintStream *output = nullptr );

    static void print_body( DeltaVirtualFrame *fr, PrettyPrintStream *output = nullptr );

    static void print( MethodOop method, KlassOop klass = nullptr, std::int32_t byteCodeIndex = -1, PrettyPrintStream *output = nullptr );

    // Pretty prints the method with the byteCodeIndex highlighted into a byteArray.
    static ByteArrayOop print_in_byteArray( MethodOop method, KlassOop klass = nullptr, std::int32_t byteCodeIndex = -1 );
};
