//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#if defined( __MINGW32__ )

#include "vm/system/win32.hpp"

#include <windows.h>

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );


LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {

    switch ( msg ) {

        case WM_DESTROY:

            PostQuitMessage( 0 );
            break;
    }

    return DefWindowProcW( hwnd, msg, wParam, lParam );
}


extern "C" int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow ) {

    MSG       msg;
    HWND      hwnd;
    WNDCLASSW wc;

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.lpszClassName = L"Window";
    wc.hInstance     = hInstance;
    wc.hbrBackground = GetSysColorBrush( COLOR_3DFACE );
    wc.lpszMenuName  = NULL;
    wc.lpfnWndProc   = WndProc;
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );

//    RegisterClassW(&wc);

    os::set_args( __argc, __argv );
    return vm_main( __argc, __argv );
}

#endif
