
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Bootstrap.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/system/os.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/oops/KlassKlass.hpp"
#include "vm/oops/SMIKlass.hpp"
#include "vm/oops/DoubleByteArrayKlass.hpp"
#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/oops/SymbolKlass.hpp"
#include "vm/oops/DoubleKlass.hpp"
#include "vm/oops/AssociationKlass.hpp"
#include "vm/oops/MethodKlass.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/oops/ProxyKlass.hpp"
#include "vm/oops/MixinKlass.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/oops/ProcessKlass.hpp"
#include "vm/oops/DoubleValueArrayKlass.hpp"
#include "vm/oops/VirtualFrameKlass.hpp"
#include "vm/oops/ContextKlass.hpp"


Bootstrap::Bootstrap( const std::string &name ) :
    _oop_table{ nullptr },
    _number_of_oops{ 0 },
    _max_number_of_oops{ 512 * 1024 },
    _new_format{ false },
    _objectCount{ 0 },
    _filename{ name },
    _stream{},
    _counter{ 0 },
    _countByType{},
    _nameByTypeByte{},
    _version_number{ 0 } {

    //
    initNameByTypeByte();

    //
    _oop_table = new_c_heap_array<Oop>( _max_number_of_oops );
}


Bootstrap::~Bootstrap() {
    FreeHeap( _oop_table );
    Universe::cleanup_after_bootstrap();
}


void Bootstrap::initNameByTypeByte() {

    //
    _nameByTypeByte[ '-' ] = "SmallIntegerOop (negative)";
    _nameByTypeByte[ '0' ] = "SmallIntegerOop";
    _nameByTypeByte[ '3' ] = "Oop";

    //
    _nameByTypeByte[ 'A' ] = "klassKlass";
    _nameByTypeByte[ 'B' ] = "smiKlass";
    _nameByTypeByte[ 'C' ] = "memOopKlass";
    _nameByTypeByte[ 'D' ] = "byteArrrayKlass";
    _nameByTypeByte[ 'E' ] = "doubleByteArrrayKlass";
    _nameByTypeByte[ 'F' ] = "objectArrayKlass";
    _nameByTypeByte[ 'G' ] = "symbolKlass";
    _nameByTypeByte[ 'H' ] = "doubleKlass";
    _nameByTypeByte[ 'I' ] = "associationKlass";
    _nameByTypeByte[ 'J' ] = "methodKlass";
    _nameByTypeByte[ 'K' ] = "blockClosureKlass";
    _nameByTypeByte[ 'L' ] = "contextKlass";
    _nameByTypeByte[ 'M' ] = "proxyKlass";
    _nameByTypeByte[ 'N' ] = "mixinKlass";
    _nameByTypeByte[ 'O' ] = "weakArrayKlass";
    _nameByTypeByte[ 'P' ] = "processKlass";
    _nameByTypeByte[ 'Q' ] = "doubleValueArrayKlass";
    _nameByTypeByte[ 'R' ] = "vframeKlass";

    //
    _nameByTypeByte[ 'a' ] = "klass";
    _nameByTypeByte[ 'b' ] = "small_int_t";
    _nameByTypeByte[ 'c' ] = "MemOop";
    _nameByTypeByte[ 'd' ] = "ByteArrayOop";
    _nameByTypeByte[ 'e' ] = "ByteArrayOop";
    _nameByTypeByte[ 'f' ] = "objectArrayOop";
    _nameByTypeByte[ 'g' ] = "SymbolOop";
    _nameByTypeByte[ 'h' ] = "DoubleOop";
    _nameByTypeByte[ 'i' ] = "associationOop";
    _nameByTypeByte[ 'j' ] = "methodOop";
    _nameByTypeByte[ 'k' ] = "blockClosure";
    _nameByTypeByte[ 'l' ] = "context";
    _nameByTypeByte[ 'm' ] = "proxy";
    _nameByTypeByte[ 'n' ] = "mixinOop";
    _nameByTypeByte[ 'o' ] = "weakArrayOop";
    _nameByTypeByte[ 'p' ] = "processOop";

}


// -----------------------------------------------------------------------------

void Bootstrap::load() {
    SPDLOG_INFO( "%booststrap-load: entry" );
    open_file();
    parse_file();
    close_file();
    summary();
    SPDLOG_INFO( "%booststrap-load: exit" );
}


void Bootstrap::open_file() {
    _stream.open( _filename, std::ifstream::binary );

    if ( not _stream.good() ) {
        SPDLOG_INFO( "%bootstrap-file-error: failed to open file [{}] for reading", _filename.c_str() );
        exit( EXIT_FAILURE );
    }
    SPDLOG_INFO( "%bootstrap-file-open: [{}]", _filename.c_str() );

    _version_number = read_uint32_t();
    check_version();
}


void Bootstrap::parse_file() {

    SPDLOG_INFO( "bootstrap-file-load: [{}]", _filename.c_str() );
    parse_objects();
    SPDLOG_INFO( "bootstrap-file-load-done: [{}] objects read from [{}]", _objectCount, _filename.c_str() );

}


void Bootstrap::close_file() {
    _stream.close();
    SPDLOG_INFO( "bootstrap-file-close: [{}]", _filename.c_str() );
}


void Bootstrap::summary() {
    for ( auto item : _countByType ) {
        SPDLOG_INFO( "bootstrap-object-count:    {} {:24s} {}", item.first, _nameByTypeByte[ item.first ].c_str(), item.second );
    }
}


// -----------------------------------------------------------------------------

void Bootstrap::extend_oop_table() {
    std::int32_t new_size = _max_number_of_oops * 2;

    SPDLOG_INFO( "bootstrap-expand: expanding oop_table to [0x{08:x}]", new_size );
    Oop *new_oop_table = new_c_heap_array<Oop>( new_size );

    for ( std::int32_t i = 0; i < _max_number_of_oops; i++ ) {
        new_oop_table[ i ] = _oop_table[ i ];
    }

    _max_number_of_oops = new_size;
    FreeHeap( _oop_table );
    _oop_table = new_oop_table;
}


void Bootstrap::add( Oop obj ) {

    if ( _number_of_oops >= _max_number_of_oops ) {
        extend_oop_table();
    }

    _oop_table[ _number_of_oops++ ] = obj;

}


// -----------------------------------------------------------------------------

char Bootstrap::read_uint8_t() {
    std::uint8_t byte;
    _stream.read( reinterpret_cast<char *>(&byte), 1 );
    return byte;
}


std::uint16_t Bootstrap::read_uint16_t() {
    return (std::uint16_t) read_uint32_t();
}


std::int32_t Bootstrap::read_uint32_t() {

    std::uint8_t lowByte = read_uint8_t();
    if ( lowByte < 128 ) {
        return lowByte;
    }

    std::int32_t highByte = read_uint32_t();
    return ( highByte * 128 ) + ( lowByte % 128 );
}


// -----------------------------------------------------------------------------

void Bootstrap::read_oop( Oop *oop_addr ) {
    *oop_addr = readNextObject();
}


// -----------------------------------------------------------------------------

void Bootstrap::check_version() {

    if ( _version_number > 100 ) {
        _new_format = true;
        _version_number -= 100;
    } else {
        _new_format = false;
    }

    std::int32_t expected = ByteCodes::version();
    std::int32_t observed = _version_number;
    if ( expected != observed ) {
        SPDLOG_INFO( "fatal: filename[{}] has unexpected bytecode version: expected: [0x{08:x}], observed: [0x{08:x}]", _filename.c_str(), expected, observed );
        exit( EXIT_FAILURE );
    }

}


void Bootstrap::parse_objects() {

    SPDLOG_INFO( "%booststrap-parse-objects: entry" );

    Universe::_systemDictionaryObject = ObjectArrayOop( readNextObject() );
    nilObject                         = MemOop( readNextObject() );
    trueObject                        = MemOop( readNextObject() );
    falseObject                       = MemOop( readNextObject() );
    smiKlassObject                    = KlassOop( readNextObject() );
    Universe::_memOopKlassObject      = KlassOop( readNextObject() );
    Universe::_objectArrayKlassObject = KlassOop( readNextObject() );
    Universe::_byteArrayKlassObject   = KlassOop( readNextObject() );
    symbolKlassObject                 = KlassOop( readNextObject() );
    doubleKlassObject                 = KlassOop( readNextObject() );
    Universe::_methodKlassObject      = KlassOop( readNextObject() );
    Universe::_associationKlassObject = KlassOop( readNextObject() );
    zeroArgumentBlockKlassObject      = KlassOop( readNextObject() );
    oneArgumentBlockKlassObject       = KlassOop( readNextObject() );
    twoArgumentBlockKlassObject       = KlassOop( readNextObject() );
    threeArgumentBlockKlassObject     = KlassOop( readNextObject() );
    fourArgumentBlockKlassObject      = KlassOop( readNextObject() );
    fiveArgumentBlockKlassObject      = KlassOop( readNextObject() );
    sixArgumentBlockKlassObject       = KlassOop( readNextObject() );
    sevenArgumentBlockKlassObject     = KlassOop( readNextObject() );
    eightArgumentBlockKlassObject     = KlassOop( readNextObject() );
    nineArgumentBlockKlassObject      = KlassOop( readNextObject() );
    contextKlassObject                = KlassOop( readNextObject() );
    Universe::_asciiCharacters        = ObjectArrayOop( readNextObject() );
    Universe::_vframeKlassObject      = KlassOop( readNextObject() );

    KlassOop       platform_klass       = KlassOop( Universe::find_global( os::platform_class_name() ) );
    AssociationOop platform_association = Universe::find_global_association( "Platform" );
    if ( platform_klass not_eq nullptr and platform_association not_eq nullptr ) {
        platform_association->set_value( platform_klass );
    }

    SPDLOG_INFO( "%booststrap-parse-objects: exit" );

}


// -----------------------------------------------------------------------------

template<typename T>
void Bootstrap::insert_symbol( MemOop m ) {
    static_cast<T>( m )->bootstrap_object( this );

    SymbolOopDescriptor *symbolOop = SymbolOop( static_cast<T>( m ) );

    if ( Universe::symbol_table->is_present( symbolOop ) ) {
        SPDLOG_INFO( "Symbol[{}] already present in symbol_table", symbolOop->as_string() );
    } else {
        Universe::symbol_table->add_symbol( symbolOop );
    }

}


void Bootstrap::klass_case_func( void (*function)( Klass * ), MemOop m ) {
    function( KlassOop( m )->klass_part() );
    KlassOop( m )->bootstrap_object( this );
}


template<typename T>
void Bootstrap::object_case_func( MemOop m ) {
    static_cast<T>( m )->bootstrap_object( this );
}


Oop Bootstrap::oopFromTable( const std::int32_t index ) {
    if ( index < 0 or index > _number_of_oops ) {
        error( "Bootstrap Oop table overflow" );
    }
    return _oop_table[ index ];
}


inline char Bootstrap::getNextTypeByte() {

    // get type byte
    char typeByte = _stream.get();

    // update counters
    _countByType[ typeByte ]++;
    _objectCount++;

    return typeByte;
}


Oop Bootstrap::readNextObject() {

    // get next byte
    char typeByte = getNextTypeByte();

    // if SMI or OOP, return it...
    switch ( typeByte ) {
        case '0': //
            return smiOopFromValue( read_uint32_t() );

        case '-': //
            return smiOopFromValue( -1 * read_uint32_t() );

        case '3': //
            return oopFromTable( read_uint32_t() );

    };


    // not one of the above; get size...
    std::int32_t size   = read_uint32_t();
    MemOop       memOop = as_memOop( Universe::allocate_tenured( size ) );

    // Clear eventual padding area for byteArray, symbol, doubleByteArray
    memOop->raw_at_put( size - 1, smiOop_zero );
    add( memOop );

    // ... call bootstrap_object
    switch ( typeByte ) {

        // Classes
        case 'A': //
            klass_case_func( setKlassVirtualTableFromKlassKlass, memOop );
            break;
        case 'B': // 
            klass_case_func( setKlassVirtualTableFromSmiKlass, memOop );
            break;
        case 'C': // 
            klass_case_func( setKlassVirtualTableFromMemOopKlass, memOop );
            break;
        case 'D': // 
            klass_case_func( setKlassVirtualTableFromByteArrayKlass, memOop );
            break;
        case 'E': // 
            klass_case_func( setKlassVirtualTableFromDoubleByteArrayKlass, memOop );
            break;
        case 'F': // 
            klass_case_func( setKlassVirtualTableFromObjectArrayKlass, memOop );
            break;
        case 'G': // 
            klass_case_func( setKlassVirtualTableFromSymbolKlass, memOop );
            break;
        case 'H': // 
            klass_case_func( setKlassVirtualTableFromDoubleKlass, memOop );
            break;
        case 'I': // 
            klass_case_func( setKlassVirtualTableFromAssociationKlass, memOop );
            break;
        case 'J': // 
            klass_case_func( setKlassVirtualTableFromMethodKlass, memOop );
            break;
        case 'K': // 
            klass_case_func( setKlassVirtualTableFromBlockClosureKlass, memOop );
            break;
        case 'L': // 
            klass_case_func( setKlassVirtualTableFromContextKlass, memOop );
            break;
        case 'M': // 
            klass_case_func( setKlassVirtualTableFromProxyKlass, memOop );
            break;
        case 'N': // 
            klass_case_func( setKlassVirtualTableFromMixinKlass, memOop );
            break;
        case 'O': // 
            klass_case_func( setKlassVirtualTableFromWeakArrayKlass, memOop );
            break;
        case 'P': // 
            klass_case_func( setKlassVirtualTableFromProcessKlass, memOop );
            break;
        case 'Q': // 
            klass_case_func( setKlassVirtualTableFromDoubleValueArrayKlass, memOop );
            break;
        case 'R': // 
            klass_case_func( setKlassVirtualTableFromVirtualFrameKlass, memOop );
            break;

            // Objects
        case 'a': // 
        st_fatal( "klass" );
            break;
        case 'b': // 
        st_fatal( "small_int_t" );
            break;
        case 'c': // 
            object_case_func<MemOop>( memOop );
            break;
        case 'd': // 
            object_case_func<ByteArrayOop>( memOop );
            break;
        case 'e': // 
            object_case_func<DoubleByteArrayOop>( memOop );
            break;
        case 'f': // 
            object_case_func<ObjectArrayOop>( memOop );
            break;
        case 'g': // 
            insert_symbol<SymbolOop>( memOop );
            break;
        case 'h': // 
            object_case_func<DoubleOop>( memOop );
            break;
        case 'i': // 
            object_case_func<AssociationOop>( memOop );
            break;
        case 'j': // 
            object_case_func<MethodOop>( memOop );
            break;
        case 'k': // 
        st_fatal( "blockClosure" );
            break;
        case 'l': // 
        st_fatal( "context" );
            break;
        case 'm': // 
        st_fatal( "proxy" );
            break;
        case 'n': // 
            object_case_func<MixinOop>( memOop );
            break;
        case 'o': // 
        st_fatal( "weakArrayOop" );
            break;
        case 'p': // 
            object_case_func<ProcessOop>( memOop );
            break;
        default: // 
        st_fatal( "unknown object typeByte" );
    }

//    SPDLOG_INFO( "[{:2d}] [{:s}]", size, memOop->print_value_string() );

    return memOop;
}


// -----------------------------------------------------------------------------

void Bootstrap::read_mark( MarkOop *mark_addr ) {

    char typeByte{ 0 };
    _stream.get( typeByte );

    MarkOop m{ nullptr };

    switch ( typeByte ) {
        case '1': //
            m = MarkOopDescriptor::untagged_prototype();
            break;
        case '2': //
            m = MarkOopDescriptor::tagged_prototype();
            break;
        default: //
        st_fatal( "expecting a markup" );
            break;
    }

    *mark_addr = m;
}


double Bootstrap::read_double() {
    double       value{ 0 };
    std::uint8_t *str = (std::uint8_t *) &value;

    for ( std::int32_t i = 0; i < 8; i++ ) {
        char c{ 0 };
        _stream.get( c );

        str[ i ] = c;
    }

    return value;
}


bool Bootstrap::is_byte() {
    return read_uint8_t() == '4';
}


bool Bootstrap::new_format() const {
    return _new_format;
}


/*

    if ( TraceBootstrap )
        SPDLOG_INFO( "%8i  address [0x%lx]  size [0x%04x]  type [%c]", _objectCount, m, size, type );
    else
        if ( _objectCount % 1000 == 0 ) {
            if ( _objectCount % 10000 == 0 ) {
                _console->print( "%d", _objectCount );
            }
            _console->print( "." );
        }

*/
