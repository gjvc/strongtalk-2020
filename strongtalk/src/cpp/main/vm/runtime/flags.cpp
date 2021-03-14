
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/macros.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


// -----------------------------------------------------------------------------

bool BeingDebugged = false;


// -----------------------------------------------------------------------------

class BooleanFlag {

public:
    const char *_name;
    bool       *_value;
    bool       _default;
    const char *_description;


    bool operator==( const bool &rhs ) const {
        return *_value == rhs;
    }


    const BooleanFlag &operator=( const bool &rhs ) {
        *_value = rhs;
        return *this;
    }

};

#define MATERIALIZE_BOOLEAN_FLAG( name, value, doc ) \
    bool name = value;

APPLY_TO_BOOLEAN_FLAGS( MATERIALIZE_BOOLEAN_FLAG )


#define MATERIALIZE_BOOLEAN_FLAG_STRUCT( name, value, doc ) \
    { XSTR(name), &name, value, doc },


static BooleanFlag booleanDebugFlags[] = {
    APPLY_TO_BOOLEAN_FLAGS( MATERIALIZE_BOOLEAN_FLAG_STRUCT ) { 0, nullptr, false, nullptr } // indicates end of table
};


class IntegerFlag {
public:
    const char   *_name;
    std::int32_t *_value;
    std::int32_t _default;
    const char   *_description;


    bool operator==( const bool &rhs ) const {
        return *_value == rhs;
    }


    const IntegerFlag &operator=( const bool &rhs ) {
        *_value = rhs;
        return *this;
    }
};

#define MATERIALIZE_INTEGER_FLAG( name, value, doc ) \
    std::int32_t name = value;

APPLY_TO_INTEGER_FLAGS( MATERIALIZE_INTEGER_FLAG )

#define MATERIALIZE_INTEGER_FLAG_STRUCT( name, value, doc ) { XSTR(name), &name, value, doc },

static IntegerFlag integerDebugFlags[] = {
    APPLY_TO_INTEGER_FLAGS( MATERIALIZE_INTEGER_FLAG_STRUCT ) { 0, nullptr, 0, nullptr } // indicates end of table
};


// -----------------------------------------------------------------------------

bool str_equal( const char *s, const char *q, std::int32_t len ) {
    // s is null terminated, q is not!
    if ( strlen( s ) not_eq (std::uint32_t) len )
        return false;
    return strncmp( s, q, len ) == 0;
}


bool debugFlags::boolAt( const char *name, std::int32_t len, bool *value ) {
    for ( BooleanFlag *current = &booleanDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            *value = *current->_value;
            return true;
        }
    }
    return false;
}


bool debugFlags::boolAtPut( const char *name, std::int32_t len, bool *value ) {

    if ( str_equal( "m", name, len ) ) {
        TraceOopPrims             = *value;
        TraceDoublePrims          = *value;
        TraceByteArrayPrims       = *value;
        TraceDoubleByteArrayPrims = *value;
        TraceObjectArrayPrims     = *value;
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

    for ( BooleanFlag *current = &booleanDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            bool old_value = *current->_value;
            *current->_value = *value;
            *value           = old_value;
            return true;
        }
    }

    return false;
}


bool debugFlags::intAt( const char *name, std::int32_t len, std::int32_t *value ) {
    for ( IntegerFlag *current = &integerDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            *value = *current->_value;
            return true;
        }
    }
    return false;
}


bool debugFlags::intAtPut( const char *name, std::int32_t len, std::int32_t *value ) {
    for ( IntegerFlag *current = &integerDebugFlags[ 0 ]; current->_name; current++ ) {
        if ( str_equal( current->_name, name, len ) ) {
            std::int32_t old_value = *current->_value;
            *current->_value = *value;
            *value           = old_value;
            return true;
        }
    }
    return false;
}


// -----------------------------------------------------------------------------

void debugFlags::printFlags() {

    for ( BooleanFlag *b = &booleanDebugFlags[ 0 ]; b->_name; b++ )
        SPDLOG_INFO( "%30s = {}", b->_name, *b->_value ? "true" : "false" );

    for ( IntegerFlag *i = &integerDebugFlags[ 0 ]; i->_name; i++ )
        SPDLOG_INFO( "%30s = 0x%08x", i->_name, *i->_value );
}


void debugFlags::print_on( ConsoleOutputStream *stream ) {
    // Boolean flags
    for ( BooleanFlag *b = &booleanDebugFlags[ 0 ]; b->_name; b++ )
        stream->print_cr( "%s%s", *b->_value ? "+" : "-" );

    // Integer flags
    for ( IntegerFlag *i = &integerDebugFlags[ 0 ]; i->_name; i++ )
        stream->print_cr( "%s=0x%08x", i->_name, *i->_value );
}


void print_diff_on( ConsoleOutputStream *stream ) {

    // Boolean flags
    for ( BooleanFlag *b = &booleanDebugFlags[ 0 ]; b->_name; b++ ) {
        if ( *b->_value not_eq b->_default )
            stream->print_cr( "%s%s", *b->_value ? "+" : "-" );
    }

    // Integer flags
    for ( IntegerFlag *i = &integerDebugFlags[ 0 ]; i->_name; i++ ) {
        if ( *i->_value not_eq i->_default )
            stream->print_cr( "%s=0x%08x", i->_name, *i->_value );
    }
}
