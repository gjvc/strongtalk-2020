//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

extern "C" {
void error( const char * m, ... );               //
void warning( const char * m, ... );             //
void compiler_warning( const char * m, ... );    //
void breakpoint();                              // called at every warning
void error_breakpoint();                        // called at every error or fatal
}

// -----------------------------------------------------------------------------

#define fatal( m )                                      { report_fatal( __FILE__, __LINE__, m                                     ); error_breakpoint(); }
#define fatal1( m, x1 )                                 { report_fatal( __FILE__, __LINE__, m, x1                                 ); error_breakpoint(); }
#define fatal2( m, x1, x2 )                             { report_fatal( __FILE__, __LINE__, m, x1, x2                             ); error_breakpoint(); }
#define fatal3( m, x1, x2, x3 )                         { report_fatal( __FILE__, __LINE__, m, x1, x2, x3                         ); error_breakpoint(); }
#define fatal4( m, x1, x2, x3, x4 )                     { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4                     ); error_breakpoint(); }
#define fatal5( m, x1, x2, x3, x4, x5 )                 { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5                 ); error_breakpoint(); }
#define fatal6( m, x1, x2, x3, x4, x5, x6 )             { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6             ); error_breakpoint(); }
#define fatal7( m, x1, x2, x3, x4, x5, x6, x7 )         { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6, x7         ); error_breakpoint(); }
#define fatal8( m, x1, x2, x3, x4, x5, x6, x7, x8 )     { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6, x7, x8     ); error_breakpoint(); }
#define fatal9( m, x1, x2, x3, x4, x5, x6, x7, x8, x9 ) { report_fatal( __FILE__, __LINE__, m, x1, x2, x3, x4, x5, x6, x7, x8, x9 ); error_breakpoint(); }


// -----------------------------------------------------------------------------

void report_assertion_failure( const char * code_str, const char * file_name, int line_no, const char * message );

void report_fatal( const char * file_name, int line_no, const char * format, ... );

void report_should_not_call( const char * file_name, int line_no );

void report_should_not_reach_here( const char * file_name, int line_no );

void report_subclass_responsibility( const char * file_name, int line_no );

void report_unimplemented( const char * file_name, int line_no );

void report_vm_state();

void report_error( const char * title, const char * format, ... );


// -----------------------------------------------------------------------------

#define guarantee( b, msg )      { if ( not ( b ) ) fatal( msg ); }
#define ShouldNotCallThis()      { report_should_not_call        ( __FILE__, __LINE__ ); error_breakpoint(); }
#define ShouldNotReachHere()     { report_should_not_reach_here  ( __FILE__, __LINE__ ); error_breakpoint(); }
#define SubclassResponsibility() { report_subclass_responsibility( __FILE__, __LINE__ ); error_breakpoint(); }
#define Unimplemented()          { report_unimplemented          ( __FILE__, __LINE__ ); error_breakpoint(); }


// -----------------------------------------------------------------------------


#define STRINGIFY( x ) #x

#define st_assert( expression, message )                                        \
    if ( not ( expression ) ) {                                                 \
        report_assertion_failure( #expression, __FILE__, __LINE__, message );   \
        breakpoint();                                                           \
    }


//#include <experimental/source_location>
//inline void st_assert_x( bool_t condition, const char *msg, const std::experimental::source_location & location = std::experimental::source_location::current() ) {
//    if ( condition ) {
//        return;
//    }
////    report_assertion_failure( XSTR( condition ), location.file_name(), location.line(), msg );
//    breakpoint();
//}
//
//std::experimental::source_location.file_name()
//std::experimental::source_location.line()



// -----------------------------------------------------------------------------

// Bogosity alert:  for non-ANSI C, 'obj' and 't' must NOT be separated by whitespace in either the parameters of the definition or the arguments of the call.

#define assert_is_type( obj, t, msg )       st_assert( CONC( Oop( obj )->is_, t() ), msg )
#define assert_smi( obj, msg )              assert_is_type( obj, smi, msg )
#define assert_objArray( obj, msg )         assert_is_type( obj, objArray, msg )
#define assert_byteArray( obj, msg )        assert_is_type( obj, byteArray, msg )
#define assert_doubleByteArray( obj, msg )  assert_is_type( obj, doubleByteArray,msg )
#define assert_doubleValueArray( obj, msg ) assert_is_type( obj, doubleValueArray, msg )
#define assert_symbol( obj, msg )           assert_is_type( obj, symbol, msg )
#define assert_double( obj, msg )           assert_is_type( obj, double, msg )
#define assert_oop_aligned( p )             st_assert( (int)(p) % 4 == 0, "not word aligned" )


// -----------------------------------------------------------------------------

#define assert_mark( obj, msg )             assert_is_type( obj, mark, msg )
#define assert_mem( obj, msg )              assert_is_type( obj, mem, msg )
#define assert_block( obj, msg )            assert_is_type( obj, block, msg )
#define assert_klass( obj, msg )            assert_is_type( obj, klass, msg )
#define assert_process( obj, msg )          assert_is_type( obj, process, msg )
