//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/NativeMethod.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/system/platform.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


// A disassembler prints out intel 386 code annotated with delta specific information.
// %note:
//   The current implementation does not annotate
//   the i386 code with delta specific information.

class Disassembler : AllStatic {
public:
    static void decode( const NativeMethod *nm, ConsoleOutputStream *stream = _console );

    static void decode( const char *begin, const char *end, ConsoleOutputStream *stream = _console );
};
