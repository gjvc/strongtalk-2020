//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#pragma once

#include <iostream>
#include <string_view>


// -----------------------------------------------------------------------------

extern "C" {
void error( const char *m, ... );               //
void warning( const char *m, ... );             //
void compiler_warning( const char *m, ... );    //
void breakpoint();                               // called at every warning
void error_breakpoint();                         // called at every error or fatal
}


// -----------------------------------------------------------------------------

//#define STRINGIFY( x ) #x

#define st_assert( expression, message )                                        \
    if ( not ( expression ) ) {                                                 \
        report_assertion_failure( #expression, __FILE__, __LINE__, message );   \
        breakpoint();                                                           \
    }


// -----------------------------------------------------------------------------

#define st_assert_is_type( obj, t, msg )       st_assert( CONC( Oop( obj )->is_, t() ), msg )
#define st_assert_smi( obj, msg )              st_assert_is_type( obj, smi, msg )
#define st_assert_objArray( obj, msg )         st_assert_is_type( obj, objArray, msg )
#define st_assert_byteArray( obj, msg )        st_assert_is_type( obj, byteArray, msg )
#define st_assert_doubleByteArray( obj, msg )  st_assert_is_type( obj, doubleByteArray,msg )
#define st_assert_doubleValueArray( obj, msg ) st_assert_is_type( obj, doubleValueArray, msg )
#define st_assert_symbol( obj, msg )           st_assert_is_type( obj, symbol, msg )
#define st_assert_double( obj, msg )           st_assert_is_type( obj, double, msg )
#define st_assert_oop_aligned( p )             st_assert( reinterpret_cast<std::int32_t>(p) % 4 == 0, "not word aligned" )


// -----------------------------------------------------------------------------

#define assert_mark( obj, msg )             st_assert_is_type( obj, mark, msg )
#define assert_mem( obj, msg )              st_assert_is_type( obj, mem, msg )
#define assert_block( obj, msg )            st_assert_is_type( obj, block, msg )
#define assert_klass( obj, msg )            st_assert_is_type( obj, klass, msg )
#define assert_process( obj, msg )          st_assert_is_type( obj, process, msg )


// -----------------------------------------------------------------------------

#define st_fatal( m )                                      { report_fatal( __FILE__, __LINE__, m                                     ); error_breakpoint(); }
#define st_fatal1( m, x1 )                                 { report_fatal( __FILE__, __LINE__, m, x1                                 ); error_breakpoint(); }
#define st_fatal2( m, x1, x2 )                             { report_fatal( __FILE__, __LINE__, m, x1, x2                             ); error_breakpoint(); }
#define st_fatal3( m, x1, x2, x3 )                         { report_fatal( __FILE__, __LINE__, m, x1, x2, x3                         ); error_breakpoint(); }
#define st_fatal4( m, x1, x2, x3, x4 )                     { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4                     ); error_breakpoint(); }
#define st_fatal5( m, x1, x2, x3, x4, x5 )                 { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5                 ); error_breakpoint(); }
#define st_fatal6( m, x1, x2, x3, x4, x5, x6 )             { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6             ); error_breakpoint(); }
#define st_fatal7( m, x1, x2, x3, x4, x5, x6, x7 )         { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6, x7         ); error_breakpoint(); }
#define st_fatal8( m, x1, x2, x3, x4, x5, x6, x7, x8 )     { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6, x7, x8     ); error_breakpoint(); }
#define st_fatal9( m, x1, x2, x3, x4, x5, x6, x7, x8, x9 ) { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6, x7, x8, x9 ); error_breakpoint(); }


// -----------------------------------------------------------------------------

void report_assertion_failure( const char *code_str, const char *file_name, std::int32_t line_no, const char *message );

void report_fatal( const char *file_name, std::int32_t line_no, const char *format, ... );

void report_should_not_call( const char *file_name, std::int32_t line_no );

void report_should_not_reach_here( const char *file_name, std::int32_t line_no );

void report_subclass_responsibility( const char *file_name, std::int32_t line_no );

void report_unimplemented( const char *file_name, std::int32_t line_no );

void report_vm_state();

void report_error( const char *title, const char *format, ... );


// -----------------------------------------------------------------------------

#define guarantee( b, msg )      { if ( not ( b ) ) st_fatal( msg ); }
#define ShouldNotCallThis()      { report_should_not_call        ( __FILE__, __LINE__ ); error_breakpoint(); }
#define ShouldNotReachHere()     { report_should_not_reach_here  ( __FILE__, __LINE__ ); error_breakpoint(); }
#define SubclassResponsibility() { report_subclass_responsibility( __FILE__, __LINE__ ); error_breakpoint(); }
#define Unimplemented()          { report_unimplemented          ( __FILE__, __LINE__ ); error_breakpoint(); }
