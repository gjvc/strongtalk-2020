
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/arguments.hpp"

#include <fstream>


const char *image_basename = "strongtalk.bst";
const char *rc_basename    = ".strongtalkrc";


static void set_bool_flag( const char *name, bool_t value ) {
    bool_t s = value;
    if ( not debugFlags::boolAtPut( name, &s ) )
        fprintf( stderr, "Boolean flag [%s] unknown.\n", name );
}


static void set_int_flag( const char *name, int value ) {
    int v = value;
    if ( not debugFlags::intAtPut( name, &v ) )
        fprintf( stderr, "Integer flag [%s] unknown.\n", name );
}


static void process_token( const char *token ) {
    if ( token[ 0 ] == '-' )
        set_bool_flag( &token[ 1 ], false );
    else if ( token[ 0 ] == '+' )
        set_bool_flag( &token[ 1 ], true );
    else {
        char name[100];
        int  value;
        if ( sscanf( token, "%[a-zA-Z]=%d", name, &value ) == 2 ) {
            set_int_flag( name, value );
        }
    }
}


void process_settings_file( const char *file_name, bool_t quiet ) {

    std::ifstream stream( file_name, std::fstream::binary );

    if ( not stream.good() ) {
        if ( quiet )
            return;
        fprintf( stderr, "Could not open %s\n", file_name );
        exit( EXIT_FAILURE );
    }

    char token[1024];
    int  pos = 0;

    bool_t in_white_space = true;
    bool_t in_comment     = false;

    int c = stream.get();
    while ( c not_eq EOF ) {
        if ( in_white_space ) {
            if ( in_comment ) {
                if ( c == '\n' )
                    in_comment = false;
            } else {
                if ( c == '#' )
                    in_comment = true;
                else if ( not isspace( c ) ) {
                    in_white_space = false;
                    token[ pos++ ] = c;
                }
            }
        } else {
            if ( isspace( c ) ) {
                token[ pos ] = '\0';
                process_token( token );
                pos            = 0;
                in_white_space = true;
            } else {
                token[ pos++ ] = c;
            }
        }
        c = stream.get();
    }
    if ( pos > 0 ) {
        token[ pos ] = '\0';
        process_token( token );
    }
    stream.close();
}


void print_credits() {
    // string minimaly encrypted to make it more difficult to
    // tamper with it by looking for it in the executable...
    const char credits[] = "\
\x11\x5a\x04\x1e\x7a\x76\x08\x38\x0b\x6e\x5b\x50\x1e\x04\x2e\x34\x6b\x21\x47\x2e\
\x73\x57\x41\x7b\x65\x18\x3c\x54\x03\x2e\x79\x0e\x1a\x49\x17\x1f\x2f\x44\x45\x73\
\x75\x21\x7e\x5c\x00\x11\x6a\x53\x5b\x3a\x43\x3e\x7e\x5b\x08\x29\x1a\x4c\x1d\x53\
\x13\x22\x73\x0e\x1a\x49\x1f\x1e\x2d\x5b\x05\x37\x0b\x6e\x49\x4b\x01\x45\x02\x7b\
\x4c\x3f\x5c\x21\x7e\x3c\
";
    int        mask      = 0xa729b65d;
    for ( int  i         = 0; i < sizeof( credits ) - 1; i++ ) {
        fputc( ( credits[ i ] ^ mask ) & 0x7f, stdout );
        mask = ( mask << 1 ) | ( mask >> 31 ) & 1; // rotate mask
    }
}


void parse_arguments( int argc, char *argv[] ) {
    bool_t parse_files = true;

    if ( argc > 1 and strcmp( argv[ 1 ], "-t" ) == 0 ) {
        fprintf( stdout, "Timers turned off, flags file and -f arguments are ignored.\n" );
        UseTimers   = false;
        EnableTasks = false;
        parse_files = false;
    }

    if ( parse_files ) {
        process_settings_file( rc_basename, true );
    }

    for ( std::size_t i = parse_files ? 1 : 2; i < argc; i++ ) {
        if ( strcmp( argv[ i ], "-?" ) == 0 ) {
            debugFlags::printFlags();
            exit( EXIT_SUCCESS );

        } else if ( strcmp( argv[ i ], "-credits" ) == 0 ) {
            print_credits();
            exit( EXIT_SUCCESS );

        } else if ( strcmp( argv[ i ], "-b" ) == 0 ) {
            i++;
            if ( i >= argc ) {
                fprintf( stderr, "file name expected after '-b'\n" );
                exit( EXIT_FAILURE );
            }
            image_basename = argv[ i ];

        } else if ( strcmp( argv[ i ], "-f" ) == 0 ) {
            i++;
            if ( i >= argc ) {
                fprintf( stderr, "file name expected after '-f'\n" );
                exit( EXIT_FAILURE );
            }
            if ( parse_files ) {
                process_settings_file( argv[ i ], true );
            }

        } else if ( strcmp( argv[ i ], "-script" ) == 0 ) {
            // The script file name is read and processed by Smalltalk
            // code, not here.  Here we just recognize it and skip over it.
            i++;
            if ( i >= argc ) {
                fprintf( stderr, "file name expected after '-script'\n" );
                exit( EXIT_FAILURE );
            }

        } else if ( strcmp( argv[ i ], "-benchmark" ) == 0 ) {
            // signals to ignore the rest of the command line, which will be
            // interpreted by Smalltalk code as benchmark commands.
            return;
        } else
            process_token( argv[ i ] );
    }
}
