//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/os.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/runtime/ResourceMark.hpp"


const char *InliningDatabase::_directory = nullptr;

InliningDatabaseKey *InliningDatabase::_table = nullptr;
std::uint32_t            InliningDatabase::_table_size      = 0;
std::uint32_t            InliningDatabase::_table_size_mask = 0;
std::uint32_t            InliningDatabase::_table_no        = 0;


const char *InliningDatabase::default_directory() {
    return "./.inlining";
}


void InliningDatabase::set_directory( const char *dir ) {
    _directory = dir;
}


const char *InliningDatabase::directory() {
    return _directory == nullptr ? default_directory() : _directory;
}


const char quote         = '_';
const char *quote_string = "_\\/:; *?~|><,+=@%&!-";


const char *InliningDatabase::mangle_name( const char *str ) {
    char        *result = new_resource_array<char>( 100 );
    std::size_t i       = 0;
    int         j       = 0;
    while ( str[ i ] not_eq '\0' ) {
        int c = str[ i ];
        if ( strchr( quote_string, c ) ) {
            result[ j++ ] = quote;
            result[ j++ ] = get_unsigned_bitfield( c, 6, 3 ) + '0';
            result[ j++ ] = get_unsigned_bitfield( c, 3, 3 ) + '0';
            result[ j++ ] = get_unsigned_bitfield( c, 0, 3 ) + '0';
        } else if ( isupper( c ) ) {
            result[ j++ ] = quote;
            result[ j++ ] = c + ( 'a' - 'A' );
        } else {
            result[ j++ ] = c;
        }
        i++;
    }
    result[ j++ ]       = '\0';
    return result;
}


const char *InliningDatabase::unmangle_name( const char *str ) {
    char        *result = new_resource_array<char>( 100 );
    std::size_t i       = 0;
    int         j       = 0;
    while ( str[ i ] not_eq '\0' ) {
        int c = str[ i ];
        if ( c == quote ) {
            i++;
            st_assert( str[ i ] not_eq '\0', "we cannot end with a quote" );
            c = str[ i ];
            if ( isdigit( c ) ) {
                int value = ( c - '0' ) << 6;
                i++;
                st_assert( str[ i ] not_eq '\0', "we cannot end with a quote" );
                c         = str[ i ];
                value += ( c - '0' ) << 3;
                i++;
                st_assert( str[ i ] not_eq '\0', "we cannot end with a quote" );
                c = str[ i ];
                value += ( c - '0' );
                result[ j++ ] = value;
            } else {
                result[ j++ ] = c - ( 'a' - 'A' );
            }
        } else {
            result[ j++ ] = c;
        }
        i++;
    }
    result[ j++ ]       = '\0';
    return result;
}


const char *InliningDatabase::selector_string( SymbolOop selector ) {
    StringOutputStream stream( 100 );
    selector->print_symbol_on( &stream );
    return stream.as_string();
}


char *InliningDatabase::method_string( MethodOop method ) {
    StringOutputStream stream( 100 );
    method->print_inlining_database_on( &stream );
    return stream.as_string();
}


char *InliningDatabase::klass_string( KlassOop klass ) {
    StringOutputStream stream( 100 );
    klass->klass_part()->print_name_on( &stream );
    return stream.as_string();
}


bool_t check_directory( const char *dir_name ) {
    return os::check_directory( dir_name );
}


const char *InliningDatabase::compute_file_name( LookupKey *outer, LookupKey *inner, bool_t create_directories ) {
    char *name = new_resource_array<char>( 1024 );

    // Outer key
    const char *outer_klass_name    = mangle_name( klass_string( outer->klass() ) );
    const char *outer_selector_name = mangle_name( selector_string( outer->selector() ) );

    if ( create_directories ) {
        if ( not check_directory( directory() ) )
            return nullptr;
    }

    strcpy( name, directory() );
    strcat( name, "\\" );
    strcat( name, outer_klass_name );

    if ( create_directories ) {
        if ( not check_directory( name ) )
            return nullptr;
    }

    strcat( name, "\\" );
    strcat( name, outer_selector_name );

    if ( inner ) {
        // Inner key
        const char *inner_klass_name  = mangle_name( klass_string( inner->klass() ) );
        const char *inner_method_name = mangle_name( method_string( inner->method() ) );

        if ( create_directories ) {
            if ( not check_directory( name ) )
                return nullptr;
        }
        strcat( name, "\\" );
        strcat( name, inner_klass_name );

        if ( create_directories ) {
            if ( not check_directory( name ) )
                return nullptr;
        }
        strcat( name, "\\" );
        strcat( name, inner_method_name );
    }
    strcat( name, ".txt" );
    return name;
}


bool_t InliningDatabase::file_out( NativeMethod *nm, ConsoleOutputStream *index_st ) {
    ResourceMark resourceMark;

    LookupKey *outer_key = nullptr;
    LookupKey *inner_key = nullptr;

    if ( nm->is_block() ) {
        NativeMethod *outer = nm->outermost();
        if ( outer->isZombie() )
            return false;
        outer_key = &outer->_lookupKey;
        inner_key = &nm->_lookupKey;
    } else {
        outer_key = &nm->_lookupKey;
        inner_key = nullptr;
    }


    // construct NativeMethod's RecompilationScope tree; we only want the inlined scopes, so use trusted = false
    RecompilationScope *rs = NonDummyRecompilationScope::constructRScopes( nm, false );
    // Ignore nativeMethods with small inlining trees
    if ( rs->inlining_database_size() < InliningDatabasePruningLimit )
        return false;


    add_lookup_entry( outer_key, inner_key );

    if ( index_st ) {
        if ( inner_key ) {
            inner_key->print_inlining_database_on( index_st );
            index_st->cr();
        }
        outer_key->print_inlining_database_on( index_st );
        index_st->cr();
    }

    const char *file_name = compute_file_name( outer_key, inner_key, true );
    if ( file_name == nullptr )
        return false;

    if ( TraceInliningDatabase ) {
        _console->print_cr( "Dumping %s", file_name );
    }
    FileOutputStream out( file_name );
    if ( out.is_open() ) {
        GrowableArray<ProgramCounterDescriptor *> *uncommon = nm->uncommonBranchList();
        if ( TraceInliningDatabase )
            rs->printTree( 0, 0 );
        rs->print_inlining_database_on( &out, uncommon );
        return true;
    }
    return false;
}


char *find_type( const char *line, bool_t *is_super, bool_t *is_block ) {

    char *sub = nullptr;

    // Find "::", "^^" or "->"
    sub = const_cast<char *>( strstr( line + 1, "::" ) );
    if ( sub ) {
        *is_super = false;
        *is_block = false;
        return sub;
    }

    sub = const_cast<char *>( strstr( line + 1, "^^" ) );
    if ( sub ) {
        *is_super = true;
        *is_block = false;
        return sub;
    }

    sub = const_cast<char *>( strstr( line + 1, "->" ) );
    if ( sub ) {
        *is_super = false;
        *is_block = true;
        return sub;
    }

    return nullptr;
}


// Returns whether the key was succesfully scanned
bool_t scan_key( RecompilationScope *sender, char *line, KlassOop *receiver_klass, MethodOop *method ) {
    bool_t is_super;
    bool_t is_block;

    char *sub = find_type( line, &is_super, &is_block );
    if ( sub == nullptr )
        return false;

    *sub = '\0';

    char *class_name = line;
    char *method_id  = sub + 2;

    bool_t class_side   = false;
    char   *class_start = strstr( class_name, " class" );
    if ( class_start not_eq nullptr ) {
        *class_start = '\0';
        class_side = true;
    }

    KlassOop rec = KlassOop( Universe::find_global( class_name, true ) );
    if ( rec == nullptr or not rec->is_klass() )
        return false;
    if ( class_side )
        rec = rec->klass();
    *receiver_klass = rec;

    GrowableArray<int> *byteCodeIndexs = new GrowableArray<int>( 10 );

    char      *byteCodeIndexs_string = strstr( method_id, " " );

    if ( byteCodeIndexs_string ) {
        *byteCodeIndexs_string++ = '\0';
        while ( *byteCodeIndexs_string not_eq '\0' ) {
            int index;
            int byteCodeIndex;
            if ( sscanf( byteCodeIndexs_string, "%d%n", &byteCodeIndex, &index ) not_eq 1 )
                return 0;
            byteCodeIndexs->push( byteCodeIndex );
            byteCodeIndexs_string += index;
            if ( *byteCodeIndexs_string == ' ' )
                byteCodeIndexs_string++;
        }
    }
    SymbolOop selector               = oopFactory::new_symbol( method_id );


    if ( is_super ) {
        st_assert( sender, "sender must be present" );
        KlassOop method_holder = sender->receiverKlass()->klass_part()->lookup_method_holder_for( sender->method() );

        if ( method_holder ) {
            MethodOop met = method_holder->klass_part()->superKlass()->klass_part()->lookup( selector );
            if ( met ) {
                *method = met;
                return true;
            }
        }
        return false;
    }

    MethodOop met = rec->klass_part()->lookup( selector );
    if ( met == nullptr )
        return false;

    for ( std::size_t i = 0; i < byteCodeIndexs->length(); i++ ) {
        int byteCodeIndex = byteCodeIndexs->at( i );
        met = met->block_method_at( byteCodeIndex );
        if ( met == nullptr )
            return false;
    }

    *method = met;
    return true;
}


// Returns the index where the scan terminated.
// index is 0 is the scan failed
int scan_prefix( const char *line, int *byteCodeIndex, int *level ) {
    int index;

    int l = 0;
    while ( *line == ' ' ) {
        line++;
        l++;
    }
    if ( sscanf( line, "%d %n", byteCodeIndex, &index ) not_eq 1 )
        return 0;
    *level = l / 2;
    return l + index;
}


// Returns whether the uncommon word was succesfully scanned
bool_t scan_uncommon( const char *line ) {
    return strcmp( line, "uncommon" ) == 0;
}


static bool_t create_rscope( char *line, GrowableArray<InliningDatabaseRecompilationScope *> *stack ) {

    // remove the cr
    int len = strlen( line );
    if ( len > 1 and line[ len - 1 ] == '\n' )
        line[ len - 1 ] = '\0';

    int       byteCodeIndex  = 0;
    int       level          = 0;
    MethodOop method         = nullptr;
    KlassOop  receiver_klass = nullptr;

    RecompilationScope *result = nullptr;

    if ( stack->isEmpty() ) {
        // the root scope
        if ( not scan_key( nullptr, line, &receiver_klass, &method ) )
            return false;
        stack->push( new InliningDatabaseRecompilationScope( nullptr, -1, receiver_klass, method, 0 ) );
    } else {
        // sub scope
        int index = scan_prefix( line, &byteCodeIndex, &level );
        if ( index <= 0 )
            return false;

        while ( stack->length() > level )
            stack->pop();
        InliningDatabaseRecompilationScope *sender = stack->top();
        if ( scan_uncommon( &line[ index ] ) ) {
            sender->mark_as_uncommon( byteCodeIndex );
        } else if ( scan_key( sender, &line[ index ], &receiver_klass, &method ) ) {
            stack->push( new InliningDatabaseRecompilationScope( sender, byteCodeIndex, receiver_klass, method, level ) );
        } else {
            return false;
        }
    }
    return true;
}


std::size_t      InliningDatabase::local_number_of_nativeMethods_written = 0;
KlassOop InliningDatabase::local_klass = nullptr;


void InliningDatabase::local_file_out_all( NativeMethod *nm ) {
    if ( nm->isZombie() )
        return;
    if ( file_out( nm ) ) {
        local_number_of_nativeMethods_written++;
    }
}


const char *InliningDatabase::index_file_name() {
    char *name = new_resource_array<char>( 1024 );
    if ( not check_directory( directory() ) )
        return nullptr;
    strcpy( name, directory() );
    strcat( name, "/index.txt" );
    return name;
}


bool_t scan_key( char *line, LookupKey *key ) {

    int len = strlen( line );
    if ( len > 1 and line[ len - 1 ] == '\n' )
        line[ len - 1 ] = '\0';

    bool_t is_super;
    bool_t is_block;

    char *sub = find_type( line, &is_super, &is_block );
    if ( sub == nullptr )
        return false;

    *sub = '\0';

    char *class_name = line;
    char *method_id  = sub + 2;

    bool_t class_side   = false;
    char   *class_start = strstr( class_name, " class" );
    if ( class_start not_eq nullptr ) {
        *class_start = '\0';
        class_side = true;
    }

    KlassOop rec = KlassOop( Universe::find_global( class_name, true ) );
    if ( rec == nullptr or not rec->is_klass() )
        return false;
    if ( class_side )
        rec = rec->klass();


    GrowableArray<int> *byteCodeIndexs = new GrowableArray<int>( 10 );

    char      *byteCodeIndexs_string = strstr( method_id, " " );

    if ( byteCodeIndexs_string ) {
        *byteCodeIndexs_string++ = '\0';
        while ( *byteCodeIndexs_string not_eq '\0' ) {
            int index;
            int byteCodeIndex;
            if ( sscanf( byteCodeIndexs_string, "%d%n", &byteCodeIndex, &index ) not_eq 1 )
                return 0;
            byteCodeIndexs->push( byteCodeIndex );
            byteCodeIndexs_string += index;
            if ( *byteCodeIndexs_string == ' ' )
                byteCodeIndexs_string++;
        }
    }
    SymbolOop selector               = oopFactory::new_symbol( method_id );

    if ( is_block ) {
        MethodOop met = rec->klass_part()->lookup( selector );
        if ( met == nullptr )
            return false;
        for ( std::size_t i = 0; i < byteCodeIndexs->length(); i++ ) {
            int byteCodeIndex = byteCodeIndexs->at( i );
            met = met->block_method_at( byteCodeIndex );
            if ( met == nullptr )
                return false;
        }
        key->initialize( rec, met );
    } else {
        key->initialize( rec, selector );
    }
    return true;
}


void InliningDatabase::load_index_file() {
    ResourceMark resourceMark;
    TraceTime    t( "Loading index for inlining database" );

    // Open the file
    std::ifstream stream( index_file_name() );

    if ( not stream.good() )
        return;

    char line[1000];

    LookupKey first;
    LookupKey second;

    while ( stream.getline( line, 1000 ) ) {
        if ( scan_key( line, &first ) ) {
            if ( first.is_block_type() ) {
                if ( stream.getline( line, 1000 ) ) {
                    if ( scan_key( line, &second ) ) {
                        // _console->print("Block ");
                        // first.print_on(_console);
                        // _console->print(" outer ");
                        // second.print_on(_console);
                        // _console->cr();
                        add_lookup_entry( &second, &first );
                    } else {
                        _console->print_cr( "%%inlining-database-index-file: filename [%s], parsing block failed for [%s]", index_file_name(), line );

                    }
                }
            } else {
                // _console->print("Method ");
                // first.print_on(_console);
                // _console->cr();
                add_lookup_entry( &first );
            }
        } else {
            _console->print_cr( "%%inlining-database-index-file: filename [%s], parsing failed for [%s]", index_file_name(), line );

        }
    }
    stream.close();
}


void InliningDatabase::local_file_out_klass( NativeMethod *nm ) {
    if ( nm->isZombie() )
        return;
    if ( nm->receiver_klass() == local_klass ) {
        if ( file_out( nm ) ) {
            local_number_of_nativeMethods_written++;
        }
    }
}


int InliningDatabase::file_out( KlassOop klass ) {
    local_number_of_nativeMethods_written = 0;
    local_klass                           = klass;
    Universe::code->nativeMethods_do( local_file_out_klass );
    return local_number_of_nativeMethods_written;
}


RecompilationScope *InliningDatabase::file_in_from( std::ifstream &stream ) {

    auto stack = new GrowableArray<InliningDatabaseRecompilationScope *>( 10 );

    char line[1000];

    // Read the first top scope
    if ( not stream.getline( line, 1000 ) )
        return nullptr;

    if ( not create_rscope( line, stack ) )
        return nullptr;

    // Read the sub scopes
    while ( stream.getline( line, 1000 ) ) {
        if ( not create_rscope( line, stack ) )
            return nullptr;
    }

    // Return the top scope
    return stack->at( 0 );
}


RecompilationScope *InliningDatabase::file_in( const char *file_path ) {

    std::ifstream stream( file_path );

    if ( not stream.good() ) {
        return nullptr;
    }
    RecompilationScope *result = file_in_from( stream );

    stream.close();
    return result;
}


RecompilationScope *InliningDatabase::file_in( LookupKey *outer, LookupKey *inner ) {
    const char *file_name = compute_file_name( outer, inner, false );
    if ( file_name == nullptr ) {
        if ( TraceInliningDatabase ) {
            _console->print( "Failed opening file for " );
            if ( inner ) {
                inner->print();
                _console->print( " " );
            }
            outer->print();
            _console->cr();
        }
        return nullptr;
    }
    RecompilationScope *result = file_in( file_name );

    if ( TraceInliningDatabase and result == nullptr ) {
        _console->print( "Failed parsing file for " );
        if ( inner ) {
            inner->print();
            _console->print( " " );
        }
        outer->print();
        _console->cr();
    }

    return result;
}


std::uint32_t InliningDatabase::index_for( LookupKey *outer, LookupKey *inner ) {
    std::uint32_t hash = (std::uint32_t) outer->klass()->identity_hash() ^(std::uint32_t) outer->selector()->identity_hash();
    if ( inner ) {
        hash ^= (std::uint32_t) inner->klass()->identity_hash() ^ (std::uint32_t) inner->selector()->identity_hash();
    }
    return hash & _table_size_mask;
}


std::uint32_t InliningDatabase::next_index( std::uint32_t index ) {
    return ( index + 1 ) & _table_size_mask;
}


void InliningDatabase::reset_lookup_table() {

    if ( not _table )
        return;

    FreeHeap( _table );
    _table           = nullptr;
    _table_size      = 0;
    _table_size_mask = 0;
    _table_no        = 0;
}


RecompilationScope *InliningDatabase::select_and_remove( bool_t *end_of_table ) {

    if ( _table_no == 0 )
        return nullptr;

    for ( std::uint32_t index = 0; index < _table_size; index++ ) {
        if ( _table[ index ].is_filled() and _table[ index ].is_outer() ) {
            RecompilationScope *result = file_in( &_table[ index ]._outer );
            _table[ index ].set_deleted();
            _table_no--;
            *end_of_table = false;
            return result;
        }
    }

    *end_of_table = true;

    return nullptr;
}


void InliningDatabase::allocate_table( std::uint32_t size ) {

    if ( TraceInliningDatabase ) {
        _console->print_cr( "InliningDatabase::allocate_table(%d)", size );
    }

    _table_size      = size;
    _table_size_mask = size - 1;
    _table_no        = 0;
    _table           = new_c_heap_array<InliningDatabaseKey>( _table_size );

    for ( std::uint32_t index = 0; index < _table_size; index++ ) {
        _table[ index ].clear();
    }

}


void InliningDatabase::add_lookup_entry( LookupKey *outer, LookupKey *inner ) {
    if ( _table_no * 2 >= _table_size ) {
        if ( _table == nullptr ) {
            allocate_table( 4 * 1024 );
        } else {
            // Expand table
            InliningDatabaseKey *old_table     = _table;
            std::uint32_t       old_table_size = _table_size;
            allocate_table( _table_size * 2 );
            for ( std::uint32_t index = 0; index < old_table_size; index++ ) {
                if ( old_table[ index ].is_filled() )
                    add_lookup_entry( &old_table[ index ]._outer, &old_table[ index ]._inner );
            }
            FreeHeap( old_table );
        }
    }
    st_assert( _table_no * 2 < _table_size, "just checking density" );

    std::uint32_t index = index_for( outer, inner );

    while ( _table[ index ].is_filled() ) {
        if ( _table[ index ].equal( outer, inner ) )
            return;
        index = next_index( index );
    }

    _table[ index ]._outer = *outer;
    if ( inner ) {
        _table[ index ]._inner = *inner;
    }
    _table_no++;

    if ( TraceInliningDatabase ) {
        _console->print_cr( "InliningDatabase::add_lookup_entry @ %d", index );
        if ( inner ) {
            inner->print();
            _console->print( " " );
        }
        outer->print();
        _console->cr();
    }
}


bool_t InliningDatabase::lookup( LookupKey *outer, LookupKey *inner ) {
    if ( _table_no == 0 )
        return false;

    std::uint32_t index = index_for( outer, inner );
    if ( not _table[ index ].is_filled() )
        return false;
    while ( not _table[ index ].equal( outer, inner ) ) {
        index = next_index( index );
        if ( _table[ index ].is_empty() )
            return false;
    }
    return true;
}


RecompilationScope *InliningDatabase::lookup_and_remove( LookupKey *outer, LookupKey *inner ) {
    if ( _table_no == 0 )
        return nullptr;

    std::uint32_t index = index_for( outer, inner );
    if ( not _table[ index ].is_filled() )
        return nullptr;

    while ( not _table[ index ].equal( outer, inner ) ) {
        index = next_index( index );
        if ( _table[ index ].is_empty() )
            return nullptr;
    }
    _table[ index ].set_deleted();
    _table_no--;
    return file_in( outer, inner );
}


void InliningDatabase::oops_do( void f( Oop * ) ) {
    for ( std::uint32_t index = 0; index < _table_size; index++ ) {
        _table[ index ].oops_do( f );
    }
}


bool_t InliningDatabase::file_out_all() {
    ResourceMark resourceMark;

    // The lookup table is used to create the index file.

    // Flush the lookup table
    reset_lookup_table();

    // File in the current index file.
    load_index_file();

    local_number_of_nativeMethods_written = 0;
    Universe::code->nativeMethods_do( local_file_out_all );

    FileOutputStream index( index_file_name() );

    // File out the index file.
    for ( std::uint32_t i = 0; i < _table_size; i++ ) {
        if ( _table[ i ].is_filled() ) {
            if ( _table[ i ].is_inner() ) {
                _table[ i ]._inner.print_inlining_database_on( &index );
                index.cr();
            }
            _table[ i ]._outer.print_inlining_database_on( &index );
            index.cr();
        }
    }

    // Flush the lookup table
    reset_lookup_table();

    return local_number_of_nativeMethods_written ? true : false;
}
