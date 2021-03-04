//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/utility/DebugNotifier.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"

#include <cstdarg>

constexpr std::int32_t TEST_NOTIFIER_BUFLEN = 2048;


class TestNotifier : public Notifier, public ResourceObject {

private:
    GrowableArray<char *> *errors;
    GrowableArray<char *> *warnings;
    GrowableArray<char *> *compilerWarnings;

public:
    TestNotifier() :
    errors{ new GrowableArray<char *> },
    warnings{ new GrowableArray<char *> },
    compilerWarnings{ new GrowableArray<char *> } {
    }


    std::int32_t errorCount() {
        return errors->length();
    }


    std::int32_t warningCount() {
        return warnings->length();
    }


    std::int32_t compilerWarningCount() {
        return compilerWarnings->length();
    }


    char *errorAt( std::int32_t index ) {
        return errors->at( index );
    }


    char *warningAt( std::int32_t index ) {
        return warnings->at( index );
    }


    char *compilerWarningAt( std::int32_t index ) {
        return compilerWarnings->at( index );
    }


    void error( const char *m, va_list ap ) {
        char *buffer = new_resource_array<char>( ::TEST_NOTIFIER_BUFLEN );
        vsnprintf( buffer, ::TEST_NOTIFIER_BUFLEN - 1, m, ap );
        errors->append( buffer );
    }


    void warning( const char *m, va_list ap ) {
        char *buffer = new_resource_array<char>( ::TEST_NOTIFIER_BUFLEN );
        vsnprintf( buffer, ::TEST_NOTIFIER_BUFLEN - 1, m, ap );
        warnings->append( buffer );
    }


    void compiler_warning( const char *m, va_list ap ) {
        char *buffer = new_resource_array<char>( ::TEST_NOTIFIER_BUFLEN );
        vsnprintf( buffer, ::TEST_NOTIFIER_BUFLEN - 1, m, ap );
        compilerWarnings->append( buffer );
    }
};
