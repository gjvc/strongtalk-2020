
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "VirtualFrame.hpp"


void trace( VirtualFrame *from_frame, std::int32_t start_frame, std::int32_t number_of_frames );
void traceCompiledFrame( Frame &f );
void traceInterpretedFrame( Frame &f );
void traceDeltaFrame( Frame &f );
void handle_exception( void *fp, void *sp, void *pc );
void except_init();
