//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/os.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitives/smi_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/primitives/debug_primitives.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/runtime/ResourceMark.hpp"


TRACE_FUNC( TraceDebugPrims, "debug" )


int debugPrimitives::number_of_calls;


PRIM_DECL_1( debugPrimitives::boolAt, Oop name ) {
    PROLOGUE_1( "boolAt", name )
    if ( not name->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    bool_t result;
    if ( debugFlags::boolAt( ByteArrayOop( name )->chars(), ByteArrayOop( name )->length(), &result ) )
        return result ? trueObj : falseObj;
    return markSymbol( vmSymbols::not_found() );
}


PRIM_DECL_2( debugPrimitives::boolAtPut, Oop name, Oop value ) {
    PROLOGUE_2( "boolAtPut", name, value )
    if ( not name->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    bool_t b;
    if ( value == trueObj )
        b = true;
    else if ( value == falseObj )
        b = false;
    else
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( debugFlags::boolAtPut( ByteArrayOop( name )->chars(), ByteArrayOop( name )->length(), &b ) )
        return b ? trueObj : falseObj;
    return markSymbol( vmSymbols::not_found() );
}


PRIM_DECL_1( debugPrimitives::smiAt, Oop name ) {
    PROLOGUE_1( "smiAt", name )
    if ( not name->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    int result;
    if ( debugFlags::intAt( ByteArrayOop( name )->chars(), ByteArrayOop( name )->length(), &result ) )
        return smiOopFromValue( result );
    return markSymbol( vmSymbols::not_found() );
}


PRIM_DECL_2( debugPrimitives::smiAtPut, Oop name, Oop value ) {
    PROLOGUE_2( "smiAtPut", name, value )
    if ( not name->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    int v = SMIOop( value )->value();
    if ( debugFlags::intAtPut( ByteArrayOop( name )->chars(), ByteArrayOop( name )->length(), &v ) )
        return smiOopFromValue( v );
    return markSymbol( vmSymbols::not_found() );
}


PRIM_DECL_0( debugPrimitives::clearLookupCache ) {
    PROLOGUE_0( "clearLookupCache" )
    LookupCache::flush();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::clearLookupCacheStatistics ) {
    PROLOGUE_0( "clearLookupCacheStatistics" )
    LookupCache::clear_statistics();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::printLookupCacheStatistics ) {
    PROLOGUE_0( "printLookupCacheStatistics" )
    LookupCache::print_statistics();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::printMemoryLayout ) {
    PROLOGUE_0( "printMemoryLayout" )
    Universe::print_layout();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::decodeAllMethods ) {
    PROLOGUE_0( "decodeAllMethods" )
    Universe::decode_methods();
    return trueObj;
}


PRIM_DECL_2( debugPrimitives::printMethodCodes, Oop receiver, Oop sel ) {
    PROLOGUE_2( "printMethodCodes", receiver, sel )
    if ( not sel->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    SymbolOop s = oopFactory::new_symbol( ByteArrayOop( sel ) );
    MethodOop m = receiver->blueprint()->lookup( s );
    if ( not m )
        return markSymbol( vmSymbols::not_found() );
    {
        ResourceMark resourceMark;
        if ( WizardMode )
            m->print_codes();
        PrettyPrinter::print( MethodOop( m ), receiver->klass() );
    }
    return receiver;
}


PRIM_DECL_2( debugPrimitives::generateIR, Oop receiver, Oop sel ) {
    _console->print_cr( "primitiveGenerateIR called..." );
    ResourceMark resourceMark;    // needed to avoid memory leaks!
    PROLOGUE_2( "generateIR", receiver, sel )
    if ( not sel->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    SymbolOop s = oopFactory::new_symbol( ByteArrayOop( sel ) );
    MethodOop m = receiver->blueprint()->lookup( s );
    if ( not m )
        return markSymbol( vmSymbols::not_found() );
    LookupKey key( receiver->klass(), s );

    auto temp = VM_OptimizeMethod( &key, m );
    VMProcess::execute( &temp );
    return receiver;
}


PRIM_DECL_2( debugPrimitives::optimizeMethod, Oop receiver, Oop sel ) {
    PROLOGUE_2( "optimizeMethod", receiver, sel );

    if ( not sel->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    SymbolOop s = oopFactory::new_symbol( ByteArrayOop( sel ) );
    MethodOop m = receiver->blueprint()->lookup( s );
    if ( not m )
        return markSymbol( vmSymbols::not_found() );

    LookupKey         key( receiver->klass(), s );
    VM_OptimizeMethod op( &key, m );
    // The operation takes place in the vmProcess
    VMProcess::execute( &op );

    return receiver;
}


PRIM_DECL_2( debugPrimitives::decodeMethod, Oop receiver, Oop sel ) {
    PROLOGUE_2( "decodeMethod", receiver, sel );
    if ( not sel->is_symbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    LookupResult result = LookupCache::ic_normal_lookup( receiver->klass(), SymbolOop( sel ) );
    if ( result.is_empty() )
        return markSymbol( vmSymbols::not_found() );
    if ( result.is_method() ) {
        // methodOop found => print byte codes
        ResourceMark resourceMark;
        result.method()->print_codes();
    } else {
        // NativeMethod found => print assembly code
        result.get_nativeMethod()->printCode();
    }
    return receiver;
}


PRIM_DECL_0( debugPrimitives::timerStart ) {
    PROLOGUE_0( "timerStart" );
    os::timerStart();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::timerStop ) {
    PROLOGUE_0( "timerStop" );
    os::timerStop();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::timerPrintBuffer ) {
    PROLOGUE_0( "timerPrintBuffer" );
    os::timerPrintBuffer();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::interpreterInvocationCounterLimit ) {
    PROLOGUE_0( "interpreterInvocationCounterLimit" );
    int limit = Interpreter::get_invocation_counter_limit();
    if ( limit < smi_min )
        limit = smi_min;
    else if ( limit > smi_max )
        limit = smi_max;
    return smiOopFromValue( limit );
}


PRIM_DECL_1( debugPrimitives::setInterpreterInvocationCounterLimit, Oop limit ) {
    PROLOGUE_1( "setInterpreterInvocationCounterLimit", limit );
    if ( not limit->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    int value = SMIOop( limit )->value();
    if ( value < 0 or value > MethodOopDescriptor::_invocation_count_max )
        return markSymbol( vmSymbols::out_of_bounds() );
    Interpreter::set_invocation_counter_limit( value );
    Interpreter::set_loop_counter_limit( value );    // for now - probably should have its own primitive
    return limit;
}


class ClearInvocationCounterClosure : public ObjectClosure {
private:
    void do_object( MemOop obj ) {
        if ( obj->is_method() )
            MethodOop( obj )->set_invocation_count( 0 );
    }
};


PRIM_DECL_0( debugPrimitives::clearInvocationCounters ) {
    PROLOGUE_0( "clearInvocationCounters" );
    ClearInvocationCounterClosure blk;
    Universe::object_iterate( &blk );
    return trueObj;
}


// Collects all methods with invocation counter >= cutoff
class CollectMethodClosure : public ObjectClosure {

private:
    GrowableArray<MethodOop> *_col;
    int _cutoff;

public:
    CollectMethodClosure( GrowableArray<MethodOop> *col, int cutoff ) {
        this->_col    = col;
        this->_cutoff = cutoff;
    }


    void do_object( MemOop obj ) {
        if ( obj->is_method() )
            if ( MethodOop( obj )->invocation_count() >= _cutoff )
                _col->push( MethodOop( obj ) );
    }
};


static int compare_method_counters( MethodOop *a, MethodOop *b ) {
    return ( *b )->invocation_count() - ( *a )->invocation_count();
}


PRIM_DECL_1( debugPrimitives::printInvocationCounterHistogram, Oop size ) {
    PROLOGUE_1( "printInvocationCounterHistogram", size );

    if ( not size->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ResourceMark rm;
    GrowableArray<MethodOop> *col = new GrowableArray<MethodOop>( 1024 );

    // Collect the methods
    CollectMethodClosure blk( col, SMIOop( size )->value() );
    Universe::object_iterate( &blk );
    _console->print_cr( "Collected %d methods", col->length() );

    // Sort the methods based on the invocation counters.
    col->sort( &compare_method_counters );

    // Print out the result
    for ( std::size_t i = 0; i < col->length(); i++ ) {
        MethodOop m = col->at( i );
        _console->print( "[%d] ", m->invocation_count() );
        m->pretty_print();
    }
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::clearInlineCaches ) {
    PROLOGUE_0( "clearInlineCaches" );

    Universe::flush_inline_caches_in_methods();
    Universe::code->clear_inline_caches();
    Universe::code->print();

    return trueObj;
}


#define FOR_ALL_NMETHOD( var ) \
  for (NativeMethod *var = Universe::code->first_nm(); var; var = Universe::code->next_nm(var))


PRIM_DECL_0( debugPrimitives::clearNativeMethodCounters ) {
    PROLOGUE_0( "clearNativeMethodCounters" );
    FOR_ALL_NMETHOD( nm )nm->set_invocation_count( 0 );
    return trueObj;
}


static int compare_NativeMethod_counters( NativeMethod **a, NativeMethod **b ) {
    return ( *b )->invocation_count() - ( *a )->invocation_count();
}


PRIM_DECL_1( debugPrimitives::printNativeMethodCounterHistogram, Oop size ) {
    PROLOGUE_1( "printNativeMethodCounterHistogram", size );
    if ( not size->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ResourceMark                  rm;
    GrowableArray<NativeMethod *> *col = new GrowableArray<NativeMethod *>( 1024 );
    // Collect the nativeMethods
    FOR_ALL_NMETHOD( nm )col->push( nm );

    _console->print_cr( "Collected %d nativeMethods", col->length() );
    // Sort the methods based on the invocation counters.
    col->sort( &compare_NativeMethod_counters );

    // Print out the result
    int end = ( col->length() > SMIOop( size )->value() ) ? SMIOop( size )->value() : col->length();

    for ( std::size_t i = 0; i < end; i++ ) {
        NativeMethod *m = col->at( i );
        _console->print( "[%d] ", m->invocation_count() );
        m->scopes()->print_partition();
        m->method()->pretty_print();
    }

    return trueObj;
}


class SumMethodInvocationClosure : public ObjectClosure {
private:
    int sum;
public:
    SumMethodInvocationClosure() {
        sum = 0;
    }


    void do_object( MemOop obj ) {
        if ( obj->is_method() )
            sum += MethodOop( obj )->invocation_count();
    }


    int result() {
        return sum;
    }
};


PRIM_DECL_0( debugPrimitives::numberOfMethodInvocations ) {
    PROLOGUE_0( "numberOfMethodInvocations" );
    SumMethodInvocationClosure blk;
    Universe::object_iterate( &blk );
    return smiOopFromValue( blk.result() );
}


PRIM_DECL_0( debugPrimitives::numberOfNativeMethodInvocations ) {
    PROLOGUE_0( "numberOfNativeMethodInvocations" );
    int sum = 0;
    FOR_ALL_NMETHOD( nm )sum += nm->invocation_count();
    return smiOopFromValue( sum );
}


PRIM_DECL_0( debugPrimitives::numberOfPrimaryLookupCacheHits ) {
    PROLOGUE_0( "numberOfPrimaryLookupCacheHits" );
    return smiOopFromValue( LookupCache::number_of_primary_hits );
}


PRIM_DECL_0( debugPrimitives::numberOfSecondaryLookupCacheHits ) {
    PROLOGUE_0( "numberOfSecondaryLookupCacheHits" );
    return smiOopFromValue( LookupCache::number_of_secondary_hits );
}


PRIM_DECL_0( debugPrimitives::numberOfLookupCacheMisses ) {
    PROLOGUE_0( "numberOfLookupCacheMisses" );
    return smiOopFromValue( LookupCache::number_of_misses );
}


PRIM_DECL_0( debugPrimitives::clearPrimitiveCounters ) {
    PROLOGUE_0( "clearPrimitiveCounters" );
    Primitives::clear_counters();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::printPrimitiveCounters ) {
    PROLOGUE_0( "printPrimitiveCounters" );
    Primitives::print_counters();
    return trueObj;
}


class Counter : public ResourceObject {
public:
    const char *title;
    int total_size;
    int number;


    Counter( const char *t ) {
        title      = t;
        total_size = 0;
        number     = 0;
    }


    void update( MemOop obj ) {
        total_size += obj->size();
        number++;
    }


    void print( const char *prefix ) {
        _console->print( "%s%s", prefix, title );
        _console->fill_to( 22 );
        _console->print_cr( "%6d %8d", number, total_size * oopSize );
    }


    void add( Counter *i ) {
        total_size += i->total_size;
        number += i->number;
    }


    static int compare( Counter **a, Counter **b ) {
        return ( *b )->total_size - ( *a )->total_size;
    }
};

class ObjectHistogram : public ObjectClosure {
private:
    Counter *doubles;
    Counter *blocks;
    Counter *objArrays;
    Counter *symbols;
    Counter *byteArrays;
    Counter *doubleByteArrays;
    Counter *klasses;
    Counter *processes;
    Counter *vframes;
    Counter *methods;
    Counter *proxies;
    Counter *mixins;
    Counter *associations;
    Counter *contexts;
    Counter *memOops;
    GrowableArray<Counter *> *counters;
public:
    ObjectHistogram();

    Counter *counter( MemOop obj );


    void do_object( MemOop obj ) {
        counter( obj )->update( obj );
    }


    void print();
};


ObjectHistogram::ObjectHistogram() {
    counters = new GrowableArray<Counter *>( 20 );
    counters->push( doubles          = new Counter( "doubles" ) );
    counters->push( blocks           = new Counter( "blocks" ) );
    counters->push( objArrays        = new Counter( "arrays" ) );
    counters->push( symbols          = new Counter( "symbols" ) );
    counters->push( byteArrays       = new Counter( "byte arrays" ) );
    counters->push( doubleByteArrays = new Counter( "double byte arrays" ) );
    counters->push( klasses          = new Counter( "class" ) );
    counters->push( processes        = new Counter( "processes" ) );
    counters->push( vframes          = new Counter( "vframes" ) );
    counters->push( methods          = new Counter( "methods" ) );
    counters->push( proxies          = new Counter( "proxies" ) );
    counters->push( mixins           = new Counter( "mixins" ) );
    counters->push( associations     = new Counter( "associations" ) );
    counters->push( contexts         = new Counter( "contexts" ) );
    counters->push( memOops          = new Counter( "oops" ) );
}


Counter *ObjectHistogram::counter( MemOop obj ) {
    if ( obj->is_double() )
        return doubles;
    if ( obj->is_block() )
        return blocks;
    if ( obj->is_objArray() )
        return objArrays;
    if ( obj->is_symbol() )
        return symbols;        // Must be before byteArray
    if ( obj->is_byteArray() )
        return byteArrays;
    if ( obj->is_doubleByteArray() )
        return doubleByteArrays;
    if ( obj->is_klass() )
        return klasses;
    if ( obj->is_process() )
        return processes;
    if ( obj->is_vframe() )
        return vframes;
    if ( obj->is_method() )
        return methods;
    if ( obj->is_proxy() )
        return proxies;
    if ( obj->is_mixin() )
        return mixins;
    if ( obj->is_association() )
        return associations;
    if ( obj->is_context() )
        return contexts;
    return memOops;
}


void ObjectHistogram::print() {
    _console->print( "Object Histogram" );
    _console->fill_to( 22 );
    _console->print_cr( "number    bytes" );
    Counter *total = new Counter( "Total" );
    counters->sort( &Counter::compare );

    for ( std::size_t i = 0; i < counters->length(); i++ ) {
        Counter *c = counters->at( i );
        if ( c->number > 0 ) {
            c->print( " - " );
            total->add( c );
        }
    }

    total->print( " " );
}


PRIM_DECL_0( debugPrimitives::printObjectHistogram ) {
    PROLOGUE_0( "printObjectHistogram" );
    ResourceMark    rm;
    ObjectHistogram blk;
    Universe::object_iterate( &blk );
    blk.print();
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::deoptimizeStacks ) {
    PROLOGUE_0( "deoptimizeStacks" );
    VM_DeoptimizeStacks op;
    // The operation takes place in the vmProcess
    VMProcess::execute( &op );
    return trueObj;
}


PRIM_DECL_0( debugPrimitives::verify ) {
    PROLOGUE_0( "verify" );
    Universe::verify();
    return trueObj;
}
