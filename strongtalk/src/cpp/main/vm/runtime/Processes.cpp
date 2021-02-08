//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Process.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"


// ======= Processes ========

DeltaProcess *Processes::_processList = nullptr;


void Processes::start( VMProcess *p ) {
    // processList = nullptr;
    // activate the vm process
    p->activate_system();
}


void Processes::add( DeltaProcess *p ) {
    p->set_next( _processList );
    _processList = p;
}


#define ALL_PROCESSES( X ) for (DeltaProcess* X = _processList; X; X = X->next())


DeltaProcess *Processes::find_from_thread_id( std::int32_t id ) {
    for ( DeltaProcess *p = _processList; p; p = p->next() )
        if ( p->thread_id() == id )
            return p;
    return nullptr;
}


void Processes::frame_iterate( FrameClosure *blk ) {
    ALL_PROCESSES( p )p->frame_iterate( blk );
}


void Processes::oop_iterate( OopClosure *blk ) {
    ALL_PROCESSES( p )p->oop_iterate( blk );
}


void Processes::process_iterate( ProcessClosure *blk ) {
    ALL_PROCESSES( p )blk->do_process( p );
}


void Processes::verify() {
    ALL_PROCESSES( p )p->verify();
}


bool Processes::has_completed_async_call() {
    ALL_PROCESSES( p ) {
        if ( p->state() == ProcessState::yielded_after_async_dll )
            return true;
    }
    return false;
}


void Processes::print() {
    SPDLOG_INFO( "All processes:" );
    ALL_PROCESSES( p ) {
        ResourceMark resourceMark;
        p->print();
        p->trace_stack();
    }
}


void Processes::remove( DeltaProcess *p ) {
    st_assert( includes( p ), "p must be present" );
    DeltaProcess *current = _processList;
    DeltaProcess *prev    = nullptr;

    while ( current not_eq p ) {
        prev    = current;
        current = current->next();
    }

    if ( prev ) {
        prev->set_next( current->next() );
    } else {
        _processList = p->next();
    }
}


bool Processes::includes( DeltaProcess *p ) {
    ALL_PROCESSES( q )if ( q == p )
            return true;
    return false;
}


DeltaProcess *Processes::last() {
    DeltaProcess *last = nullptr;
    ALL_PROCESSES( q )last = q;

    return last;
}


void Processes::kill_all() {
    DeltaProcess *current = _processList;
    while ( current ) {
        DeltaProcess *next = current->next();
        VMProcess::terminate( current );
        current->set_next( nullptr );
        delete current;
        current = next;
    }

    _processList = nullptr;
}


class ScavengeOopClosure : public OopClosure {
    void do_oop( Oop *o ) {
        SCAVENGE_TEMPLATE( o );
    }
};


void Processes::scavenge_contents() {
    ScavengeOopClosure blk;
    oop_iterate( &blk );
}


void Processes::follow_roots() {
    ALL_PROCESSES( p )p->follow_roots();
}


class ConvertHCodePointersClosure : public FrameClosure {
    void do_frame( Frame *f ) {
        if ( f->is_interpreted_frame() ) {
            f->convert_heap_code_pointer();
        }
    }
};


void Processes::convert_heap_code_pointers() {
    ConvertHCodePointersClosure blk;
    frame_iterate( &blk );
}


class RestoreHCodePointersClosure : public FrameClosure {
    void do_frame( Frame *f ) {
        if ( f->is_interpreted_frame() ) {
            f->restore_heap_code_pointer();
        }
    }
};


void Processes::restore_heap_code_pointers() {
    RestoreHCodePointersClosure blk;
    frame_iterate( &blk );
}


void Processes::deoptimized_wrt_marked_nativeMethods() {
    StackChunkBuilder::begin_deoptimization();
    ALL_PROCESSES( p )p->deoptimized_wrt_marked_native_methods();
    StackChunkBuilder::end_deoptimization();
}


void Processes::deoptimize_wrt( NativeMethod *nm ) {
    GrowableArray<NativeMethod *> *nms = nm->invalidation_family();
    // mark family for deoptimization

    for ( std::int32_t i = 0; i < nms->length(); i++ )
        nms->at( i )->mark_for_deoptimization();

    // deoptimize
    deoptimized_wrt_marked_nativeMethods();

    // unmark for deoptimization
    for ( std::int32_t i = 0; i < nms->length(); i++ )
        nms->at( i )->unmark_for_deoptimization();
}


void Processes::deoptimize_wrt( GrowableArray<NativeMethod *> *list ) {
    // mark for deoptimization
    for ( std::int32_t i = 0; i < list->length(); i++ ) {
        NativeMethod                  *nm  = list->at( i );
        GrowableArray<NativeMethod *> *nms = nm->invalidation_family();

        for ( std::int32_t j = 0; j < nms->length(); j++ )
            nms->at( j )->mark_for_deoptimization();
    }

    // deoptimize
    deoptimized_wrt_marked_nativeMethods();

    // unmark for deoptimization
    for ( std::int32_t i = 0; i < list->length(); i++ ) {
        NativeMethod                  *nativeMethod = list->at( i );
        GrowableArray<NativeMethod *> *nms          = nativeMethod->invalidation_family();

        for ( std::int32_t j = 0; j < nms->length(); j++ )
            nms->at( j )->unmark_for_deoptimization();
    }
}


void Processes::update_nlr_targets( CompiledVirtualFrame *f, ContextOop con ) {
    ALL_PROCESSES( p )p->update_nlr_targets( f, con );
}


void Processes::deoptimize_all() {
    Universe::code->mark_all_for_deoptimization();
    deoptimized_wrt_marked_nativeMethods();
    Universe::code->unmark_all_for_deoptimization();
}
