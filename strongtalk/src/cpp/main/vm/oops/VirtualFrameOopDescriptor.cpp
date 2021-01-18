//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/VirtualFrameOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/runtime/VirtualFrame.hpp"


VirtualFrame *VirtualFrameOopDescriptor::get_vframe() {

    DeltaProcess *proc = process()->process();

    // Check process
    if ( proc == nullptr )
        return nullptr;

    // Check time stamp
    if ( proc->time_stamp() not_eq time_stamp() )
        return nullptr;

    VirtualFrame *vf = proc->last_delta_vframe();

    for ( std::size_t i = 1; i < index() and vf; i++ ) {
        vf = vf->sender();
    }

    return vf;
}
