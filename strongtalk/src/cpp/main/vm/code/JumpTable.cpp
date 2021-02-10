//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/system/os.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/disassembler.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"


static const char halt_instruction = '\xF4';
static const char jump_instruction = '\xE9';


const char *JumpTable::allocate_jump_entries( std::int32_t size ) {
//  return AllocateHeap(size * JumpTableEntry::size(), "jump table");
    return os::exec_memory( size * JumpTableEntry::size() ); //, "jump table");
}


JumpTableEntry *JumpTable::jump_entry_for_at( const char *entries, std::int32_t index ) {
    return (JumpTableEntry *) &entries[ index * JumpTableEntry::size() ];
}


JumpTable::JumpTable() :
    _firstFree{ 0 },
    _entries{ nullptr },
    length{ Universe::current_sizes._jump_table_size },
    usedIDs{ 0 } {
    st_assert( length < 32 * 1024, "must change code to handle >32K entries" );
    _entries = allocate_jump_entries( length );
    init();
}


void JumpTable::init() {
    // free list: firstFree keeps first free index
    // entries[firstFree] keeps index of next free element, etc.
    for ( std::size_t i = 0; i < length; i++ ) {
        major_at( i )->initialize_as_unused( i + 1 );
    }
}


JumpTableID JumpTable::allocate( std::int32_t number_of_entries ) {
    std::int32_t   id     = newID();
    JumpTableEntry *entry = major_at( id );

    st_assert( entry->is_unused(), "cannot allocate used jump entry" );
    if ( number_of_entries == 1 ) {
        entry->initialize_NativeMethod_stub( nullptr );
        return JumpTableID( id );
    } else {
        const char *new_block = allocate_jump_entries( number_of_entries );
        // Initialize the first entry as a NativeMethod stub
        jump_entry_for_at( new_block, 0 )->initialize_NativeMethod_stub( nullptr );
        // initialize the rest as block closure stubs
        for ( std::size_t i = 1; i < number_of_entries; i++ ) {
            jump_entry_for_at( new_block, i )->initialize_block_closure_stub();
        }
        entry->initialize_as_link( new_block );
        return JumpTableID( id, 0 );
    }
}


JumpTable::~JumpTable() {
    free( const_cast<char *>( _entries ) );
}


JumpTableEntry *JumpTable::major_at( std::uint16_t index ) {
    return jump_entry_for_at( _entries, index );
}


JumpTableEntry *JumpTable::at( JumpTableID id ) {
    st_assert( id.is_valid(), "invalid ID" );
    if ( not id.has_minor() )
        return major_at( id.major() );
    return jump_entry_for_at( major_at( id._major )->link(), id._minor );
}


std::int32_t JumpTable::newID() {
    std::int32_t id = _firstFree;
    if ( id >= length - 2 ) st_fatal( "grow not implemented" );
    _firstFree = major_at( _firstFree )->next_free();
    usedIDs++;
    return id;
}


std::int32_t JumpTable::peekID() {
    return _firstFree;
}


void JumpTable::freeID( std::int32_t index ) {
    st_assert( index >= 0 and index < length and index not_eq _firstFree, "invalid ID" );
    if ( major_at( index )->is_link() ) {
        // free the chunk
        free( const_cast<char *>( major_at( index )->link() ) );
    }
    major_at( index )->initialize_as_unused( _firstFree );
    _firstFree = index;
    usedIDs--;
    st_assert( usedIDs >= 0, "freed too many IDs" );
}


void JumpTable::print() {
    SPDLOG_INFO( "JumpTable 0x{0:x}: capacity{0:d} (%ld used)", static_cast<const void *>(this), length, usedIDs );
    for ( std::size_t i = 0; i < length; i++ ) {
        if ( not major_at( i )->is_unused() ) {
            _console->print( " %3d: ", i );
            major_at( i )->print();
        }
    }
}


const char *JumpTable::compile_new_block( BlockClosureOop blk ) {

    // Called from the compile_block stub routine (see StubRoutines)
    BlockScavenge bs;
    ResourceMark  rm;
    NativeMethod  *nm = compile_block( blk );

    // return the entry point for the new NativeMethod.
    return nm->entryPoint();
}


NativeMethod *JumpTable::compile_block( BlockClosureOop closure ) {
    // compute the scope for noninlined block
    std::int32_t                   index;
    NativeMethod                   *parent = closure->jump_table_entry()->parent_nativeMethod( index );
    NonInlinedBlockScopeDescriptor *scope  = parent->noninlined_block_scope_at( index );

    // save it in case it gets flushed during allocation!
    JumpTableEntry *jumpEntry = closure->jump_table_entry();

    // compile the top-level block NativeMethod
    VM_OptimizeBlockMethod op( closure, scope );
    VMProcess::execute( &op );
    NativeMethod *nm = op.method();

    // patch the jump entry with the entry point of the compiled NativeMethod
    jumpEntry->set_destination( nm->entryPoint() );

    JumpTableEntry *b = nm->jump_table_entry();
    st_assert( jumpEntry == b, "jump table discrepancy" );
    return nm;
}


// XXX this verify function needs verification itself
void JumpTable::verify() {

    std::int32_t id   = 0;
    ResourceMark resourceMark;
    std::int32_t prev = -1;

    bool               *check = new_resource_array<bool>( length );
    for ( std::int32_t i      = 0; i < length; i++ )
        check[ i ] = false;

    std::int32_t j = 0;
    for ( id = _firstFree, j = 0; j < length - usedIDs; id = _entries[ id ], j++ ) {
        if ( id < 0 or id >= length ) {
            error( "JumpTable: invalid ID %ld in free list (#%ld)\n", id, j );
        }
        if ( check[ id ] ) {
            error( "JumpTable: loop with ID %ld in free list (#%ld)\n", id, j );
        }
        check[ id ] = true;
        prev = id;
    }

    if ( id not_eq length )
        error( "JumpTable: wrong free list length, last = %ld, prev = %ld\n", id, prev );

}


static const char nativeMethod_entry  = 0;
static const char block_closure_entry = 1;
static const char link_entry          = 2;
static const char unused_entry        = 3;


JumpTableEntry *JumpTableEntry::previous_stub() const {
    return (JumpTableEntry *) ( ( (const char *) this ) - size() );
}


JumpTableEntry *JumpTableEntry::next_stub() const {
    return (JumpTableEntry *) ( ( (const char *) this ) + size() );
}


void JumpTableEntry::fill_entry( const char instr, const char *dest, char state ) {
    *jump_inst_addr()   = instr;
    *destination_addr() = dest;
    *state_addr()       = state;
}


void JumpTableEntry::initialize_as_unused( std::int32_t index ) {
    fill_entry( halt_instruction, (char *) index, unused_entry );
}


void JumpTableEntry::initialize_as_link( const char *link ) {
    fill_entry( halt_instruction, link, link_entry );
}


void JumpTableEntry::initialize_NativeMethod_stub( char *dest ) {
    fill_entry( jump_instruction, dest - (std::int32_t) state_addr(), nativeMethod_entry );
}


void JumpTableEntry::initialize_block_closure_stub() {
    fill_entry( jump_instruction, const_cast<char *>( StubRoutines::compile_block_entry() - (std::int32_t) state_addr() ), block_closure_entry );
}


bool JumpTableEntry::is_NativeMethod_stub() const {
    return state() == nativeMethod_entry;
}


bool JumpTableEntry::is_block_closure_stub() const {
    return state() == block_closure_entry;
}


bool JumpTableEntry::is_unused() const {
    return state() == unused_entry;
}


bool JumpTableEntry::is_link() const {
    return state() == link_entry;
}


const char *JumpTableEntry::link() const {
    return *destination_addr();
}


const char **JumpTableEntry::destination_addr() const {
    return (const char **) ( ( (const char *) this ) + sizeof( char ) );
}


const char *JumpTableEntry::destination() const {
    return *destination_addr() + (std::int32_t) state_addr();
}


void JumpTableEntry::set_destination( const char *dest ) {
    *destination_addr() = dest - (std::int32_t) state_addr();
}


std::int32_t JumpTableEntry::next_free() const {
    st_assert( is_unused(), "must be a unused entry" );
    return (std::int32_t) *destination_addr();
}


NativeMethod *JumpTableEntry::method() const {
    st_assert( is_NativeMethod_stub(), "must be a nativeMethod_stub" );
    return Universe::code->findNativeMethod( destination() );
}


bool JumpTableEntry::block_has_nativeMethod() const {
    st_assert( is_block_closure_stub(), "must be a block_closure_stub" );
    return destination() not_eq StubRoutines::compile_block_entry();
}


NativeMethod *JumpTableEntry::block_nativeMethod() const {
    st_assert( is_block_closure_stub(), "must be a block_closure_stub" );
    if ( block_has_nativeMethod() ) {
        return Universe::code->findNativeMethod( destination() );
    } else {
        return nullptr;
    }
}


MethodOop JumpTableEntry::block_method() const {
    st_assert( is_block_closure_stub(), "must be a block_closure_stub" );
    if ( block_has_nativeMethod() ) {
        NativeMethod *nm = Universe::code->findNativeMethod( destination() );
        st_assert( nm not_eq nullptr, "NativeMethod must exists" );
        return nm->method();
    } else {
        std::int32_t   index;
        JumpTableEntry *pe = parent_entry( index );
        // find methodOop inside the NativeMethod:
        return pe->method()->noninlined_block_method_at( index );
    }
}


JumpTableEntry *JumpTableEntry::parent_entry( std::int32_t &index ) const {
    st_assert( is_block_closure_stub(), "must be a block_closure_stub" );
    // search back in the jump table to find the NativeMethod stub
    // responsible for this block closure stub.
    JumpTableEntry *result = (JumpTableEntry *) this;
    index = 0;
    do {
        index++;
        result = result->previous_stub();
    } while ( result->is_block_closure_stub() );
    return result;
}


NativeMethod *JumpTableEntry::parent_nativeMethod( std::int32_t &index ) const {
    return parent_entry( index )->method();
}


void JumpTableEntry::print() {
    if ( is_unused() ) {
        SPDLOG_INFO( "Unused {next = {}}", (std::int32_t) destination() );
        return;
    }
    if ( is_NativeMethod_stub() ) {
        _console->print( "NativeMethod stub " );
        Disassembler::decode( jump_inst_addr(), state_addr() );
        NativeMethod *nm = method();
        if ( nm ) {
            nm->_lookupKey.print();
        } else {
            SPDLOG_INFO( "{not pointing to NativeMethod}" );
        }
        return;
    }

    if ( is_block_closure_stub() ) {
        _console->print( "Block closure stub" );
        Disassembler::decode( jump_inst_addr(), state_addr() );
        NativeMethod *nm = block_nativeMethod();
        if ( nm ) {
            nm->_lookupKey.print();
        } else {
            SPDLOG_INFO( "{not compiled yet}" );
        }
        return;
    }

    if ( is_link() ) {
        SPDLOG_INFO( "Link for:" );
        JumpTable::jump_entry_for_at( link(), 0 )->print();
        return;
    }

    st_fatal( "unknown jump table entry" );
}


void JumpTableEntry::report_verify_error( const char *message ) {
    error( "JumpTableEntry 0x{0:x}: %s", this, message );
}


void JumpTableEntry::verify() {
    if ( is_unused() ) {
        // Nothing to check in an unused entry
        return;
    }

    if ( is_NativeMethod_stub() ) {
        // Check NativeMethod
        const char *addr = destination();
        if ( not Universe::code->contains( addr ) )
            report_verify_error( "NativeMethod not in zone" );
        if ( method()->entryPoint() not_eq addr )
            report_verify_error( "destination doesn't point to beginning of NativeMethod" );
        return;
    }

    if ( is_link() ) {
        // Verify the elements in the list {NativeMethod} {block_closure}+
        JumpTableEntry *head = JumpTable::jump_entry_for_at( link(), 0 );
        if ( not head->is_NativeMethod_stub() )
            report_verify_error( "must be NativeMethod stub" );
        head->verify();
        NativeMethod *nm = method();
        if ( not nm->has_noninlined_blocks() )
            report_verify_error( "NativeMethod must have noninlined blocks" );
        for ( std::size_t i = 1; i <= nm->number_of_noninlined_blocks(); i++ ) {
            JumpTableEntry *son = JumpTable::jump_entry_for_at( link(), i );
            if ( not son->is_block_closure_stub() )
                report_verify_error( "must be block closure stub" );
            son->verify();
        }
        return;
    }

    if ( is_block_closure_stub() ) {
        const char *addr = destination();
        if ( not Universe::code->contains( addr ) ) {
            if ( addr not_eq StubRoutines::compile_block_entry() )
                report_verify_error( "destination points neither into zone nor to compile stub" );
        } else {
            NativeMethod *nm = block_nativeMethod();
            if ( nm->entryPoint() not_eq addr )
                report_verify_error( "destination doesn't point to beginning of NativeMethod" );
        }
        return;
    }

    report_verify_error( "invalid state" );
}
