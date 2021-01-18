
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

typedef class OopDescriptor *Oop;


// -----------------------------------------------------------------------------
// Macros to do tracing of primitives

#define ST_TRACE_PRIMITIVES

#ifdef ST_TRACE_PRIMITIVES

#define TRACE_FUNC( flag, label ) \
      static void Trace(const char* name, \
                        const char* s1 = nullptr, Oop a1 = nullptr, \
                        const char* s2 = nullptr, Oop a2 = nullptr, \
                        const char* s3 = nullptr, Oop a3 = nullptr, \
                        const char* s4 = nullptr, Oop a4 = nullptr, \
                        const char* s5 = nullptr, Oop a5 = nullptr, \
                        const char* s6 = nullptr, Oop a6 = nullptr, \
                        const char* s7 = nullptr, Oop a7 = nullptr, \
                        const char* s8 = nullptr, Oop a8 = nullptr, \
                        const char* s9 = nullptr, Oop a9 = nullptr, \
                        const char* s10 = nullptr, Oop a10 = nullptr) { \
      if (not flag) return; \
      lprintf("{%s::%s", label, name); \
      if (s1) { lprintf(" %s=", s1); a1->print_value(); } \
      if (s2) { lprintf(" %s=", s2); a2->print_value(); } \
      if (s3) { lprintf(" %s=", s3); a3->print_value(); } \
      if (s4) { lprintf(" %s=", s4); a4->print_value(); } \
      if (s5) { lprintf(" %s=", s5); a5->print_value(); } \
      if (s6) { lprintf(" %s=", s6); a6->print_value(); } \
      if (s7) { lprintf(" %s=", s7); a7->print_value(); } \
      if (s8) { lprintf(" %s=", s8); a8->print_value(); } \
      if (s9) { lprintf(" %s=", s9); a9->print_value(); } \
      if (s10) { lprintf(" %s=", s10); a10->print_value(); } \
      lprintf("}\n"); \
    }

#define TRACE_0( name ) \
      inc_calls(); \
      Trace(name);

#define TRACE_1( name, a1 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1);

#define TRACE_2( name, a1, a2 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2);

#define TRACE_3( name, a1, a2, a3 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3);

#define TRACE_4( name, a1, a2, a3, a4 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4);

#define TRACE_5( name, a1, a2, a3, a4, a5 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4, XSTR(a5), a5);

#define TRACE_6( name, a1, a2, a3, a4, a5, a6 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4, XSTR(a5), a5, XSTR(a6), a6);

#define TRACE_7( name, a1, a2, a3, a4, a5, a6, a7 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4, XSTR(a5), a5, XSTR(a6), a6, XSTR(a7), a7);

#define TRACE_8( name, a1, a2, a3, a4, a5, a6, a7, a8 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4, XSTR(a5), a5, XSTR(a6), a6, XSTR(a7), a7, XSTR(a8), a8);

#define TRACE_9( name, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4, XSTR(a5), a5, XSTR(a6), a6, XSTR(a7), a7, XSTR(a8), a8, XSTR(a9), a9);

#define TRACE_10( name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
      inc_calls(); \
      Trace(name, XSTR(a1), a1, XSTR(a2), a2, XSTR(a3), a3, XSTR(a4), a4, XSTR(a5), a5, XSTR(a6), a6, XSTR(a7), a7, XSTR(a8), a8, XSTR(a9), a9, XSTR(a10), a10);

#else

#define TRACE_FUNC( flag, label )

#define TRACE_0( name )
#define TRACE_1( name, a1 )
#define TRACE_2( name, a1, a2 )
#define TRACE_3( name, a1, a2, a3 )
#define TRACE_4( name, a1, a2, a3, a4 )
#define TRACE_5( name, a1, a2, a3, a4, a5 )
#define TRACE_6( name, a1, a2, a3, a4, a5, a6 )
#define TRACE_7( name, a1, a2, a3, a4, a5, a6, a7 )
#define TRACE_8( name, a1, a2, a3, a4, a5, a6, a7, a8 )
#define TRACE_9( name, a1, a2, a3, a4, a5, a6, a7, a8, a9 )
#define TRACE_10( name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )

#endif

#define PROLOGUE_0( name ) TRACE_0(name)
#define PROLOGUE_1( name, a1 ) TRACE_1(name, a1)
#define PROLOGUE_2( name, a1, a2 ) TRACE_2(name, a1, a2)
#define PROLOGUE_3( name, a1, a2, a3 ) TRACE_3(name, a1, a2, a3)
#define PROLOGUE_4( name, a1, a2, a3, a4 ) TRACE_4(name, a1, a2, a3, a4)
#define PROLOGUE_5( name, a1, a2, a3, a4, a5 ) TRACE_5(name, a1, a2, a3, a4, a5)
#define PROLOGUE_6( name, a1, a2, a3, a4, a5, a6 ) TRACE_6(name, a1, a2, a3, a4, a5, a6)
#define PROLOGUE_7( name, a1, a2, a3, a4, a5, a6, a7 ) TRACE_7(name, a1, a2, a3, a4, a5, a6, a7)
#define PROLOGUE_8( name, a1, a2, a3, a4, a5, a6, a7, a8 ) TRACE_8(name, a1, a2, a3, a4, a5, a6, a7, a8)
#define PROLOGUE_9( name, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) TRACE_9(name, a1, a2, a3, a4, a5, a6, a7, a8, a9)
#define PROLOGUE_10( name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) TRACE_10(name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
