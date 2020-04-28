//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



// init_globals replaces C++ global objects so we can use the standard linker
// to link Delta (which is at least twice as fast as using the GNU C++ linker).
// Also, init.c gives explicit control over the sequence of initialization.

// Programming convention: instead of using a global object (e,g, "Foo foo;"),
// use "Foo* foo;", create a function init_foo() in foo.c, and add a call
// to init_foo in init.cpp.

void init_globals();            // call constructors at startup


//
void console_init();

void os_init();

void except_init();

void primitives_init();

void eventlog_init();

void bytecodes_init();

void universe_init();

void generatedPrimitives_init_before_interpreter();

void interpreter_init();

void dispatchTable_init();

void costModel_init();

void sweeper_init();

void fprofiler_init();

void systemAverage_init();

void preemption_init();

void generatedPrimitives_init_after_interpreter();

//
void compiler_init();

void mapping_init();

void opcode_init();

//
void lprintf_exit();

void os_exit();
