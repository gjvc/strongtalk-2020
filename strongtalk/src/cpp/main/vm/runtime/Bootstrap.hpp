
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"

#include <array>
#include <fstream>
#include <map>

// Bootstrap parses and allocates memOops from a file

class Klass;

class Bootstrap : CHeapAllocatedObject {

private:

    Oop           *_oop_table;
    std::int32_t  _number_of_oops;
    std::int32_t  _max_number_of_oops;
    bool          _new_format;
    std::int32_t  _objectCount;
    std::string   _filename;
    std::ifstream _stream;

    std::map<char, std::int32_t> _countByType{};
    std::map<char, std::string>  _nameByTypeByte{};

    Bootstrap( const Bootstrap & ); // private copy constructor -> cannot copy [-Weffc++]
    const Bootstrap &operator=( const Bootstrap & ); // private assignment operator -> cannot assign[-Weffc++]

public:
    Bootstrap( const std::string &name );
    ~Bootstrap();
    void initNameByTypeByte();

    char readNextChar();

    std::int32_t get_next_int32_t();

    bool has_error();

    void read_mark( MarkOop *mark_addr );

    void read_oop( Oop *oop_addr );

    char read_byte();

    std::uint16_t read_doubleByte();

    std::int32_t read_integer();

    double read_double();

    bool new_format() const;

    bool is_byte();

    void klass_case_func( void (*function)( Klass * ), MemOop m );

    template<typename T>
    void object_case_func( MemOop m );

    template<typename T>
    void insert_symbol( MemOop m );

    void object_error_func( const char *str );

    void load();

    void check_version();

    void parse_objects();


    void add( Oop obj );

//        Oop at( std::int32_t index );


    void open_file();

    void parse_file();

    Oop readNextObject();

    void close_file();
    void extend_oop_table();


    void summary();
    Oop oopFromTable( const std::int32_t index );
    char getNextTypeByte();
};
