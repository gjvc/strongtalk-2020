//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"


class InliningDatabaseKey {

    public:
        LookupKey _outer;
        LookupKey _inner;


        bool_t is_empty() const {
            return _outer.selector_or_method() == smiOop_zero;
        }


        bool_t is_filled() const {
            return SMIOop( _outer.klass() ) not_eq smiOop_zero;
        }


        bool_t is_deleted() const {
            return _outer.selector_or_method() == smiOop_one;
        }


        bool_t is_inner() const {
            return _inner.selector_or_method() not_eq smiOop_zero;
        }


        bool_t is_outer() const {
            return not is_inner();
        }


        void clear() {
            _outer.initialize( nullptr, smiOop_zero );
            _inner.initialize( nullptr, smiOop_zero );
        }


        void set_deleted() {
            _outer.initialize( nullptr, smiOop_one );
        }


        bool_t equal( LookupKey * o, LookupKey * i ) {
            return _outer.equal( o ) and ( is_outer() or _inner.equal( i ) );
        }


        void oops_do( void f( Oop * ) ) {
            if ( is_filled() ) {
                _outer.oops_do( f );
                if ( is_inner() )
                    _inner.oops_do( f );
            }
        }

};

class RecompilationScope;

class InliningDatabase : AllStatic {

    private:
        // Helper functions to compute file path when filing out.
        static const char * selector_string( SymbolOop selector );

        static char * method_string( MethodOop method );

        static char * klass_string( KlassOop klass );

        // Read the scope structure from a stream
        static RecompilationScope * file_in_from( std::ifstream & stream );

        // Helper functions when iterating over zone.
        static int local_number_of_nativeMethods_written;

        static void local_file_out_all( NativeMethod * nm );

        static KlassOop local_klass;

        static void local_file_out_klass( NativeMethod * nm );

        // Returns the file name for the two keys and creates the necessary directories
        static const char * compute_file_name( LookupKey * outer, LookupKey * inner, bool_t create_directories );

        // Returns the file name for the index file
        static const char * index_file_name();

        static const char * _directory;

        static InliningDatabaseKey * _table;
        static uint32_t _table_size;      // Size of table power of 2
        static uint32_t _table_size_mask; // nthMask(table_size)
        static uint32_t _table_no;        // Number of elements in the table

    public:
        // Accessor for the root of the database
        static const char * default_directory();

        static void set_directory( const char * dir );

        static const char * directory();

        // Writes the inlining structure for all compiled code.
        // Returns the number of written inlining structures.
        static bool_t file_out_all();

        // Writes the inlining structure for compiled method.
        // Returns whether the information was written.
        static bool_t file_out( NativeMethod * nm, ConsoleOutputStream * index_st = nullptr );

        // Writes the inlining structure for all compiled methods with a
        // specific receiver klass, returns the number of written structures.
        static int file_out( KlassOop klass );

        // Reads the inlining structure from file_name.
        // Returns nullptr if the attempt failed.
        static RecompilationScope * file_in( const char * file_name );

        // Reads the inlining structure for receiver_class/selector.
        // Returns nullptr if the attempt failed.
        static RecompilationScope * file_in( LookupKey * outer, LookupKey * inner = nullptr );

        // Converts a string into a mangled name that is a valid filename
        // on the running platform.
        static const char * mangle_name( const char * str );

        // Converts a mangled string back to the orignal sting.
        static const char * unmangle_name( const char * str );

        // the lookup table
        static void reset_lookup_table();

        static void add_lookup_entry( LookupKey * outer, LookupKey * inner = nullptr );

        static bool_t lookup( LookupKey * outer, LookupKey * inner = nullptr );

        static RecompilationScope * lookup_and_remove( LookupKey * outer, LookupKey * inner = nullptr );

        static RecompilationScope * select_and_remove( bool_t * end_of_table ); // For background compilation

        // Index file
        static void load_index_file();

        // Iterates through all oops stored in the inlining database
        static void oops_do( void f( Oop * ) );

    private:
        static void allocate_table( uint32_t size );

        static inline uint32_t index_for( LookupKey * outer, LookupKey * inner );

        static inline uint32_t next_index( uint32_t index );
};

