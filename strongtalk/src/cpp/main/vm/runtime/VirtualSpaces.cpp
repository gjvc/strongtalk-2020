
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/VirtualSpace.hpp"


VirtualSpace *VirtualSpaces::head = nullptr;


void VirtualSpaces::add( VirtualSpace *sp ) {
    sp->next = head;
    head = sp;
}


void VirtualSpaces::remove( VirtualSpace *sp ) {
    print();

    if ( not head ) {
        return;
    }

    if ( head == sp ) {
        head = sp->next;
    } else {
        for ( VirtualSpace *p = head; p->next; p = p->next ) {
            if ( p->next == sp ) {
                p->next = sp->next;
            }
        }
    }
}


std::int32_t VirtualSpaces::committed_size() {
    std::int32_t total{ 0 };

    for ( VirtualSpace *p{ head }; p; p = p->next )
        total += p->committed_size();

    return total;
}


std::int32_t VirtualSpaces::reserved_size() {
    std::int32_t total{ 0 };

    for ( VirtualSpace *p{ head }; p; p = p->next )
        total += p->reserved_size();

    return total;
}


std::int32_t VirtualSpaces::uncommitted_size() {
    std::int32_t total{ 0 };

    for ( VirtualSpace *p{ head }; p; p = p->next )
        total += p->uncommitted_size();

    return total;
}


void VirtualSpaces::print() {
    SPDLOG_INFO( "--- begin VirtualSpaces ---" );
    for ( VirtualSpace *p{ head }; p; p = p->next ) {
        p->print();
    }
    SPDLOG_INFO( "--- end VirtualSpaces ---" );
}


void VirtualSpaces::test() {
    VirtualSpace space( 128 * 1024, 4 * 1024 );
    space.print();
    space.expand( 4 * 1024 );
    space.print();
    space.expand( 4 * 1024 );
    space.print();
    space.shrink( 4 * 1024 );
    space.print();
    space.shrink( 4 * 1024 );
    space.print();
}
