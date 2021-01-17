
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/macros.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"


// -----------------------------------------------------------------------------

bool_t BeingDebugged = false;


// -----------------------------------------------------------------------------

class BooleanFlag {

    public:
        const char * _name;
        bool_t     * _value;
        bool_t _default;
        const char * _description;


        bool operator==( const bool_t & rhs ) const {
            return *_value == rhs;
        }


        const BooleanFlag & operator=( const bool_t & rhs ) {
            *_value = rhs;
            return *this;
        }

};

#define MATERIALIZE_BOOLEAN_FLAG( name, value, doc ) \
  bool_t name = value;

APPLY_TO_BOOLEAN_FLAGS( MATERIALIZE_BOOLEAN_FLAG )


#define MATERIALIZE_BOOLEAN_FLAG_STRUCT( name, value, doc ) \
  { XSTR(name), &name, value, doc },


static BooleanFlag booleanDebugFlags[] = {
    APPLY_TO_BOOLEAN_FLAGS( MATERIALIZE_BOOLEAN_FLAG_STRUCT ) { 0, nullptr, false, nullptr } // indicates end of table
};


class IntegerFlag {
    public:
        const char * _name;
        int        * _value;
        int _default;
        const char * _description;


        bool operator==( const bool_t & rhs ) const {
            return *_value == rhs;
        }


        const IntegerFlag & operator=( const bool_t & rhs ) {
            *_value = rhs;
            return *this;
        }
};

#define MATERIALIZE_INTEGER_FLAG( name, value, doc ) \
  int name = value;

APPLY_TO_INTEGER_FLAGS( MATERIALIZE_INTEGER_FLAG )


#define MATERIALIZE_INTEGER_FLAG_STRUCT( name, value, doc ) { XSTR(name), &name, value, doc },

static IntegerFlag integerDebugFlags[] = {
    APPLY_TO_INTEGER_FLAGS( MATERIALIZE_INTEGER_FLAG_STRUCT ) { 0, nullptr, 0, nullptr } // indicates end of table
};


// -----------------------------------------------------------------------------

bool_t str_equal( const char * s, const char * q, int len ) {
    // s is null terminated, q is not!
    if ( strlen( s ) not_eq ( uint32_t ) len )
        return false;
    return strncmp( s, q, len ) == 0;
}


bool_t debugFlags::boolAt( const char * name, int len, bool_t * value ) {
    for ( BooleanFlag * current = &booleanDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            *value = *current->_value;
            return true;
        }
    }
    return false;
}


bool_t debugFlags::boolAtPut( const char * name, int len, bool_t * value ) {

    if ( str_equal( "m", name, len ) ) {
        TraceOopPrims             = *value;
        TraceDoublePrims          = *value;
        TraceByteArrayPrims       = *value;
        TraceDoubleByteArrayPrims = *value;
        TraceObjArrayPrims        = *value;
        TraceSmiPrims             = *value;
        TraceProxyPrims           = *value;
        TraceBehaviorPrims        = *value;
        TraceBlockPrims           = *value;
        TraceDebugPrims           = *value;
        TraceSystemPrims          = *value;
        TraceProcessPrims         = *value;
        TraceCallBackPrims        = *value;
        TraceMethodPrims          = *value;
        TraceMixinPrims           = *value;

        *value = not*value;

        return true;
    }

    for ( BooleanFlag * current = &booleanDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            bool_t old_value = *current->_value;
            *current->_value = *value;
            *value           = old_value;
            return true;
        }
    }

    return false;
}


bool_t debugFlags::intAt( const char * name, int len, int * value ) {
    for ( IntegerFlag * current = &integerDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            *value = *current->_value;
            return true;
        }
    }
    return false;
}


bool_t debugFlags::intAtPut( const char * name, int len, int * value ) {
    for ( IntegerFlag * current = &integerDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            int old_value = *current->_value;
            *current->_value = *value;
            *value           = old_value;
            return true;
        }
    }
    return false;
}


// -----------------------------------------------------------------------------

void debugFlags::printFlags() {

    for ( BooleanFlag * b = &booleanDebugFlags[ 0 ]; b->_name; b++ )
        lprintf( "%30s = %s\n", b->_name, *b->_value ? "true" : "false" );

    for ( IntegerFlag * i = &integerDebugFlags[ 0 ]; i->_name; i++ )
        lprintf( "%30s = 0x%08x\n", i->_name, *i->_value );
}


void debugFlags::print_on( ConsoleOutputStream * stream ) {
    // Boolean flags
    for ( BooleanFlag * b = &booleanDebugFlags[ 0 ]; b->_name; b++ )
        stream->print_cr( "%s%s", *b->_value ? "+" : "-" );

    // Integer flags
    for ( IntegerFlag * i = &integerDebugFlags[ 0 ]; i->_name; i++ )
        stream->print_cr( "%s=0x%08x", i->_name, *i->_value );
}


void print_diff_on( ConsoleOutputStream * stream ) {

    // Boolean flags
    for ( BooleanFlag * b = &booleanDebugFlags[ 0 ]; b->_name; b++ ) {
        if ( *b->_value not_eq b->_default )
            stream->print_cr( "%s%s", *b->_value ? "+" : "-" );
    }

    // Integer flags
    for ( IntegerFlag * i = &integerDebugFlags[ 0 ]; i->_name; i++ ) {
        if ( *i->_value not_eq i->_default )
            stream->print_cr( "%s=0x%08x", i->_name, *i->_value );
    }
}
