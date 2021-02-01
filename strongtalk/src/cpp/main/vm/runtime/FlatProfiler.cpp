//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/FlatProfiler.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/PeriodicTask.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


ProfiledNode **FlatProfiler::_table = nullptr;
std::int32_t  FlatProfiler::_tableSize = 1024;

DeltaProcess      *FlatProfiler::_deltaProcess     = nullptr;
FlatProfilerTask  *FlatProfiler::_flatProfilerTask = nullptr;
Timer             FlatProfiler::_timer;

std::int32_t FlatProfiler::_gc_ticks        = 0;
std::int32_t FlatProfiler::_semaphore_ticks = 0;
std::int32_t FlatProfiler::_stub_ticks      = 0;
std::int32_t FlatProfiler::_unknown_ticks   = 0;
std::int32_t FlatProfiler::_compiler_ticks  = 0;

static constexpr std::int32_t col2 = 11;    // position of output column 2
static constexpr std::int32_t col3 = 30;    // position of output column 3
static constexpr std::int32_t col4 = 55;    // position of output column 4


std::int32_t FlatProfiler::entry( std::int32_t value ) {
    return value % _tableSize;
}


void FlatProfiler::interpreted_update( MethodOop method, KlassOop klass, TickPosition where ) {

    std::int32_t index = entry( method->selector_or_method()->identity_hash() );
    if ( not _table[ index ] ) {
        _table[ index ] = new InterpretedNode( method, klass, where );
    } else {
        for ( ProfiledNode *node = _table[ index ]; node; node = node->next() ) {
            if ( node->match( method, klass ) ) {
                node->update( where );
                return;
            }
            if ( not node->next() )
                node->set_next( new InterpretedNode( method, klass, where ) );
        }
    }
}


void FlatProfiler::compiled_update( NativeMethod *nm, TickPosition where ) {
    std::int32_t index = entry( nm->_mainId.major() );
    if ( not _table[ index ] ) {
        _table[ index ] = new CompiledNode( nm, where );
    } else {
        for ( ProfiledNode *node = _table[ index ]; node; node = node->next() ) {
            if ( node->match( nm ) ) {
                node->update( where );
                return;
            }
            if ( not node->next() )
                node->set_next( new CompiledNode( nm, where ) );
        }
    }
}


class FlatProfilerTask : public PeriodicTask {
public:
    FlatProfilerTask( std::int32_t interval_time ) :
        PeriodicTask( interval_time ) {
    }


    virtual ~FlatProfilerTask() {

    }


    void task();
};


void FlatProfilerTask::task() {

    // ignore of we're not in the right process
    if ( FlatProfiler::_deltaProcess == nullptr )
        return; // profiler not active

    if ( DeltaProcess::active() == FlatProfiler::_deltaProcess or UseGlobalFlatProfiling ) {
        FlatProfiler::record_tick();
    }

}


void FlatProfiler::record_tick_for_running_frame( Frame fr ) {

    // The tick happened in real code -> non VM code
    if ( fr.is_interpreted_frame() ) {
        MethodOop method = fr.method();
        if ( method == nullptr )
            return;
        st_assert( method->is_method(), "must be method" );
        FlatProfiler::interpreted_update( method, fr.receiver()->klass(), TickPosition::in_code );

    } else if ( fr.is_compiled_frame() ) {
        FlatProfiler::compiled_update( findNativeMethod( fr.pc() ), TickPosition::in_code );

    } else if ( PolymorphicInlineCache::in_heap( fr.pc() ) ) {
        PolymorphicInlineCache *pic = PolymorphicInlineCache::find( fr.pc() );
        FlatProfiler::compiled_update( findNativeMethod( (const char *) pic->compiled_ic() ), TickPosition::in_pic );

    } else if ( StubRoutines::contains( fr.pc() ) ) {
        FlatProfiler::_stub_ticks++;
    }
}


void FlatProfiler::record_tick_for_calling_frame( Frame fr ) {

    // The tick happened in VM code
    TickPosition where = TickPosition::other;
    if ( theCompiler ) {
        where = TickPosition::in_compiler;
    }
    if ( fr.is_interpreted_frame() ) {
        MethodOop method = fr.method();
        if ( method == nullptr )
            return;
        st_assert( method->is_method(), "must be method" );
        std::int32_t byteCodeIndex = method->byteCodeIndex_from( fr.hp() );
        if ( ByteCodes::code_type( (ByteCodes::Code) *method->codes( byteCodeIndex ) ) == ByteCodes::CodeType::primitive_call ) {
            where = TickPosition::in_primitive;
        }
        FlatProfiler::interpreted_update( method, fr.receiver()->klass(), where );

    } else if ( fr.is_compiled_frame() ) {
        NativeMethod                  *nm = findNativeMethod( fr.pc() );
        RelocationInformationIterator iter( nm );
        while ( iter.next() ) {
            if ( iter.is_call() and iter.call_end() == fr.pc() ) {
                if ( iter.type() == RelocationInformation::RelocationType::primitive_type )
                    where = TickPosition::in_primitive;
            }
        }
        FlatProfiler::compiled_update( nm, where );

    } else {
        if ( StubRoutines::contains( fr.pc() ) ) {
            FlatProfiler::_stub_ticks++;
        } else {
            FlatProfiler::_unknown_ticks++;
        }
    }
}


void FlatProfiler::record_tick() {

    // If we're idle forget about the tick.
    if ( DeltaProcess::is_idle() )
        return;

    // check for special vm flags
    if ( theCompiler ) {
        FlatProfiler::_compiler_ticks++;
    }

    if ( garbageCollectionInProgress ) {
        FlatProfiler::_gc_ticks++;
        return;
    }

    if ( processSemaphore ) {
        FlatProfiler::_semaphore_ticks++;
        return;
    }

    {
        FlagSetting( processSemaphore, true );
        DeltaProcess *p = DeltaProcess::active();
        if ( p->last_Delta_fp() ) {
            record_tick_for_calling_frame( p->last_frame() );
        } else {
            record_tick_for_running_frame( p->profile_top_frame() );
        }
    }
}


void FlatProfiler::allocate_table() {
    _table = new_c_heap_array<ProfiledNode *>( _tableSize );
    for ( std::int32_t i = 0; i < _tableSize; i++ )
        _table[ i ] = nullptr;
}


void FlatProfiler::reset() {
    _deltaProcess     = nullptr;
    _flatProfilerTask = nullptr;

    for ( std::int32_t i = 0; i < _tableSize; i++ ) {
        ProfiledNode *n = _table[ i ];
        if ( n ) {
            delete n;
            _table[ i ] = nullptr;
        }
    }

    _gc_ticks        = 0;
    _unknown_ticks   = 0;
    _semaphore_ticks = 0;
    _compiler_ticks  = 0;
    _stub_ticks      = 0;
}


void FlatProfiler::engage( DeltaProcess *p ) {
    _deltaProcess = p;
    if ( _flatProfilerTask == nullptr ) {
        _flatProfilerTask = new FlatProfilerTask( 10 );
        _flatProfilerTask->enroll();
        _timer.start();
    }
}


DeltaProcess *FlatProfiler::disengage() {
    if ( not _flatProfilerTask )
        return nullptr;
    _flatProfilerTask->deroll();
    delete _flatProfilerTask;
    _flatProfilerTask = nullptr;
    _timer.stop();
//    DeltaProcess *p = process();
    _deltaProcess = nullptr;
    return _deltaProcess;
}


bool FlatProfiler::is_active() {
    return _flatProfilerTask not_eq nullptr;
}


static std::int32_t compare_nodes( const void *p1, const void *p2 ) {
    ProfiledNode **pn1 = (ProfiledNode **) p1;
    ProfiledNode **pn2 = (ProfiledNode **) p2;
    return ( *pn2 )->total_ticks() - ( *pn1 )->total_ticks();
}


void print_ticks( const char *title, std::int32_t ticks, std::int32_t total ) {
    if ( ticks > 0 )
        spdlog::info( "total [%5.1f%%]  ticks [%3d]  title[{}]", ticks * 100.0 / total, ticks, title );
}


void FlatProfiler::print( std::int32_t cutoff ) {
    static_cast<void>(cutoff); // unused

    FlagSetting  f( PrintObjectID, false );
    ResourceMark resourceMark;
    double       secs = _timer.seconds();

    GrowableArray<ProfiledNode *> *array = new GrowableArray<ProfiledNode *>( 200 );

    for ( std::int32_t i = 0; i < _tableSize; i++ ) {
        for ( ProfiledNode *node = _table[ i ]; node; node = node->next() )
            array->append( node );
    }

    array->sort( &ProfiledNode::compare );

    // compute total
    std::int32_t total = total_ticks();

    for ( std::int32_t index = 0; index < array->length(); index++ ) {
        total += array->at( index )->ticks.total();
    }

    spdlog::info( "FlatProfiler {:3.2f} secs, ({} ticks)", secs, total );

    // print interpreted methods
    TickCounter interpreted_ticks;

    bool         has_interpreted_ticks = false;
    std::int32_t print_count           = 0;
    std::int32_t index                 = 0;

    for ( ; index < array->length(); index++ ) {
        ProfiledNode *n = array->at( index );
        if ( n->is_interpreted() ) {
            interpreted_ticks.add( &n->ticks );
            if ( not has_interpreted_ticks ) {
                InterpretedNode::print_title( _console );
                has_interpreted_ticks = true;
            }
            if ( print_count++ < ProfilerNumberOfInterpreterMethods ) {
                n->print( _console, total );
            }
        }
    }

    if ( has_interpreted_ticks ) {
        TickCounter others;
        for ( ; index < array->length(); index++ )
            others.add( &array->at( index )->ticks );
        if ( others.total() > 0 ) {
            InterpretedNode::print_total( _console, &interpreted_ticks, total, "(all above)" );
            InterpretedNode::print_total( _console, &others, total, "(all others)" );
        }
        interpreted_ticks.add( &others );
        InterpretedNode::print_total( _console, &interpreted_ticks, total, "Total interpreted" );
        _console->cr();
    }

    // print compiled methods
    print_count = 0;
    bool        has_compiled_ticks = false;
    TickCounter compiled_ticks;

    for ( std::int32_t i = 0; i < array->length(); i++ ) {
        ProfiledNode *n = array->at( i );
        if ( n->is_compiled() ) {
            compiled_ticks.add( &n->ticks );
            if ( not has_compiled_ticks ) {
                CompiledNode::print_title( _console );
                has_compiled_ticks = true;
            }
            if ( print_count++ < ProfilerNumberOfCompiledMethods ) {
                n->print( _console, total );
            }
        }
    }

    if ( has_compiled_ticks ) {
        TickCounter others;
        for ( ; index < array->length(); index++ )
            others.add( &array->at( index )->ticks );

        if ( others.total() > 0 ) {
            CompiledNode::print_total( _console, &compiled_ticks, total, "(all above)" );
            CompiledNode::print_total( _console, &others, total, "(all others)" );
        }
        compiled_ticks.add( &others );
        CompiledNode::print_total( _console, &compiled_ticks, total, "Total compiled" );
        _console->cr();
    }

    _console->cr();

    if ( total_ticks() > 0 ) {
        spdlog::info( " Additional ticks:" );
        print_ticks( "Garbage collector", _gc_ticks, total );
        print_ticks( "Process semaphore", _semaphore_ticks, total );
        print_ticks( "Unknown code", _unknown_ticks, total );
        print_ticks( "Stub routines", _stub_ticks, total );
        print_ticks( "Total compilation (already included above)", _compiler_ticks, total );
    }

}


void fprofiler_init() {
    spdlog::info( "%system-init:  fprofiler_init" );

    FlatProfiler::allocate_table();
}


void ProfiledNode::print( ConsoleOutputStream *stream, std::int32_t total_ticks ) const {
    static_cast<void>(stream); // unused

    MethodOop m = method();
    if ( m->is_blockMethod() ) {
        spdlog::info( "{:<24}  {:<24}  {:<48}  {:<24}", total_ticks, receiver_klass()->klass_part()->name(), m->selector()->as_string(), m->enclosing_method_selector()->print_value_string() );
    } else {
        spdlog::info( "{:<24}  {:<24}  {:<48}  {:<24}", total_ticks, receiver_klass()->klass_part()->name(), m->selector()->as_string(), m->selector()->print_value_string() );
    }
//            m->selector()->print_symbol_on( stream );
//
//        if ( ProfilerShowMethodHolder ) {
//            KlassOop method_holder = receiver_klass()->klass_part()->lookup_method_holder_for( m );
//            if ( method_holder and ( method_holder not_eq receiver_klass() ) ) {
//                spdlog::info( "{:<24}  {:<24}  {:<24}", total_ticks, receiver_klass()->klass_part()->name(), method_holder->klass_part()->name() );
//                method_holder->klass_part()->print_name_on( _console );
//            }
//        }


//        spdlog::info( "{:<24}  {:<24}  {:<24}", total_ticks, receiver_klass()->klass_part()->name(), m->enclosing_method_selector()->print_value_string() );
//        ticks.print_code( stream, total_ticks );
//        stream->fill_to( col2 );
//        print_receiver_klass_on( stream );
//        stream->fill_to( col3 );
//
//        print_method_on( stream );
//        stream->fill_to( col4 );
//
//        ticks.print_other( stream );
//        stream->cr();
}


std::int32_t ProfiledNode::compare( ProfiledNode **a, ProfiledNode **b ) {
    return ( *b )->total_ticks() - ( *a )->total_ticks();
}


void ProfiledNode::print_method_on( ConsoleOutputStream *stream ) const {
    static_cast<void>(stream); // unused

//        MethodOop m = method();
//        if ( m->is_blockMethod() ) {
//            stream->print( "[] " );
//            m->enclosing_method_selector()->print_symbol_on( stream );
//        } else {
//            m->selector()->print_symbol_on( stream );
//        }
//
//        if ( ProfilerShowMethodHolder ) {
//            KlassOop method_holder = receiver_klass()->klass_part()->lookup_method_holder_for( m );
//            if ( method_holder and ( method_holder not_eq receiver_klass() ) ) {
//                _console->print( ", in " );
//                method_holder->klass_part()->print_name_on( _console );
//            }
//        }
}


void ProfiledNode::print_total( ConsoleOutputStream *stream, TickCounter *t, std::int32_t total, const char *msg ) {
    static_cast<void>(stream); // unused
    static_cast<void>(t); // unused
    static_cast<void>(total); // unused
    static_cast<void>(msg); // unused
//    t->print_code( stream, total );
//    stream->print( msg );
//    stream->fill_to( col4 );
//    t->print_other( stream );
    spdlog::info( "{:<24}  {:<24}  {:<48}  {:<24}", "TOTALS", "", "", "" );
}


void ProfiledNode::print_title( ConsoleOutputStream *stream ) {
    static_cast<void>(stream); // unused
//        spdlog::info( "{:<24}  {:<24}  {:<48}  {:<24}", "Receiver", "Method", "Leaf ticks", "extra1" );
}


void ProfiledNode::print_receiver_klass_on( ConsoleOutputStream *stream ) const {
    receiver_klass()->klass_part()->print_name_on( stream );
}


ProfiledNode::ProfiledNode() {
    _next = nullptr;
}


ProfiledNode::~ProfiledNode() {
    if ( _next ) {
        delete _next;
    }
}


void ProfiledNode::set_next( ProfiledNode *n ) {
    _next = n;
}


ProfiledNode *ProfiledNode::next() const {
    return _next;
}


void ProfiledNode::update( TickPosition where ) {
    ticks.update( where );
}


TickCounter::TickCounter() {
    ticks_in_code       = 0;
    ticks_in_primitives = 0;
    ticks_in_compiler   = 0;
    ticks_in_pics       = 0;
    ticks_in_other      = 0;
}


void TickCounter::update( TickPosition where ) {
    switch ( where ) {
        case TickPosition::in_code:
            ticks_in_code++;
            break;
        case TickPosition::in_primitive:
            ticks_in_primitives++;
            break;
        case TickPosition::in_compiler:
            ticks_in_compiler++;
            break;
        case TickPosition::in_pic:
            ticks_in_pics++;
            break;
        case TickPosition::other:
            ticks_in_other++;
            break;
    }
}


void TickCounter::print_other( ConsoleOutputStream *stream ) const {
    if ( ticks_in_primitives > 0 )
        stream->print( "prim=%d ", ticks_in_primitives );
    if ( ticks_in_compiler > 0 )
        stream->print( "comp=%d ", ticks_in_compiler );
    if ( ticks_in_pics > 0 )
        stream->print( "pics=%d ", ticks_in_pics );
    if ( ticks_in_other > 0 )
        stream->print( "other=%d ", ticks_in_other );
}


void TickCounter::print_code( ConsoleOutputStream *stream, std::int32_t total_ticks ) const {
    stream->print( "%5.1f%% %3d ", total() * 100.0 / total_ticks, ticks_in_code );
}


void TickCounter::add( TickCounter *a ) {
    ticks_in_code += a->ticks_in_code;
    ticks_in_primitives += a->ticks_in_primitives;
    ticks_in_compiler += a->ticks_in_compiler;
    ticks_in_pics += a->ticks_in_pics;
    ticks_in_other += a->ticks_in_other;
}


std::int32_t TickCounter::total() const {
    return ticks_in_code + ticks_in_primitives + ticks_in_compiler + ticks_in_pics + ticks_in_other;
}
