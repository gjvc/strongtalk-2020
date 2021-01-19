//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/utilities/DebugNotifier.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"

#include <cstdarg>

constexpr int BUFLEN = 2048;


class TestNotifier : public Notifier, public ResourceObject {

private:
    GrowableArray<char *> *errors;
    GrowableArray<char *> *warnings;
    GrowableArray<char *> *compilerWarnings;

public:
    TestNotifier() {
        errors           = new GrowableArray<char *>;
        warnings         = new GrowableArray<char *>;
        compilerWarnings = new GrowableArray<char *>;
    }


    int errorCount() {
        return errors->length();
    }


    int warningCount() {
        return warnings->length();
    }


    int compilerWarningCount() {
        return compilerWarnings->length();
    }


    char *errorAt( int index ) {
        return errors->at( index );
    }


    char *warningAt( int index ) {
        return warnings->at( index );
    }


    char *compilerWarningAt( int index ) {
        return compilerWarnings->at( index );
    }


    void error( const char *m, va_list ap ) {
        char *buffer = new_resource_array<char>( ::BUFLEN );
        vsnprintf( buffer, ::BUFLEN - 1, m, ap );
        errors->append( buffer );
    }


    void warning( const char *m, va_list ap ) {
        char *buffer = new_resource_array<char>( ::BUFLEN );
        vsnprintf( buffer, ::BUFLEN - 1, m, ap );
        warnings->append( buffer );
    }


    void compiler_warning( const char *m, va_list ap ) {
        char *buffer = new_resource_array<char>( ::BUFLEN );
        vsnprintf( buffer, ::BUFLEN - 1, m, ap );
        compilerWarnings->append( buffer );
    }
};
