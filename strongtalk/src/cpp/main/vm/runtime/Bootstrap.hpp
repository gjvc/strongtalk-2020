
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"

#include <array>
#include <fstream>
#include <map>

// Bootstrap parses and allocates memOops from a file

class Klass;

class Bootstrap : CHeapAllocatedObject {

    private:

        Oop * _oop_table;
        int           _number_of_oops;
        int           _max_number_of_oops;
        bool_t        _new_format;
        int           _objectCount;
        std::string   _filename;
        std::ifstream _stream;

        std::map <char, int>         _countByType{};
        std::map <char, std::string> _nameByTypeByte{};

    public:
        Bootstrap( const std::string & name );
        ~Bootstrap();
        void initNameByTypeByte();

        char readNextChar();

        int get_integer();

        bool_t has_error();

        void read_mark( MarkOop * mark_addr );

        void read_oop( Oop * oop_addr );

        char read_byte();

        uint16_t read_doubleByte();

        int32_t read_integer();

        double read_double();

        bool_t new_format() const;

        bool_t is_byte();

        void klass_case_func( void (* function)( Klass * ), MemOop m );

        template <typename T>
        void object_case_func( MemOop m );

        template <typename T>
        void insert_symbol( MemOop m );

        void object_error_func( const char * str );

        void load();

        void check_version();

        void parse_objects();


        void add( Oop obj );

//        Oop at( int index );


        void open_file();

        void parse_file();

        Oop readNextObject();

        void close_file();
        void extend_oop_table();


        void summary();
        Oop oopFromTable( const int index );
};
