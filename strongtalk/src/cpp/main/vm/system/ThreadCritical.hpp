//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



// A critical region for controlling thread transfer at interrupts
class ThreadCritical {

private:
    static bool _initialized;

    friend void os_init();

    friend void os_exit();

    static void intialize();

    static void release();

public:
    static bool initialized() {
        return _initialized;
    }


    ThreadCritical();

    ~ThreadCritical();
};
