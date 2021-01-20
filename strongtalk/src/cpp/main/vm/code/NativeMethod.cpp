//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/utilities/disassembler.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/ResourceMark.hpp"


void NativeMethodFlags::clear() {
    st_assert( sizeof( NativeMethodFlags ) == sizeof( int ), "using more than one word for NativeMethodFlags" );
    *(int *) this = 0;
}


static std::size_t instruction_length;
static std::size_t location_length;
static std::size_t scope_length;
static std::size_t nof_noninlined_blocks;


NativeMethod *new_nativeMethod( Compiler *c ) {

    // This grossness is brought to you by the great way in which C++ handles non-standard allocation...
    instruction_length    = roundTo( c->code()->code_size(), oopSize );
    location_length       = roundTo( c->code()->reloc_size(), oopSize );
    scope_length          = roundTo( c->scopeDescRecorder()->size(), oopSize );
    nof_noninlined_blocks = c->number_of_noninlined_blocks();

    NativeMethod *nm = new NativeMethod( c );
    if ( c->is_method_compile() ) {
        Universe::code->addToCodeTable( nm );
    }

    return nm;
}


void *NativeMethod::operator new( std::size_t size ) {
    st_assert( sizeof( NativeMethod ) % oopSize == 0, "NativeMethod size must be multiple of a word" );
    int nativeMethod_size = sizeof( NativeMethod ) + instruction_length + location_length + scope_length + roundTo( ( nof_noninlined_blocks ) * sizeof( std::uint16_t ), oopSize );
    void *p = Universe::code->allocate( nativeMethod_size );
    if ( not p ) st_fatal( "out of Space in code cache" );
    return p;
}


void NativeMethod::initForTesting( std::size_t size, LookupKey *key ) {
    this->_lookupKey.initialize( key->klass(), key->selector_or_method() );

    _instructionsLength       = size - ( sizeof( NativeMethod ) );
    _locsLen                  = 0;
    _scopeLen                 = 0;
    _numberOfNoninlinedBlocks = 0;
    _mainId                   = Universe::code->jump_table()->allocate( 1 );
    _promotedId               = 0;
    _invocationCount          = 0;
    _uncommonTrapCounter      = 0;
    _numberOfLinks            = 0;
    _specialHandlerCallOffset = 0;
    _entryPointOffset         = 0;
    _verifiedEntryPointOffset = 0;
    _numberOfFloatTemporaries = 0;
    _floatSectionSize         = 0;
    _floatSectionStartOffset  = 0;

    _nativeMethodFlags.clear();
    _nativeMethodFlags.state = zombie;
}


NativeMethod::NativeMethod( Compiler *c ) :
        _lookupKey( c->key->klass(), c->key->selector_or_method() ) {

    LookupCache::verify();

    st_assert( instruction_length <= 10 * MaxNmInstrSize, "too many instructions" );

    // Initializing the chunks sizes
    _instructionsLength = instruction_length;
    _locsLen            = location_length;
    _scopeLen           = scope_length;

    _mainId     = c->main_jumpTable_id;
    _promotedId = c->promoted_jumpTable_id;

    _numberOfNoninlinedBlocks = nof_noninlined_blocks;

    _invocationCount     = 0;
    _uncommonTrapCounter = 0;
    _numberOfLinks       = 0;

    _specialHandlerCallOffset = theCompiler->special_handler_call_offset();
    _entryPointOffset         = theCompiler->entry_point_offset();
    _verifiedEntryPointOffset = theCompiler->verified_entry_point_offset();

    st_assert( _entryPointOffset % oopSize == 0, "entry point is not aligned" );
    st_assert( _verifiedEntryPointOffset % oopSize == 0, "verified entry point is not aligned" );
    st_assert( 0 <= _specialHandlerCallOffset and _specialHandlerCallOffset < instruction_length, "bad special handler call offset" );
    st_assert( 0 <= _entryPointOffset and _entryPointOffset < instruction_length, "bad entry point offset" );
    st_assert( 0 <= _verifiedEntryPointOffset and _verifiedEntryPointOffset < instruction_length, "bad verified entry point offset" );

    _numberOfFloatTemporaries = theCompiler->totalNofFloatTemporaries();
    _floatSectionSize         = theCompiler->float_section_size();
    _floatSectionStartOffset  = theCompiler->float_section_start_offset();

    _nativeMethodFlags.clear();
    _nativeMethodFlags.isUncommonRecompiled = c->is_uncommon_compile();

    _nativeMethodFlags.level   = c->level();
    _nativeMethodFlags.version = c->version();
    _nativeMethodFlags.isBlock = c->is_block_compile() ? 1 : 0;

    _nativeMethodFlags.state = alive;

    if ( UseNativeMethodAging )
        makeYoung();

//    st_assert( c->frameSize() >= 0, "frame size cannot be negative" );
//    frame_size = c->frameSize();

    debug();

    // Fill in instructions and locations.
    c->code()->copyTo( this );

    // Fill in scope information
    c->scopeDescRecorder()->copyTo( this );

    // Fill in noninlined block scope table
    c->copy_noninlined_block_info( this );

    flushICacheRange( instructionsStart(), instructionsEnd() );
    flushICache();

    check_store();

    // Set the jumptable entry for the NativeMethod
    if ( c->is_method_compile() ) {
        Universe::code->jump_table()->at( _mainId )->set_destination( entryPoint() );
    } else {
        if ( has_noninlined_blocks() ) {
            Universe::code->jump_table()->at( _promotedId )->set_destination( entryPoint() );
        }
    }
    if ( this == (NativeMethod *) catchThisOne )
        warning( "caught NativeMethod" );

    // turned off because they're very slow  -Urs 4/96
    LookupCache::verify();
    verify_expression_stacks();

}


NativeMethod *NativeMethod::parent() {
    if ( is_block() ) {
        int index = 0;
        return Universe::code->jump_table()->at( _mainId )->parent_nativeMethod( index );
    }
    return nullptr;
}


NativeMethod *NativeMethod::outermost() {
    NativeMethod *p = parent();
    if ( p == nullptr )
        return this;
    return p->outermost();
}


int NativeMethod::level() const {
    st_assert( _nativeMethodFlags.level >= 0 and _nativeMethodFlags.level <= MaxRecompilationLevels, "invalid level" );
    return _nativeMethodFlags.level;
}


JumpTableEntry *NativeMethod::jump_table_entry() const {
    return Universe::code->jump_table()->at( _mainId );
}


void NativeMethod::setVersion( int v ) {
    st_assert( v > 0 and v <= MaxVersions, "bad version" );
    _nativeMethodFlags.version = v;
}


ScopeDescriptor *NativeMethod::containingScopeDesc( const char *pc ) const {
    ProgramCounterDescriptor *pcd = containingProgramCounterDescriptor( pc );
    if ( not pcd )
        return nullptr;
    return pcd->containingDesc( this );
}


void NativeMethod::check_store() {
    // Make sure all oops in the compiled code are tenured
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::oop_type ) {
            Oop obj = *iter.oop_addr();
            if ( obj->is_mem() and obj->is_new() ) {
                st_fatal( "must be tenured Oop in compiled code" );
            }
        }
    }
}


void NativeMethod::fix_relocation_at_move( int delta ) {
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        if ( iter.is_position_dependent() ) {
            if ( iter.type() == RelocationInformation::RelocationType::internal_word_type ) {
                *iter.word_addr() -= delta;
            } else {
                *iter.word_addr() += delta;
            }
        }
    }
}


std::size_t NativeMethod::_allUncommonTrapCounter = 0;


MethodOop NativeMethod::method() const {
    ResourceMark resourceMark;
    return scopes()->root()->method();
}


KlassOop NativeMethod::receiver_klass() const {
    ResourceMark resourceMark;
    return scopes()->root()->selfKlass();
}


void NativeMethod::moveTo( void *p, std::size_t size ) {
#ifdef NOT_IMPLEMENTED
    NativeMethod* to = (NativeMethod*)p;
    if (this == to) return;
    if (PrintCodeCompaction) {
      printf("*moving NativeMethod %#lx (", this);
      key.print();
      printf(") to %#lx\n", to);
      fflush(stdout);
    }

    assert(iabs((char*)to - (char*)this) >= sizeof(NCodeBase),
       "nativeMethods overlap too much");
    assert(sizeof(NCodeBase) % oopSize == 0, "should be word-aligned");
    copy_oops((Oop*)this, (Oop*)to, sizeof(NCodeBase) / oopSize);
            // init to's vtable
    int delta = (char*) to - (char*) this;

    for (RelocationInformation* q = locs(), *pend = locsEnd(); q < pend; q++) {
      bool_t needShift;		// speed optimization - q->shift() is slow
      if (q->isIC()) {
        InlineCache* sd = q->asIC(this);
        sd->shift(delta, this);
        needShift = true;
      } else {
        needShift = true;
      }
      if (needShift) {
        q->shift(this, delta);
      }
    }

    assert(size % oopSize == 0, "not a multiple of oopSize");
    copy_oops_overlapping((Oop*) this, (Oop*) to, size / oopSize);
    flushICacheRange(to->insts(), to->instsEnd());
#endif
}


void NativeMethod::clear_inline_caches() {
    // Iterate over all inline caches and flush them
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::ic_type ) {
            iter.ic()->clear();
        }
    }
}


void NativeMethod::cleanup_inline_caches() {
    // Ignore zombies
    if ( isZombie() )
        return;

    // Iterate over all inline caches and flush them
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::ic_type ) {
            iter.ic()->cleanup();
        }
    }
}


void NativeMethod::makeOld() {
    LOG_EVENT1( "marking NativeMethod %#x as old", this );
    _nativeMethodFlags.isYoung = 0;
}


void NativeMethod::forwardLinkedSends( NativeMethod *to ) {
    // the to NativeMethod is about to replace the receiver; replace receiver in all inline caches
    Unimplemented();
}


void NativeMethod::unlink() {
    LOG_EVENT1( "unlinking NativeMethod %#lx", this );

    if ( is_method() ) {
        // Remove from LookupCache.
        LookupCache::flush( &_lookupKey );
        // Remove the NativeMethod from the code table.if it's still there (another NativeMethod with the same key may have been added in the meantime).
        if ( Universe::code->_methodTable->is_present( this ) )
            Universe::code->_methodTable->remove( this );
    }

    // Now clear all inline caches filled with this NativeMethod
    // (*not* done by clear_inline_caches() -- that clears the inline caches *in* this NativeMethod).
    // Right now, can't do this -- don't know who is calling this NativeMethod.
}


void NativeMethod::makeZombie( bool_t clearInlineCaches ) {
    // mark this NativeMethod as zombie (it is almost dead and can be flushed as soon as it is no longer on the stack)
    if ( isZombie() )
        return;

    if ( isResurrected() ) {
        // was a resurrected zombie, so just reset its state
        _nativeMethodFlags.state = zombie;
        return;
    }

    // overwrite call to recompiler by call to zombie handler
    LOG_EVENT2( "%s NativeMethod 0x%x becomes zombie", ( is_method() ? "normal" : "block" ), this );
    NativeCall *call = nativeCall_at( specialHandlerCall() );

    // Fix this:
    //   CODE NEEDED UNTIL Lars IMPLEMENTS CORRECT DEOPTIMIZATION FOR BLOCK NMETHODS
    if ( is_block() and not MakeBlockMethodZombies )
        return;

    if ( is_method() ) {
        call->set_destination( StubRoutines::zombie_NativeMethod_entry() );
    } else {
        call->set_destination( StubRoutines::zombie_block_NativeMethod_entry() );
    }

    // WARNING: INTEL SPECIFIC CODE PROVIDED BY ROBERT
    // at verified entry point: overwrite "push ebp, mov ebp esp" instructions
    // belonging to activation frame construction with jump to zombie handler
    // call (note that this code can be savely overwritten, since there's no
    // relocation info nor oops associated with it).

    const char *enter = "\x55\x8b\xec";
    char       *p     = verifiedEntryPoint();
    guarantee( p[ 0 ] == enter[ 0 ] and p[ 1 ] == enter[ 1 ] and p[ 2 ] == enter[ 2 ], "not \"push ebp, mov ebp esp\" - check this" );

    // overwrite with "nop, jmp specialHandlerCall" (nop first so it can be replaced by int3 for debugging)
    const char nop    = '\x90';
    const char jmp    = '\xeb'; // std::int16_t jump with 8bit signed offset
    int        offset = specialHandlerCall() - &p[ 3 ];
    guarantee( -128 <= offset and offset < 128, "offset too big for std::int16_t jump" );
    p[ 0 ] = nop;
    p[ 1 ] = jmp;
    p[ 2 ] = char( offset );

    if ( TraceZombieCreation ) {
        _console->print_cr( "%s NativeMethod 0x%x becomes zombie", ( is_method() ? "normal" : "block" ), this );
        if ( WizardMode ) {
            _console->print_cr( "entry code sequence:" );
            char *beg = (char *) min( int( specialHandlerCall() ), int( entryPoint() ), int( verifiedEntryPoint() ) );
            char *end = (char *) max( int( specialHandlerCall() ), int( entryPoint() ), int( verifiedEntryPoint() ) );
            Disassembler::decode( beg, end + 10 );
        }
    }

    // Update flags
    _nativeMethodFlags.state            = zombie;
    _nativeMethodFlags.isToBeRecompiled = 0;
    st_assert( isZombie(), "just checking" );

    // Make sure the entry is gone from the lookup cache & inline caches
    LookupCache::flush( &_lookupKey );
    if ( clearInlineCaches )
        clear_inline_caches();

    // Remove from NativeMethod tables
    unlink();
}


bool_t NativeMethod::has_noninlined_blocks() const {
    return number_of_noninlined_blocks() > 0;
}


int NativeMethod::number_of_noninlined_blocks() const {
    return _numberOfNoninlinedBlocks;
}


MethodOop NativeMethod::noninlined_block_method_at( int noninlined_block_index ) const {
    ResourceMark resourceMark;
    return noninlined_block_scope_at( noninlined_block_index )->method();
}


void NativeMethod::validate_noninlined_block_scope_index( int index ) const {
    st_assert( index > 0, "noninlined_block_index must be positive" );
    st_assert( index <= number_of_noninlined_blocks(), "noninlined_block_index must be within boundary" );
}


NonInlinedBlockScopeDescriptor *NativeMethod::noninlined_block_scope_at( int noninlined_block_index ) const {
    validate_noninlined_block_scope_index( noninlined_block_index );
    int offset = noninlined_block_offsets()[ noninlined_block_index - 1 ];
    return scopes()->noninlined_block_scope_at( offset );
}


void NativeMethod::noninlined_block_at_put( int noninlined_block_index, int offset ) const {
    validate_noninlined_block_scope_index( noninlined_block_index );
    noninlined_block_offsets()[ noninlined_block_index - 1 ] = offset;
}


JumpTableEntry *NativeMethod::noninlined_block_jumpEntry_at( int noninlined_block_index ) const {
    validate_noninlined_block_scope_index( noninlined_block_index );
    JumpTableID id = is_block() ? _promotedId : _mainId;
    return Universe::code->jump_table()->at( id.sub( noninlined_block_index ) );
}


void NativeMethod::flush() {
    // completely deallocate this method
    EventMarker em( "flushing NativeMethod %#lx %s", this, "" );
    if ( PrintMethodFlushing ) {
        _console->print_cr( "*flushing NativeMethod %#lx", this );
    }

    if ( isZombie() ) {
        clear_inline_caches();    // make sure they're cleared (may not have been done by makeZombie)
    } else {
        makeZombie( true );
    }
    unlink();

    Universe::code->free( this );
}


bool_t NativeMethod::depends_on_invalid_klass() {
    // Check receiver class
    if ( receiver_klass()->is_invalid() )
        return true;

    // Check dependents
    NativeMethodScopes *ns = scopes();
    for ( std::size_t i = ns->dependent_length() - 1; i >= 0; i-- ) {
        if ( ns->dependent_at( i )->is_invalid() )
            return true;
    }

    // We do not depend on an invalid class
    return false;
}


void NativeMethod::add_family( GrowableArray<NativeMethod *> *result ) {
    // Add myself
    result->append( this );

    // Find the major for all my sub JumpTable entries
    int major = is_method() ? _mainId.major() : _promotedId.major();

    // Add all filled JumpTable entries to the family
    for ( std::size_t minor = 1; minor <= number_of_noninlined_blocks(); minor++ ) {
        JumpTableEntry *entry = Universe::code->jump_table()->at( JumpTableID( major, minor ) );
        NativeMethod   *bm    = entry->block_nativeMethod();
        if ( bm )
            bm->add_family( result );
    }
}


GrowableArray<NativeMethod *> *NativeMethod::invalidation_family() {
    GrowableArray<NativeMethod *> *result = new GrowableArray<NativeMethod *>( 10 );
    add_family( result ); // Call the recusive function
    return result;
}


ProgramCounterDescriptor *NativeMethod::containingProgramCounterDescriptorOrNULL( const char *pc, ProgramCounterDescriptor *stream ) const {

    // returns ProgramCounterDescriptor that is closest one before or == to pc, or nullptr if no stored programCounterDescriptor exists
    // called a lot, so watch out for performance bugs

    st_assert( contains( pc ), "NativeMethod must contain pc into frame" );
    int offset = pc - instructionsStart();
    ProgramCounterDescriptor *start = stream ? stream : pcs();
    ProgramCounterDescriptor *end   = pcsEnd() - 1;

    // return if only one programCounterDescriptor is present.
    if ( start == end )
        return start;

    st_assert( start <= end, "no ProgramCounterDescriptors to search" );

    // binary search to find approximate location
    ProgramCounterDescriptor *middle = nullptr;

    int l = 0;
    int h = end - start;

    do {
        // avoid pointer arithmetic -- gcc uses a division for ProgramCounterDescriptor* - ProgramCounterDescriptor*
        int m = l + ( h - l ) / 2;
        _console->print_cr( "l [0x%x], h [0x%x], m [0x%x], middle [%#lx]", l, h, m, middle );

        middle = &start[ m ];
        if ( middle->_pc < offset ) {
            l = m + 1;
        } else {
            h = m - 1;
        }

    } while ( middle->_pc not_eq offset and l < h );

    // may not have found exact offset, so search for closest match
    while ( middle->_pc <= offset and middle < end )
        middle++;
    while ( middle->_pc > offset and middle > start )
        middle--;

    st_assert( start <= middle and middle <= end, "should have found a ProgramCounterDescriptor" );
    ProgramCounterDescriptor *d       = stream ? stream : pcs();
    ProgramCounterDescriptor *closest = d;
    for ( ; d <= end; d++ ) {
        if ( d->_pc <= offset and ( closest == nullptr or closest->_pc <= d->_pc ) ) {
            closest = d;
        }
    }
    st_assert( closest == middle, "found different ProgramCounterDescriptor" );

    if ( middle->_pc > offset ) {
        st_assert( middle == start, "should be the first ProgramCounterDescriptor" ); // in prologue; caller has to deal with this
        return nullptr;
    }

    return middle;
}


// Create a static ProgramCounterDescriptor for prologue ProgramCounterDescriptor's
static ProgramCounterDescriptor prologue_pd( 0, 0, PrologueByteCodeIndex );


ProgramCounterDescriptor *NativeMethod::containingProgramCounterDescriptor( const char *pc, ProgramCounterDescriptor *start ) const {

    // returns ProgramCounterDescriptor that is closest one before or == to pc
    ProgramCounterDescriptor *p = containingProgramCounterDescriptorOrNULL( pc, start );

    // in prologue; there is no ProgramCounterDescriptor stored in the NativeMethod (to save Space), so we have to create one
    return p ? p : &prologue_pd;
}


int NativeMethod::estimatedInvocationCount() const {
    Unimplemented();
    return 0;
}


static std::size_t cmp_addrs( const void *p1, const void *p2 ) {
    char **r1 = (char **) p1;
    char **r2 = (char **) p2;
    return *r1 - *r2;
}


int NativeMethod::ncallers() const {
    return number_of_links();
}


// Memory operations: return true if need to invalidate cache

void NativeMethod::relocate() {
    _lookupKey.relocate();
    scopes()->relocate();
    OopNativeCode::relocate();
}


bool_t NativeMethod::switch_pointers( Oop from, Oop to, GrowableArray<NativeMethod *> *nativeMethods_to_invalidate ) {
    _lookupKey.switch_pointers( from, to );
    scopes()->switch_pointers( from, to, nativeMethods_to_invalidate );
    check_store();

    return OopNativeCode::switch_pointers( from, to, nativeMethods_to_invalidate );
}


void NativeMethod::oops_do( void f( Oop * ) ) {

    // LookupKey
    _lookupKey.oops_do( f );

    // Compiled code
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::oop_type ) {
            f( iter.oop_addr() );
        }
    }

    // Debugging information
    scopes()->oops_do( f );
}


void NativeMethod::verify() {

    ResourceMark resourceMark;

    OopNativeCode::verify2( "NativeMethod" );

    // Make sure all entry points are aligned
    // The interpreter counts on it for InterpreterPICs

    if ( not Oop( instructionsStart() )->is_smi() )
        error( "NativeMethod at %#lx has unaligned instruction start", this );

    if ( not Oop( entryPoint() )->is_smi() )
        error( "NativeMethod at %#lx has unaligned entryPoint", this );

    if ( not Oop( verifiedEntryPoint() )->is_smi() )
        error( "NativeMethod at %#lx has unaligned verifiedEntryPoint", this );

    if ( not Universe::code->contains( this ) )
        error( "NativeMethod at %#lx not in zone", this );

    scopes()->verify();

    for ( ProgramCounterDescriptor *p = pcs(); p < pcsEnd(); p++ ) {
        if ( not p->verify( this ) ) {
            _console->print_cr( "\t\tin NativeMethod at %#lx (pcs)", this );
        }
    }

    if ( findNativeMethod( (char *) instructionsEnd() - oopSize ) not_eq this ) {
        error( "findNativeMethod did not find this NativeMethod (%#lx)", this );
    }

    verify_expression_stacks();
}


void NativeMethod::verify_expression_stacks_at( const char *pc ) {

    ProgramCounterDescriptor *pd = containingProgramCounterDescriptor( pc );
    if ( not pd ) st_fatal( "ProgramCounterDescriptor not found" );

    ScopeDescriptor *sd = scopes()->at( pd->_scope, pc );
    int byteCodeIndex = pd->_byteCodeIndex;
    while ( sd ) {
        sd->verify_expression_stack( byteCodeIndex );
        ScopeDescriptor *next = sd->sender();
        if ( next )
            byteCodeIndex = sd->senderByteCodeIndex();

        sd = next;
    }

}


void NativeMethod::verify_expression_stacks() {
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        switch ( iter.type() ) {
            case RelocationInformation::RelocationType::ic_type:
                verify_expression_stacks_at( iter.ic()->begin_addr() );
                break;
            case RelocationInformation::RelocationType::primitive_type:
                if ( iter.primIC()->primitive()->can_walk_stack() ) {
                    verify_expression_stacks_at( iter.primIC()->begin_addr() );
                }
                break;
        }
    }
}


void NativeMethod::CompiledICs_do( void f( CompiledInlineCache * ) ) {
    RelocationInformationIterator iter( this );
    while ( iter.next() )
        if ( iter.type() == RelocationInformation::RelocationType::ic_type )
            f( iter.ic() );
}


void NativeMethod::PrimitiveICs_do( void f( PrimitiveInlineCache * ) ) {
    RelocationInformationIterator iter( this );
    while ( iter.next() )
        if ( iter.type() == RelocationInformation::RelocationType::primitive_type )
            f( iter.primIC() );
}


// Printing operations

void NativeMethod::print() {
    ResourceMark resourceMark;
    printIndent();
    _console->print( "NativeMethod [%#lx] for method [%#lx]", this, method() );
    _lookupKey.print();
    _console->print( " { " );

    if ( isYoung() )
        _console->print( "YOUNG " );
    if ( version() )
        _console->print( "v0x%08x ", version() );
    if ( level() )
        _console->print( "l%d ", level() );
    if ( isZombie() )
        _console->print( "zombie " );
    if ( isToBeRecompiled() )
        _console->print( "TBR " );
    if ( isUncommonRecompiled() )
        _console->print( "UNCOMMON " );
    _console->print_cr( "}:" );
    Indent++;

    printIndent();
    _console->print_cr( "instructions (%ld bytes): [%#lx..%#lx]", size(), instructionsStart(), instructionsEnd() );
    printIndent();
    _console->print_cr( "NativeMethod [%#lx]", this );
    // don't print code/locs/pcs by default -- too much output   -Urs 1/95
//    printCode();
    scopes()->print();
//    printLocs();
//    printPcs();
    Indent--;
}


void NativeMethod::printCode() {
    ResourceMark m;
    Disassembler::decode( this );
}


void NativeMethod::printLocs() {
    ResourceMark m;    // in case methods get printed via the debugger
    printIndent();
    _console->print_cr( "locations:" );
    Indent++;
    RelocationInformationIterator iter( this );
    int                           last_offset = 0;

    for ( RelocationInformation *l = locs(); l < locsEnd(); l++ ) {
        iter.next();
        last_offset = l->print( this, last_offset );
        if ( iter.type() == RelocationInformation::RelocationType::uncommon_type and iter.wasUncommonTrapExecuted() )
            _console->print( " (taken)" );
        _console->cr();
    }
    Indent--;
}


void NativeMethod::printPcs() {
    ResourceMark m;    // in case methods get printed via debugger
    printIndent();
    lprintf( "pc-bytecode offsets:\n" );
    Indent++;
    for ( ProgramCounterDescriptor *p = pcs(); p < pcsEnd(); p++ )
        p->print( this );
    Indent--;
}


void NativeMethod::print_value_on( ConsoleOutputStream *stream ) {
    stream->print( "NativeMethod" );
    if ( WizardMode )
        stream->print( " (0x%lx)", this );
    stream->print( ":" );
    method()->print_value_for( receiver_klass(), stream );
}


static ScopeDescriptor *print_scope_node( NativeMethodScopes *scopes, ScopeDescriptor *sd, int level, ConsoleOutputStream *stream, bool_t with_debug_info ) {
    // indent
    stream->fill_to( 2 + level * 2 );

    // print scope
    sd->print_value_on( stream );
    stream->cr();
    if ( with_debug_info )
        sd->print( 4 + level * 2, UseNewBackend );

    // print sons
    ScopeDescriptor *son = scopes->getNext( sd );
    while ( son and son->sender_scope_offset() == sd->offset() ) {
        son = print_scope_node( scopes, son, level + 1, stream, with_debug_info );
    }
    return son;
}


void NativeMethod::print_inlining( ConsoleOutputStream *stream, bool_t with_debug_info ) {
    // Takes advantage of the fact that the scope tree is stored in a depth first traversal order.
    ResourceMark resourceMark;
    if ( stream == nullptr )
        stream = _console;
    stream->print_cr( "NativeMethod inlining structure" );
    ScopeDescriptor *result = print_scope_node( scopes(), scopes()->root(), 0, stream, with_debug_info );
    if ( result not_eq nullptr )
        warning( "print_inlining returned prematurely" );
}


NativeMethod *nativeMethodContaining( const char *pc, char *likelyEntryPoint ) {
    st_assert( Universe::code->contains( pc ), "should contain address" );
    if ( likelyEntryPoint and Universe::code->contains( likelyEntryPoint ) ) {
        NativeMethod *result = nativeMethod_from_insts( likelyEntryPoint );
        if ( result->contains( pc ) )
            return result;
    }
    return findNativeMethod( pc );
}


NativeMethod *findNativeMethod( const void *start ) {
    NativeMethod *m = Universe::code->findNativeMethod( start );
    st_assert( m->encompasses( start ), "returned wrong NativeMethod" );
    return m;
}


NativeMethod *findNativeMethod_maybe( void *start ) {
    NativeMethod *m = Universe::code->findNativeMethod_maybe( start );
    st_assert( not m or m->encompasses( start ), "returned wrong NativeMethod" );
    return m;
}


bool_t includes( const void *p, const void *from, void *to ) {
    return from <= p and p < to;
}


bool_t NativeMethod::encompasses( const void *p ) const {
    return includes( p, (const void *) this, pcsEnd() );
}


#ifdef DEBUG_LATER
ProgramCounterDescriptor* NativeMethod::correspondingPC(ScopeDescriptor* sd, int byteCodeIndex) {
  // find the starting PC of this scope
  assert(scopes()->includes(sd), "scope not in this NativeMethod");
  int scope = scopes()->offsetTo(sd);
  for (ProgramCounterDescriptor* p = pcs(), *end = pcsEnd(); p < end; p ++) {
    if (p->scope == scope and p->byteCode == byteCodeIndex) break;
  }
  if (p < end ) {
    return p;
  } else {
    // no PC corresponding to this scope
    return nullptr;
  }
}
#endif


CompiledInlineCache *NativeMethod::IC_at( const char *p ) const {
    RelocationInformationIterator iter( this );
    while ( iter.next() )
        if ( iter.type() == RelocationInformation::RelocationType::ic_type )
            if ( iter.ic()->begin_addr() == p )
                return iter.ic();
    return nullptr;
}


PrimitiveInlineCache *NativeMethod::primitiveIC_at( const char *p ) const {
    RelocationInformationIterator iter( this );
    while ( iter.next() )
        if ( iter.type() == RelocationInformation::RelocationType::primitive_type )
            if ( iter.primIC()->begin_addr() == p )
                return iter.primIC();
    return nullptr;
}


Oop *NativeMethod::embeddedOop_at( const char *p ) const {
    RelocationInformationIterator iter( this );
    while ( iter.next() )
        if ( iter.type() == RelocationInformation::RelocationType::oop_type )
            if ( iter.oop_addr() == (Oop *) p )
                return iter.oop_addr();
    return nullptr;
}


bool_t NativeMethod::in_delta_code_at( const char *pc ) const {
    ProgramCounterDescriptor *pd = containingProgramCounterDescriptorOrNULL( pc );
    if ( pd == nullptr )
        return false;
    return not( pd->_byteCodeIndex == PrologueByteCodeIndex or pd->_byteCodeIndex == EpilogueByteCodeIndex );
}


// Support for preemption:

void NativeMethod::overwrite_for_trapping( nativeMethod_patch *data ) {
    RelocationInformationIterator iter( this );
    while ( iter.next() ) {
        switch ( iter.type() ) {
            case RelocationInformation::RelocationType::ic_type:
                break;
            case RelocationInformation::RelocationType::primitive_type:
                break;
            case RelocationInformation::RelocationType::runtime_call_type:
                break;
            case RelocationInformation::RelocationType::uncommon_type:
                break;
        }
    }
}


void NativeMethod::restore_from_patch( nativeMethod_patch *data ) {
    Unimplemented();
}


void NativeMethod::print_inlining_database() {
    print_inlining_database_on( _console );
}


void NativeMethod::print_inlining_database_on( ConsoleOutputStream *stream ) {
    // WARNING: this method is for debugging only -- it's not used to actually file out the DB
    ResourceMark rm;
    RecompilationScope *root = NonDummyRecompilationScope::constructRScopes( this, false );
    GrowableArray<ProgramCounterDescriptor *> *uncommon = uncommonBranchList();
    root->print_inlining_database_on( stream, uncommon );
}


static std::size_t compare_pcDescs( ProgramCounterDescriptor **a, ProgramCounterDescriptor **b ) {
    // to sort by ascending scope and ascending byteCodeIndex
    int diff = ( *a )->_scope - ( *b )->_scope;
    return diff ? diff : ( *a )->_byteCodeIndex - ( *b )->_byteCodeIndex;
}


GrowableArray<ProgramCounterDescriptor *> *NativeMethod::uncommonBranchList() {
    // return a list of *all* uncommon branches (taken or not) of nm, sorted by scope and byteCodeIndex
    // (for inlining DB)
    GrowableArray<ProgramCounterDescriptor *> *uncommon = new GrowableArray<ProgramCounterDescriptor *>( 20 );
    RelocationInformationIterator             iter( this );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::uncommon_type ) {
            uncommon->append( containingProgramCounterDescriptor( (const char *) iter.word_addr() ) );
        }
    }

    uncommon->sort( &compare_pcDescs );
    return uncommon;
}


void NativeMethod::decay_invocation_count( double decay_factor ) {
    double new_count = (double) invocation_count() / decay_factor;
    set_invocation_count( (int) new_count );
}


// Perform a sweeper task
void NativeMethod::sweeper_step( double decay_factor ) {
    // Ignore zombies
    if ( isZombie() )
        return;
    decay_invocation_count( decay_factor );
    _uncommonTrapCounter = int( _uncommonTrapCounter / decay_factor );
    cleanup_inline_caches();
    incrementAge();
}


bool_t NativeMethod::isYoung() {
    if ( not UseNativeMethodAging )
        return false;
    if ( not _nativeMethodFlags.isYoung )
        return false;
    // flags.isYoung == 1, but maybe it has become old in the meantime
    if ( age() >= NativeMethodAgeLimit or invocation_count() >= InvocationCounterLimit ) {
        makeOld();
    }
    return _nativeMethodFlags.isYoung;
}


void nativeMethod_init() {
    // make sure you didn't forget to adjust the filler fields
    st_assert( sizeof( NativeMethodFlags ) <= 4, "NativeMethodFlags occupies more than a word" );
}
