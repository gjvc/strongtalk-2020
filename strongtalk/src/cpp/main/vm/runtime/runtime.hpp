//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



/*
Implementation of a function accessible from compiled code should follow the below structure (see verifyNonLocalReturn for an example)

    // Microsoft Visual C++, Compiler option: Generate frame pointer
    #pragma optimize("y", on)

    extern "C" volatile void function_called_from_vm(...) {
    VM_StackMarker mark;
    .. code ...
    }

    // Microsoft Visual C++, Compiler option:  Default settings
    #pragma optimize("",  on)

*/


/*
Pascal Calling Convention
A routine that uses the Pascal calling convention must preserve EBX, EBP, ESI, EDI.
(extracted from Microsoft Development Library, MASM 6.1 Programmer's Guide)

C Calling Convention
    A routine that uses the C calling convention must preserve EBP, ESI, EDI.
*/


extern "C" const char *byte_map_base;
extern "C" const char *MaxSP;


inline void Set_Byte_Map_Base( const char *base ) {
    byte_map_base = base;
}


inline void setSPMax( const char *m ) {
    MaxSP = m;
}


inline const char *currentSPMax() {
    return MaxSP;
}
