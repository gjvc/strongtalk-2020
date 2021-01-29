
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <mutex>


// -----------------------------------------------------------------------------

// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html

#include "vm/system/gnu-mingw32.hpp"
#include "vm/system/llvm.hpp"
#include "vm/system/msvc.hpp"


// -----------------------------------------------------------------------------

#include "vm/system/sizes.hpp"
#include "vm/system/bits.hpp"


// -----------------------------------------------------------------------------

#define SPDLOG_COMPILED_LIB

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/bundled/printf.h>


// -----------------------------------------------------------------------------

#include "vm/utilities/lprintf.hpp"


// -----------------------------------------------------------------------------

namespace strongtalk::vm {

    class Log {
    public:
        Log();
        static Log *getInstance();

        template<typename... Args>
        void info( const char *message, const Args &... args ) {
            logger_->info( fmt::sprintf( message, args... ) );
        }

    private:
        static Log *_instance;
        static std::once_flag                  initFlag_;   //
        static std::shared_ptr<spdlog::logger> logger_;     //
    };


    template<typename... Args>
    void Info( const char *message, const Args &... args ) {
        Log::getInstance()->info( message, args... );
    }

};


#define LOG_INFO( MESSAGE, ... ) strongtalk::vm::Log::Info(MESSAGE, ##__VA_ARGS__)


// -----------------------------------------------------------------------------

#define LOGI( ... )  \
 do{char buf[256]; snprintf(buf, 256,__VA_ARGS__);  spdlog::info(buf);}while(0)
//use:
//LOGI("hello %d, %s", 0, "msg");
