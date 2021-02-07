
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"

#include <cstdarg>


class Notifier {
public:
    Notifier() = default;
    virtual ~Notifier() = default;

    static Notifier *current;

    virtual void error( const char *m, va_list argptr ) = 0;

    virtual void warning( const char *m, va_list argptr ) = 0;

    virtual void compiler_warning( const char *m, va_list argptr ) = 0;
};


class DebugNotifier : public Notifier, public CHeapAllocatedObject {
public:
    DebugNotifier() = default;
    virtual ~DebugNotifier() = default;

    void error( const char *m, va_list ap );

    void warning( const char *m, va_list ap );

    void compiler_warning( const char *m, va_list ap );
};
