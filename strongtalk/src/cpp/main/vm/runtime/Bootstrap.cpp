
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Bootstrap.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/system/os.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
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
#include "vm/utilities/lprintf.hpp"


// -----------------------------------------------------------------------------

Bootstrap::Bootstrap( const std::string & name ) {
    _filename           = name;
    _number_of_oops     = 0;
    _max_number_of_oops = 512 * 1024;
    _objectCount        = 0;
    _oop_table          = new_c_heap_array <Oop>( _max_number_of_oops );
}


Bootstrap::~Bootstrap() {
    FreeHeap( _oop_table );
    Universe::cleanup_after_bootstrap();
}


void Bootstrap::initNameByTypeByte() {

    _nameByTypeByte[ '-' ] = "SMIOop (negative)";
    _nameByTypeByte[ '0' ] = "SMIOop";
    _nameByTypeByte[ '3' ] = "Oop";
    _nameByTypeByte[ 'A' ] = "klassKlass";
    _nameByTypeByte[ 'B' ] = "smiKlass";
    _nameByTypeByte[ 'C' ] = "memOopKlass";
    _nameByTypeByte[ 'D' ] = "byteArrrayKlass";
    _nameByTypeByte[ 'E' ] = "doubleByteArrrayKlass";
    _nameByTypeByte[ 'F' ] = "objArrayKlass";
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
    _nameByTypeByte[ 'a' ] = "klass";
    _nameByTypeByte[ 'b' ] = "smi_t";
    _nameByTypeByte[ 'c' ] = "MemOop";
    _nameByTypeByte[ 'd' ] = "ByteArrayOop";
    _nameByTypeByte[ 'e' ] = "ByteArrayOop";
    _nameByTypeByte[ 'f' ] = "objArrayOop";
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
    open_file();
    parse_file();
    close_file();
    summary();
}


void Bootstrap::open_file() {
    _stream.open( _filename, std::ifstream::binary );

    if ( not _stream.good() ) {
        _console->print_cr( "%%bootstrap-file-error: failed to open file [%s] for reading", _filename.c_str() );
        exit( EXIT_FAILURE );
    }
    _console->print_cr( "%%bootstrap-file-opened [%s]", _filename.c_str() );
    check_version();
}


void Bootstrap::parse_file() {

    _console->print_cr( "%%bootstrap-file-load: [%s]", _filename.c_str() );
    parse_objects();
    _console->print_cr( "%%bootstrap-file-load-complete: [%d] objects read from [%s]", _objectCount, _filename.c_str() );

}


void Bootstrap::close_file() {
    _stream.close();
    _console->print_cr( "%%bootstrap-file-closed [%s]", _filename.c_str() );
}


void Bootstrap::summary() {
    initNameByTypeByte();
    for ( auto item : _countByType ) {
        _console->print_cr( "%%bootstrap-object-count    %-4c %-24s %d", item.first, _nameByTypeByte[ item.first ].c_str(), item.second );
    }
}


// -----------------------------------------------------------------------------

void Bootstrap::extend_oop_table() {
    int new_size = _max_number_of_oops * 2;

    _console->print_cr( "Expanding boot table to [0x%08x]", new_size );
    Oop * new_oop_table = new_c_heap_array <Oop>( new_size );

    for ( int i = 0; i < _max_number_of_oops; i++ )
        new_oop_table[ i ] = _oop_table[ i ];

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

char Bootstrap::readNextChar() {
    return _stream.get();
}


int Bootstrap::get_integer() {

    uint8_t lo;
    _stream.read( reinterpret_cast<char *>(&lo), 1 );

    if ( lo < 128 ) {
        return lo;
    }

    int hi = get_integer();
    return ( hi * 128 ) + ( lo % 128 );
}


uint16_t Bootstrap::read_doubleByte() {
    return ( uint16_t ) get_integer();
}


char Bootstrap::read_byte() {
    return readNextChar();
}


// -----------------------------------------------------------------------------

void Bootstrap::read_oop( Oop * oop_addr ) {
    *oop_addr = readNextObject();
}


// -----------------------------------------------------------------------------

void Bootstrap::check_version() {

    int version_number = get_integer();
    if ( version_number > 100 ) {
        _new_format = true;
        version_number -= 100;
    } else {
        _new_format = false;
    }

    int expected = ByteCodes::version();
    int observed = version_number;
    if ( expected != observed ) {
        lprintf( "fatal: filename [%s] has unexpected bytecode version: expected: [0x%08x], observed: [0x%08x]\n", _filename.c_str(), expected, observed );
        exit( EXIT_FAILURE );
    }

}


void Bootstrap::parse_objects() {

    Universe::_systemDictionaryObj = ObjectArrayOop( readNextObject() );
    nilObj                         = MemOop( readNextObject() );
    trueObj                        = MemOop( readNextObject() );
    falseObj                       = MemOop( readNextObject() );
    smiKlassObj                    = KlassOop( readNextObject() );
    Universe::_memOopKlassObj      = KlassOop( readNextObject() );
    Universe::_objArrayKlassObj    = KlassOop( readNextObject() );
    Universe::_byteArrayKlassObj   = KlassOop( readNextObject() );
    symbolKlassObj                 = KlassOop( readNextObject() );
    doubleKlassObj                 = KlassOop( readNextObject() );
    Universe::_methodKlassObj      = KlassOop( readNextObject() );
    Universe::_associationKlassObj = KlassOop( readNextObject() );
    zeroArgumentBlockKlassObj      = KlassOop( readNextObject() );
    oneArgumentBlockKlassObj       = KlassOop( readNextObject() );
    twoArgumentBlockKlassObj       = KlassOop( readNextObject() );
    threeArgumentBlockKlassObj     = KlassOop( readNextObject() );
    fourArgumentBlockKlassObj      = KlassOop( readNextObject() );
    fiveArgumentBlockKlassObj      = KlassOop( readNextObject() );
    sixArgumentBlockKlassObj       = KlassOop( readNextObject() );
    sevenArgumentBlockKlassObj     = KlassOop( readNextObject() );
    eightArgumentBlockKlassObj     = KlassOop( readNextObject() );
    nineArgumentBlockKlassObj      = KlassOop( readNextObject() );
    contextKlassObj                = KlassOop( readNextObject() );
    Universe::_asciiCharacters     = ObjectArrayOop( readNextObject() );
    Universe::_vframeKlassObj      = KlassOop( readNextObject() );

    KlassOop       platform_klass       = KlassOop( Universe::find_global( os::platform_class_name() ) );
    AssociationOop platform_association = Universe::find_global_association( "Platform" );
    if ( platform_klass not_eq nullptr and platform_association not_eq nullptr ) {
        platform_association->set_value( platform_klass );
    }

}



void Bootstrap::object_error_func( const char * str ) {
    fatal( str );
}


// -----------------------------------------------------------------------------

template <typename T>
void Bootstrap::insert_symbol( MemOop m ) {
    static_cast<T>( m )->bootstrap_object( this );

    if ( Universe::symbol_table->is_present( SymbolOop( static_cast<T>( m ) ) ) ) {
        _console->print_cr( "Symbol [%s] already present in symbol_table!", SymbolOop( static_cast<T>( m ) )->as_string() );
    } else {
        Universe::symbol_table->add_symbol( SymbolOop( static_cast<T>( m ) ) );
    }

}


void Bootstrap::klass_case_func( void (* function)( Klass * ), MemOop m ) {
    function( KlassOop( m )->klass_part() );
    KlassOop( m )->bootstrap_object( this );
}


template <typename T>
void Bootstrap::object_case_func( MemOop m ) {
    static_cast<T>( m )->bootstrap_object( this );
}


Oop Bootstrap::oopFromTable( const int index ) {
    if ( index < 0 or index > _number_of_oops )
        error( "Bootstrap Oop table overflow" );
    return _oop_table[ index ];
}


Oop Bootstrap::readNextObject() {

    // get type byte
    char typeByte = _stream.get();

    // update counters
    _objectCount++;
    _countByType[ typeByte ]++;

    // if SMI or OOP, return it...
    switch ( typeByte ) {
        case '0': //
        {
            int v{ get_integer() };
            return smiOopFromValue( v );
        }

        case '-': //
        {
            int v{ get_integer() };
            return smiOopFromValue( -v );
        }

        case '3': //
        {
            int v{ get_integer() };
            return oopFromTable( v );
        }
    };

    // not one of the above; get size...
    int    size = get_integer();
    MemOop m    = as_memOop( Universe::allocate_tenured( size ) );
    m->raw_at_put( size - 1, smiOop_zero ); // Clear eventual padding area for byteArray, symbol, doubleByteArray.
    add( m );

    // ... call bootstrap_object
    switch ( typeByte ) {

        // Classes
        case 'A': //
            klass_case_func( setKlassVirtualTableFromKlassKlass, m );
            break;
        case 'B': // 
            klass_case_func( setKlassVirtualTableFromSmiKlass, m );
            break;
        case 'C': // 
            klass_case_func( setKlassVirtualTableFromMemOopKlass, m );
            break;
        case 'D': // 
            klass_case_func( setKlassVirtualTableFromByteArrayKlass, m );
            break;
        case 'E': // 
            klass_case_func( setKlassVirtualTableFromDoubleByteArrayKlass, m );
            break;
        case 'F': // 
            klass_case_func( setKlassVirtualTableFromObjArrayKlass, m );
            break;
        case 'G': // 
            klass_case_func( setKlassVirtualTableFromSymbolKlass, m );
            break;
        case 'H': // 
            klass_case_func( setKlassVirtualTableFromDoubleKlass, m );
            break;
        case 'I': // 
            klass_case_func( setKlassVirtualTableFromAssociationKlass, m );
            break;
        case 'J': // 
            klass_case_func( setKlassVirtualTableFromMethodKlass, m );
            break;
        case 'K': // 
            klass_case_func( setKlassVirtualTableFromBlockClosureKlass, m );
            break;
        case 'L': // 
            klass_case_func( setKlassVirtualTableFromContextKlass, m );
            break;
        case 'M': // 
            klass_case_func( setKlassVirtualTableFromProxyKlass, m );
            break;
        case 'N': // 
            klass_case_func( setKlassVirtualTableFromMixinKlass, m );
            break;
        case 'O': // 
            klass_case_func( setKlassVirtualTableFromWeakArrayKlass, m );
            break;
        case 'P': // 
            klass_case_func( setKlassVirtualTableFromProcessKlass, m );
            break;
        case 'Q': // 
            klass_case_func( setKlassVirtualTableFromDoubleValueArrayKlass, m );
            break;
        case 'R': // 
            klass_case_func( setKlassVirtualTableFromVirtualFrameKlass, m );
            break;

            // Objects
        case 'a': // 
        fatal( "klass" );
            break;
        case 'b': // 
        fatal( "smi_t" );
            break;
        case 'c': // 
            object_case_func <MemOop>( m );
            break;
        case 'd': // 
            object_case_func <ByteArrayOop>( m );
            break;
        case 'e': // 
            object_case_func <DoubleByteArrayOop>( m );
            break;
        case 'f': // 
            object_case_func <ObjectArrayOop>( m );
            break;
        case 'g': // 
            insert_symbol <SymbolOop>( m );
            break;
        case 'h': // 
            object_case_func <DoubleOop>( m );
            break;
        case 'i': // 
            object_case_func <AssociationOop>( m );
            break;
        case 'j': // 
            object_case_func <MethodOop>( m );
            break;
        case 'k': // 
            fatal( "blockClosure" );
            break;
        case 'l': // 
            fatal( "context" );
            break;
        case 'm': // 
            fatal( "proxy" );
            break;
        case 'n': // 
            object_case_func <MixinOop>( m );
            break;
        case 'o': // 
            fatal( "weakArrayOop" );
            break;
        case 'p': // 
            object_case_func <ProcessOop>( m );
            break;
        default: // 
            fatal( "unknown object typeByte" );
    }

    return m;
}


// -----------------------------------------------------------------------------

void Bootstrap::read_mark( MarkOop * mark_addr ) {
    MarkOop m;
    char    typeByte = _stream.get();

    switch ( typeByte ) {
        case '1': //
            m = MarkOopDescriptor::untagged_prototype();
            break;
        case '2': //
            m = MarkOopDescriptor::tagged_prototype();
            break;
        default: //
            fatal( "expecting a markup" );
    }
    *mark_addr = m;
}


double Bootstrap::read_double() {
    double  value;
    uint8_t * str = ( uint8_t * ) &value;

    for ( int i = 0; i < 8; i++ ) {
        char c{};
        _stream.get( c );

        str[ i ] = c;
    }

    return value;
}


bool_t Bootstrap::is_byte() {
    return read_byte() == '4';
}


bool_t Bootstrap::new_format() const {
    return _new_format;
}


/*

    if ( TraceBootstrap )
        _console->print_cr( "%8i  address [0x%lx]  size [0x%04x]  type [%c]", _objectCount, m, size, type );
    else
        if ( _objectCount % 1000 == 0 ) {
            if ( _objectCount % 10000 == 0 ) {
                _console->print( "%d", _objectCount );
            }
            _console->print( "." );
        }

*/
