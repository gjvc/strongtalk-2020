
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/assembler/Assembler.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/runtime/flags.hpp"


#define disp_at( L )                Displacement( long_at( (L).pos() ) )
#define disp_at_put( L, disp )      long_at_put( (L).pos(), (disp).data() )
#define emit_disp( L, type, info ) \
    { \
        Displacement disp( (L), (type), (info) ); \
        L.link_to( offset() );  \
        emit_long( int(disp.data()) ); \
    }


Assembler::Assembler( CodeBuffer *code ) {
    _code       = code;
    _code_begin = code->code_begin();
    _code_limit = code->code_limit();
    _code_pos   = code->code_end();
}


void Assembler::finalize() {
    if ( _unbound_label.is_unbound() ) {
        bind_to( _unbound_label, _binding_pos );
    }
}


void Assembler::emit_byte( int x ) {
    st_assert( isByte( x ), "not a byte" );
    *(std::uint8_t *) _code_pos = (std::uint8_t) x;
    _code_pos += sizeof( std::uint8_t );
    code()->set_code_end( _code_pos );
}


void Assembler::emit_long( int x ) {
    *(int *) _code_pos = x;
    _code_pos += sizeof( int );
    code()->set_code_end( _code_pos );
}


void Assembler::emit_data( int data, RelocationInformation::RelocationType rtype ) {
    if ( rtype not_eq RelocationInformation::RelocationType::none )
        code()->relocate( _code_pos, rtype );
    emit_long( data );
}


void Assembler::emit_arith_b( int op1, int op2, const Register &dst, int imm8 ) {
    guarantee( dst.hasByteRegister(), "must have byte register" );
    st_assert( isByte( op1 ) and isByte( op2 ), "wrong opcode" );
    st_assert( isByte( imm8 ), "not a byte" );
    st_assert( ( op1 & 0x01 ) == 0, "should be 8bit operation" );
    emit_byte( op1 );
    emit_byte( op2 | dst.number() );
    emit_byte( imm8 );
}


void Assembler::emit_arith( int op1, int op2, const Register &dst, int imm32 ) {
    st_assert( isByte( op1 ) and isByte( op2 ), "wrong opcode" );
    st_assert( ( op1 & 0x01 ) == 1, "should be 32bit operation" );
    st_assert( ( op1 & 0x02 ) == 0, "sign-extension bit should not be set" );
    if ( is8bit( imm32 ) ) {
        emit_byte( op1 | 0x02 ); // set sign bit
        emit_byte( op2 | dst.number() );
        emit_byte( imm32 & 0xFF );
    } else {
        emit_byte( op1 );
        emit_byte( op2 | dst.number() );
        emit_long( imm32 );
    }
}


void Assembler::emit_arith( int op1, int op2, const Register &dst, Oop obj ) {
    st_assert( isByte( op1 ) and isByte( op2 ), "wrong opcode" );
    st_assert( ( op1 & 0x01 ) == 1, "should be 32bit operation" );
    st_assert( ( op1 & 0x02 ) == 0, "sign-extension bit should not be set" );
    emit_byte( op1 );
    emit_byte( op2 | dst.number() );
    emit_data( (int) obj, RelocationInformation::RelocationType::oop_type );
}


void Assembler::emit_arith( int op1, int op2, const Register &dst, const Register &src ) {
    st_assert( isByte( op1 ) and isByte( op2 ), "wrong opcode" );
    emit_byte( op1 );
    emit_byte( op2 | dst.number() << 3 | src.number() );
}


void Assembler::emit_operand( const Register &reg, const Register &base, const Register &index, Address::ScaleFactor scale, int disp, RelocationInformation::RelocationType rtype ) {
    if ( base.isValid() ) {
        if ( index.isValid() ) {
            st_assert( scale not_eq Address::ScaleFactor::no_scale, "inconsistent address" );
            // [base + index*scale + disp]
            if ( disp == 0 and rtype == RelocationInformation::RelocationType::none ) {
                // [base + index*scale]
                // [00 reg 100][ss index base]
                st_assert( index not_eq esp and base not_eq ebp, "illegal addressing mode" );
                emit_byte( 0x04 | reg.number() << 3 );
                emit_byte( static_cast<int>(scale) << 6 | index.number() << 3 | base.number() );
            } else if ( is8bit( disp ) and rtype == RelocationInformation::RelocationType::none ) {
                // [base + index*scale + imm8]
                // [01 reg 100][ss index base] imm8
                st_assert( index not_eq esp, "illegal addressing mode" );
                emit_byte( 0x44 | reg.number() << 3 );
                emit_byte( static_cast<int>(scale) << 6 | index.number() << 3 | base.number() );
                emit_byte( disp & 0xFF );
            } else {
                // [base + index*scale + imm32]
                // [10 reg 100][ss index base] imm32
                st_assert( index not_eq esp, "illegal addressing mode" );
                emit_byte( 0x84 | reg.number() << 3 );
                emit_byte( static_cast<int>(scale) << 6 | index.number() << 3 | base.number() );
                emit_data( disp, rtype );
            }
        } else if ( base == esp ) {
            // [esp + disp]
            if ( disp == 0 and rtype == RelocationInformation::RelocationType::none ) {
                // [esp]
                // [00 reg 100][00 100 100]
                emit_byte( 0x04 | reg.number() << 3 );
                emit_byte( 0x24 );
            } else if ( is8bit( disp ) and rtype == RelocationInformation::RelocationType::none ) {
                // [esp + imm8]
                // [01 reg 100][00 100 100] imm8
                emit_byte( 0x44 | reg.number() << 3 );
                emit_byte( 0x24 );
                emit_byte( disp & 0xFF );
            } else {
                // [esp + imm32]
                // [10 reg 100][00 100 100] imm32
                emit_byte( 0x84 | reg.number() << 3 );
                emit_byte( 0x24 );
                emit_data( disp, rtype );
            }
        } else {
            // [base + disp]
            st_assert( base not_eq esp, "illegal addressing mode" );
            if ( disp == 0 and rtype == RelocationInformation::RelocationType::none and base not_eq ebp ) {
                // [base]
                // [00 reg base]
                st_assert( base not_eq ebp, "illegal addressing mode" );
                emit_byte( 0x00 | reg.number() << 3 | base.number() );
            } else if ( is8bit( disp ) and rtype == RelocationInformation::RelocationType::none ) {
                // [base + imm8]
                // [01 reg base] imm8
                emit_byte( 0x40 | reg.number() << 3 | base.number() );
                emit_byte( disp & 0xFF );
            } else {
                // [base + imm32]
                // [10 reg base] imm32
                emit_byte( 0x80 | reg.number() << 3 | base.number() );
                emit_data( disp, rtype );
            }
        }
    } else {
        if ( index.isValid() ) {
            st_assert( scale not_eq Address::ScaleFactor::no_scale, "inconsistent address" );
            // [index*scale + disp]
            // [00 reg 100][ss index 101] imm32
            st_assert( index not_eq esp, "illegal addressing mode" );
            emit_byte( 0x04 | reg.number() << 3 );
            emit_byte( static_cast<int>(scale) << 6 | index.number() << 3 | 0x05 );
            emit_data( disp, rtype );
        } else {
            // [disp]
            // [00 reg 101] imm32
            emit_byte( 0x05 | reg.number() << 3 );
            emit_data( disp, rtype );
        }
    }
}


void Assembler::emit_operand( const Register &r, const Address &a ) {
    emit_operand( r, a._base, a._index, a._scale, a._displacement, a._relocationType );
}


void Assembler::emit_farith( int b1, int b2, int i ) {
    st_assert( isByte( b1 ) and isByte( b2 ), "wrong opcode" );
    st_assert( 0 <= i and i < 8, "illegal stack offset" );
    emit_byte( b1 );
    emit_byte( b2 + i );
}


void Assembler::pushad() {
    emit_byte( 0x60 );
}


void Assembler::popad() {
    emit_byte( 0x61 );
}


void Assembler::pushl( int imm32 ) {
    emit_byte( 0x68 );
    emit_long( imm32 );
}


void Assembler::pushl( Oop obj ) {
    emit_byte( 0x68 );
    emit_data( (int) obj, RelocationInformation::RelocationType::oop_type );
}


void Assembler::pushl( const Register &src ) {
    emit_byte( 0x50 | src.number() );
}


void Assembler::pushl( const Address &src ) {
    emit_byte( 0xFF );
    emit_operand( esi, src );
}


void Assembler::popl( const Register &dst ) {
    emit_byte( 0x58 | dst.number() );
}


void Assembler::popl( const Address &dst ) {
    emit_byte( 0x8F );
    emit_operand( eax, dst );
}


void Assembler::movb( const Register &dst, const Address &src ) {
    guarantee( dst.hasByteRegister(), "must have byte register" );
    emit_byte( 0x8A );
    emit_operand( dst, src );
}


void Assembler::movb( const Address &dst, int imm8 ) {
    emit_byte( 0xC6 );
    emit_operand( eax, dst );
    emit_byte( imm8 );
}


void Assembler::movb( const Address &dst, const Register &src ) {
    guarantee( src.hasByteRegister(), "must have byte register" );
    emit_byte( 0x88 );
    emit_operand( src, dst );
}


void Assembler::movw( const Register &dst, const Address &src ) {
    emit_byte( 0x66 );
    emit_byte( 0x8B );
    emit_operand( dst, src );
}


void Assembler::movw( const Address &dst, const Register &src ) {
    emit_byte( 0x66 );
    emit_byte( 0x89 );
    emit_operand( src, dst );
}


void Assembler::movl( const Register &dst, int imm32 ) {
    emit_byte( 0xB8 | dst.number() );
    emit_long( imm32 );
}


void Assembler::movl( const Register &dst, Oop obj ) {
    emit_byte( 0xB8 | dst.number() );
    emit_data( (int) obj, RelocationInformation::RelocationType::oop_type );
}


void Assembler::movl( const Register &dst, const Register &src ) {
    emit_byte( 0x8B );
    emit_byte( 0xC0 | ( dst.number() << 3 ) | src.number() );
}


void Assembler::movl( const Register &dst, const Address &src ) {
    emit_byte( 0x8B );
    emit_operand( dst, src );
}


void Assembler::movl( const Address &dst, int imm32 ) {
    emit_byte( 0xC7 );
    emit_operand( eax, dst );
    emit_long( imm32 );
}


void Assembler::movl( const Address &dst, Oop obj ) {
    emit_byte( 0xC7 );
    emit_operand( eax, dst );
    emit_data( (int) obj, RelocationInformation::RelocationType::oop_type );
}


void Assembler::movl( const Address &dst, const Register &src ) {
    emit_byte( 0x89 );
    emit_operand( src, dst );
}


void Assembler::movsxb( const Register &dst, const Address &src ) {
    emit_byte( 0x0F );
    emit_byte( 0xBE );
    emit_operand( dst, src );
}


void Assembler::movsxb( const Register &dst, const Register &src ) {
    guarantee( src.hasByteRegister(), "must have byte register" );
    emit_byte( 0x0F );
    emit_byte( 0xBE );
    emit_byte( 0xC0 | ( dst.number() << 3 ) | src.number() );
}


void Assembler::movsxw( const Register &dst, const Address &src ) {
    emit_byte( 0x0F );
    emit_byte( 0xBF );
    emit_operand( dst, src );
}


void Assembler::movsxw( const Register &dst, const Register &src ) {
    emit_byte( 0x0F );
    emit_byte( 0xBF );
    emit_byte( 0xC0 | ( dst.number() << 3 ) | src.number() );
}


void Assembler::cmovccl( Condition cc, const Register &dst, int imm32 ) {
    Unimplemented();
}


void Assembler::cmovccl( Condition cc, const Register &dst, Oop obj ) {
    Unimplemented();
}


void Assembler::cmovccl( Condition cc, const Register &dst, const Register &src ) {
    Unimplemented();
}


void Assembler::cmovccl( Condition cc, const Register &dst, const Address &src ) {
    Unimplemented();
}


void Assembler::adcl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xD0, dst, imm32 );
}


void Assembler::adcl( const Register &dst, const Register &src ) {
    emit_arith( 0x13, 0xC0, dst, src );
}


void Assembler::addl( const Address &dst, int imm32 ) {
    emit_byte( 0x81 );
    emit_operand( eax, dst );
    emit_long( imm32 );
}


void Assembler::addl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xC0, dst, imm32 );
}


void Assembler::addl( const Register &dst, const Register &src ) {
    emit_arith( 0x03, 0xC0, dst, src );
}


void Assembler::addl( const Register &dst, const Address &src ) {
    emit_byte( 0x03 );
    emit_operand( dst, src );
}


void Assembler::andl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xE0, dst, imm32 );
}


void Assembler::andl( const Register &dst, const Register &src ) {
    emit_arith( 0x23, 0xC0, dst, src );
}


void Assembler::cmpl( const Address &dst, int imm32 ) {
    emit_byte( 0x81 );
    emit_operand( edi, dst );
    emit_long( imm32 );
}


void Assembler::cmpl( const Address &dst, Oop obj ) {
    emit_byte( 0x81 );
    emit_operand( edi, dst );
    emit_data( (int) obj, RelocationInformation::RelocationType::oop_type );
}


void Assembler::cmpl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xF8, dst, imm32 );
}


void Assembler::cmpl( const Register &dst, Oop obj ) {
    emit_arith( 0x81, 0xF8, dst, obj );
}


void Assembler::cmpl( const Register &dst, const Register &src ) {
    emit_arith( 0x3B, 0xC0, dst, src );
}


void Assembler::cmpl( const Register &dst, const Address &src ) {
    emit_byte( 0x3B );
    emit_operand( dst, src );
}


void Assembler::decb( const Register &dst ) {
    guarantee( dst.hasByteRegister(), "must have byte register" );
    emit_byte( 0xFE );
    emit_byte( 0xC8 | dst.number() );
}


void Assembler::decl( const Register &dst ) {
    emit_byte( 0x48 | dst.number() );
}


void Assembler::decl( const Address &dst ) {
    emit_byte( 0xFF );
    emit_operand( ecx, dst );
}


void Assembler::idivl( const Register &src ) {
    emit_byte( 0xF7 );
    emit_byte( 0xF8 | src.number() );
}


void Assembler::imull( const Register &src ) {
    emit_byte( 0xF7 );
    emit_byte( 0xE8 | src.number() );
}


void Assembler::imull( const Register &dst, const Register &src ) {
    emit_byte( 0x0F );
    emit_byte( 0xAF );
    emit_byte( 0xC0 | dst.number() << 3 | src.number() );
}


void Assembler::imull( const Register &dst, const Register &src, int value ) {
    if ( is8bit( value ) ) {
        emit_byte( 0x6B );
        emit_byte( 0xC0 | dst.number() << 3 | src.number() );
        emit_byte( value );
    } else {
        emit_byte( 0x69 );
        emit_byte( 0xC0 | dst.number() << 3 | src.number() );
        emit_long( value );
    }
}


void Assembler::incl( const Register &dst ) {
    emit_byte( 0x40 | dst.number() );
}


void Assembler::incl( const Address &dst ) {
    emit_byte( 0xFF );
    emit_operand( eax, dst );
}


void Assembler::leal( const Register &dst, const Address &src ) {
    emit_byte( 0x8D );
    emit_operand( dst, src );
}


void Assembler::mull( const Register &src ) {
    emit_byte( 0xF7 );
    emit_byte( 0xE0 | src.number() );
}


void Assembler::negl( const Register &dst ) {
    emit_byte( 0xF7 );
    emit_byte( 0xD8 | dst.number() );
}


void Assembler::notl( const Register &dst ) {
    emit_byte( 0xF7 );
    emit_byte( 0xD0 | dst.number() );
}


void Assembler::orl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xC8, dst, imm32 );
}


void Assembler::orl( const Register &dst, const Register &src ) {
    emit_arith( 0x0B, 0xC0, dst, src );
}


void Assembler::orl( const Register &dst, const Address &src ) {
    emit_byte( 0x0B );
    emit_operand( dst, src );
}


void Assembler::rcll( const Register &dst, int imm8 ) {
    st_assert( isShiftCount( imm8 ), "illegal shift count" );
    if ( imm8 == 1 ) {
        emit_byte( 0xD1 );
        emit_byte( 0xD0 | dst.number() );
    } else {
        emit_byte( 0xC1 );
        emit_byte( 0xD0 | dst.number() );
        emit_byte( imm8 );
    }
}


void Assembler::sarl( const Register &dst, int imm8 ) {
    st_assert( isShiftCount( imm8 ), "illegal shift count" );
    if ( imm8 == 1 ) {
        emit_byte( 0xD1 );
        emit_byte( 0xF8 | dst.number() );
    } else {
        emit_byte( 0xC1 );
        emit_byte( 0xF8 | dst.number() );
        emit_byte( imm8 );
    }
}


void Assembler::sarl( const Register &dst ) {
    emit_byte( 0xD3 );
    emit_byte( 0xF8 | dst.number() );
}


void Assembler::sbbl( const Register &dst, int imm32 ) {
    Unimplemented();
}


void Assembler::sbbl( const Register &dst, const Register &src ) {
    emit_arith( 0x1B, 0xC0, dst, src );
}


void Assembler::shldl( const Register &dst, const Register &src ) {
    emit_byte( 0x0F );
    emit_byte( 0xA5 );
    emit_byte( 0xC0 | src.number() << 3 | dst.number() );
}


void Assembler::shll( const Register &dst, int imm8 ) {
    st_assert( isShiftCount( imm8 ), "illegal shift count" );
    if ( imm8 == 1 ) {
        emit_byte( 0xD1 );
        emit_byte( 0xE0 | dst.number() );
    } else {
        emit_byte( 0xC1 );
        emit_byte( 0xE0 | dst.number() );
        emit_byte( imm8 );
    }
}


void Assembler::shll( const Register &dst ) {
    emit_byte( 0xD3 );
    emit_byte( 0xE0 | dst.number() );
}


void Assembler::shrdl( const Register &dst, const Register &src ) {
    emit_byte( 0x0F );
    emit_byte( 0xAD );
    emit_byte( 0xC0 | src.number() << 3 | dst.number() );
}


void Assembler::shrl( const Register &dst, int imm8 ) {
    st_assert( isShiftCount( imm8 ), "illegal shift count" );
    emit_byte( 0xC1 );
    emit_byte( 0xE8 | dst.number() );
    emit_byte( imm8 );
}


void Assembler::shrl( const Register &dst ) {
    emit_byte( 0xD3 );
    emit_byte( 0xE8 | dst.number() );
}


void Assembler::subl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xE8, dst, imm32 );
}


void Assembler::subl( const Register &dst, const Register &src ) {
    emit_arith( 0x2B, 0xC0, dst, src );
}


void Assembler::subl( const Register &dst, const Address &src ) {
    emit_byte( 0x2B );
    emit_operand( dst, src );
}


void Assembler::testb( const Register &dst, int imm8 ) {
    guarantee( dst.hasByteRegister(), "must have byte register" );
    emit_arith_b( 0xF6, 0xC0, dst, imm8 );
}


void Assembler::testl( const Register &dst, int imm32 ) {
    // not using emit_arith because test
    // doesn't support sign-extension of
    // 8bit operands
    if ( dst.number() == 0 ) {
        emit_byte( 0xA9 );
    } else {
        emit_byte( 0xF7 );
        emit_byte( 0xC0 | dst.number() );
    }
    emit_long( imm32 );
}


void Assembler::testl( const Register &dst, const Register &src ) {
    emit_arith( 0x85, 0xC0, dst, src );
}


void Assembler::xorl( const Register &dst, int imm32 ) {
    emit_arith( 0x81, 0xF0, dst, imm32 );
}


void Assembler::xorl( const Register &dst, const Register &src ) {
    emit_arith( 0x33, 0xC0, dst, src );
}


void Assembler::cdq() {
    emit_byte( 0x99 );
}


void Assembler::hlt() {
    emit_byte( 0xF4 );
}


void Assembler::int3() {
    if ( EnableInt3 )
        emit_byte( 0xCC );
}


void Assembler::nop() {
    emit_byte( 0x90 );
}


void Assembler::ret( int imm16 ) {
    if ( imm16 == 0 ) {
        emit_byte( 0xC3 );
    } else {
        emit_byte( 0xC2 );
        emit_byte( imm16 & 0xFF );
        emit_byte( ( imm16 >> 8 ) & 0xFF );
    }
}


void Assembler::print( const Label &L ) {
    if ( L.is_unused() ) {
        _console->print_cr( "undefined label" );
    } else if ( L.is_bound() ) {
        _console->print_cr( "bound label to %d", L.pos() );
    } else if ( L.is_unbound() ) {
        Label l = L;
        _console->print_cr( "unbound label" );
        while ( l.is_unbound() ) {
            Displacement disp = Displacement( long_at( l.pos() ) );
            _console->print( "@ %d ", l.pos() );
            disp.print();
            _console->cr();
            disp.next( l );
        }
    } else {
        _console->print_cr( "label in inconsistent state (pos = %d)", L._pos );
    }
}


void Assembler::bind_to( const Label &L, int pos ) {
    bool_t tellRobert = false;

    st_assert( 0 <= pos and pos <= offset(), "must have a valid binding position" );
    while ( L.is_unbound() ) {
        Displacement disp      = Displacement( long_at( L.pos() ) );
        int          fixup_pos = L.pos();
        int          imm32     = 0;
        switch ( disp.type() ) {
            case Displacement::Type::call: {
                st_assert( byte_at( fixup_pos - 1 ) == 0xE8, "call expected" );
                imm32 = pos - ( fixup_pos + sizeof( int ) );
            }
                break;
            case Displacement::Type::absolute_jump: {
                st_assert( byte_at( fixup_pos - 1 ) == 0xE9, "jmp expected" );
                imm32          = pos - ( fixup_pos + sizeof( int ) );
                if ( imm32 == 0 and EliminateJumpsToJumps )
                    tellRobert = true;
            }
                break;
            case Displacement::Type::conditional_jump: {
                st_assert( byte_at( fixup_pos - 2 ) == 0x0F, "jcc expected" );
                st_assert( byte_at( fixup_pos - 1 ) == ( 0x80 | disp.info() ), "jcc expected" );
                imm32 = pos - ( fixup_pos + sizeof( int ) );
            }
                break;
            case Displacement::Type::ic_info: {
                st_assert( byte_at( fixup_pos - 1 ) == 0xA9, "test eax expected" );
                int offs = pos - ( fixup_pos - InlineCacheInfo::info_offset );
                st_assert( ( ( offs << InlineCacheInfo::number_of_flags ) >> InlineCacheInfo::number_of_flags ) == offs, "NonLocalReturn offset out of bounds" );
                imm32 = ( offs << InlineCacheInfo::number_of_flags ) | disp.info();
            }
                break;
            default: ShouldNotReachHere();
        }
        long_at_put( fixup_pos, imm32 );
        disp.next( L );
    }
    L.bind_to( pos );

    if ( tellRobert ) {
        //warning("jmp to next has not been eliminated - tell Robert, please");
        code()->decode();
    }
}


void Assembler::link_to( const Label &L, const Label &appendix ) {
    if ( appendix.is_unbound() ) {
        if ( L.is_unbound() ) {
            // append appendix to L's list
            Label p, q = L;
            do {
                p = q;
                Displacement( long_at( q.pos() ) ).next( q );
            } while ( q.is_unbound() );
            Displacement disp = Displacement( long_at( p.pos() ) );
            disp.link_to( appendix );
            long_at_put( p.pos(), disp.data() );
            p.unuse(); // to avoid assertion failure in ~Label
        } else {
            // L is empty, simply use appendix
            L = appendix;
        }
    }
    appendix.unuse(); // appendix should not be used anymore
}


void Assembler::bind( const Label &L ) {
    st_assert( not L.is_bound(), "label can only be bound once" );
    if ( EliminateJumpsToJumps ) {
        // resolve unbound label
        if ( _unbound_label.is_unbound() ) {
            // unbound label exists => link it with L if same binding position, otherwise fix it
            if ( _binding_pos == offset() ) {
                // link it to L's list
                link_to( L, _unbound_label );
            } else {
                // otherwise bind unbound label
                st_assert( _binding_pos < offset(), "assembler error" );
                bind_to( _unbound_label, _binding_pos );
            }
        }
        st_assert( not _unbound_label.is_unbound(), "assembler error" );
        // try to eliminate jumps to next instruction
        while ( L.is_unbound() and ( L.pos() + int( sizeof( int ) ) == offset() ) and ( Displacement( long_at( L.pos() ) ).type() == Displacement::Type::absolute_jump ) ) {
            // previous instruction is jump jumping immediately after it => eliminate it
            constexpr int long_size = 5;
            st_assert( byte_at( offset() - long_size ) == 0xE9, "jmp expected" );
            if ( PrintEliminatedJumps )
                _console->print_cr( "@ %d jump to next eliminated", L.pos() );
            // remove first entry from label list
            Displacement( long_at( L.pos() ) ).next( L );
            // eliminate instruction (set code pointers back)
            _code_pos -= long_size;
            code()->set_code_end( _code_pos );
        }
        // delay fixup of L => store it as unbound label
        _unbound_label = L;
        _binding_pos   = offset();
        L.unuse();
    }
    bind_to( L, offset() );
}


void Assembler::merge( const Label &L, const Label &with ) {
    Unimplemented();
}


void Assembler::call( const Label &L ) {
    if ( L.is_bound() ) {
        constexpr int long_size = 5;
        int           offs      = L.pos() - offset();
        st_assert( offs <= 0, "assembler error" );
        // 1110 1000 #32-bit disp
        emit_byte( 0xE8 );
        emit_long( offs - long_size );
    } else {
        // 1110 1000 #32-bit disp
        emit_byte( 0xE8 );
        Displacement disp( L, Displacement::Type::call, 0 );
        L.link_to( offset() );
        emit_long( int( disp.data() ) );
    }
}


void Assembler::call( const char *entry, RelocationInformation::RelocationType rtype ) {
    emit_byte( 0xE8 );
    emit_data( (int) entry - ( (int) _code_pos + sizeof( std::int32_t ) ), rtype );
}


void Assembler::call( const Register &dst ) {
    emit_byte( 0xFF );
    emit_byte( 0xD0 | dst.number() );
}


void Assembler::call( const Address &adr ) {
    emit_byte( 0xFF );
    emit_operand( edx, adr );
}


void Assembler::jmp( const char *entry, RelocationInformation::RelocationType rtype ) {
    emit_byte( 0xE9 );
    emit_data( (int) entry - ( (int) _code_pos + sizeof( std::int32_t ) ), rtype );
}


void Assembler::jmp( const Register &reg ) {
    emit_byte( 0xFF );
    emit_byte( 0xE0 | reg.number() );
}


void Assembler::jmp( const Address &adr ) {
    emit_byte( 0xFF );
    emit_operand( esp, adr );
}


void Assembler::jmp( const Label &L ) {
    if ( L.is_bound() ) {
        constexpr int short_size = 2;
        constexpr int long_size  = 5;
        int           offs       = L.pos() - offset();
        st_assert( offs <= 0, "assembler error" );
        if ( isByte( offs - short_size ) ) {
            // 1110 1011 #8-bit disp
            emit_byte( 0xEB );
            emit_byte( ( offs - short_size ) & 0xFF );
        } else {
            // 1110 1001 #32-bit disp
            emit_byte( 0xE9 );
            emit_long( offs - long_size );
        }
    } else {
        if ( EliminateJumpsToJumps and _unbound_label.is_unbound() and _binding_pos == offset() ) {
            // current position is target of jumps
            if ( PrintEliminatedJumps ) {
                _console->print_cr( "eliminated jumps/calls to %d", _binding_pos );
                _console->print( "from " );
                print( _unbound_label );
            }
            link_to( L, _unbound_label );
        }
        // 1110 1001 #32-bit disp
        emit_byte( 0xE9 );
        Displacement disp( L, Displacement::Type::absolute_jump, 0 );
        L.link_to( offset() );
        emit_long( int( disp.data() ) );
    }
}


void Assembler::jcc( Condition cc, Label &L ) {
    st_assert( ( 0 <= static_cast<int>(cc) ) and ( static_cast<int>(cc) < 16 ), "illegal cc" );
    if ( L.is_bound() ) {
        constexpr int short_size = 2;
        constexpr int long_size  = 6;
        int           offs       = L.pos() - offset();
        st_assert( offs <= 0, "assembler error" );
        if ( isByte( offs - short_size ) ) {
            // 0111 tttn #8-bit disp
            emit_byte( 0x70 | static_cast<int>(cc) );
            emit_byte( ( offs - short_size ) & 0xFF );
        } else {
            // 0000 1111 1000 tttn #32-bit disp
            emit_byte( 0x0F );
            emit_byte( 0x80 | static_cast<int>(cc) );
            emit_long( offs - long_size );
        }
    } else {
        // 0000 1111 1000 tttn #32-bit disp
        // Note: could eliminate cond. jumps to this jump if condition
        //       is the same however, seems to be rather unlikely case.
        emit_byte( 0x0F );
        emit_byte( 0x80 | static_cast<int>(cc) );
        Displacement disp( L, Displacement::Type::conditional_jump, static_cast<int>(cc) );
        L.link_to( offset() );
        emit_long( int( disp.data() ) );
    }
}


void Assembler::jcc( Condition cc, const char *dst, RelocationInformation::RelocationType rtype ) {
    st_assert( ( 0 <= static_cast<int>(cc) ) and ( static_cast<int>(cc) < 16 ), "illegal cc" );
    // 0000 1111 1000 tttn #32-bit disp
    emit_byte( 0x0F );
    emit_byte( 0x80 | static_cast<int>(cc) );
    emit_data( (int) dst - ( (int) _code_pos + sizeof( std::int32_t ) ), rtype );
}


void Assembler::ic_info( const Label &L, int flags ) {
    st_assert( (std::uint32_t) flags >> InlineCacheInfo::number_of_flags == 0, "too many flags set" );
    if ( L.is_bound() ) {
        int offs = L.pos() - offset();
        st_assert( offs <= 0, "assembler error" );
        st_assert( ( ( offs << InlineCacheInfo::number_of_flags ) >> InlineCacheInfo::number_of_flags ) == offs, "NonLocalReturn offset out of bounds" );
        emit_byte( 0xA9 );
        emit_long( ( offs << InlineCacheInfo::number_of_flags ) | flags );
    } else {
        emit_byte( 0xA9 );
        Displacement disp( L, Displacement::Type::ic_info, flags );
        L.link_to( offset() );
        emit_long( int( disp.data() ) );
    }
}


// FPU instructions

void Assembler::fld1() {
    emit_byte( 0xD9 );
    emit_byte( 0xE8 );
}


void Assembler::fldz() {
    emit_byte( 0xD9 );
    emit_byte( 0xEE );
}


void Assembler::fld_s( const Address &a ) {
    emit_byte( 0xD9 );
    emit_operand( eax, a );
}


void Assembler::fld_d( const Address &a ) {
    emit_byte( 0xDD );
    emit_operand( eax, a );
}


void Assembler::fstp_s( const Address &a ) {
    emit_byte( 0xD9 );
    emit_operand( ebx, a );
}


void Assembler::fstp_d( const Address &a ) {
    emit_byte( 0xDD );
    emit_operand( ebx, a );
}


void Assembler::fild_s( const Address &a ) {
    emit_byte( 0xDB );
    emit_operand( eax, a );
}


void Assembler::fild_d( const Address &a ) {
    emit_byte( 0xDF );
    emit_operand( ebp, a );
}


void Assembler::fistp_s( const Address &a ) {
    emit_byte( 0xDB );
    emit_operand( ebx, a );
}


void Assembler::fistp_d( const Address &a ) {
    emit_byte( 0xDF );
    emit_operand( edi, a );
}


void Assembler::fabs() {
    emit_byte( 0xD9 );
    emit_byte( 0xE1 );
}


void Assembler::fchs() {
    emit_byte( 0xD9 );
    emit_byte( 0xE0 );
}


void Assembler::fadd_d( const Address &a ) {
    emit_byte( 0xDC );
    emit_operand( eax, a );
}


void Assembler::fsub_d( const Address &a ) {
    emit_byte( 0xDC );
    emit_operand( esp, a );
}


void Assembler::fmul_d( const Address &a ) {
    emit_byte( 0xDC );
    emit_operand( ecx, a );
}


void Assembler::fdiv_d( const Address &a ) {
    emit_byte( 0xDC );
    emit_operand( esi, a );
}


void Assembler::fadd( int i ) {
    emit_farith( 0xDC, 0xC0, i );
}


void Assembler::fsub( int i ) {
    emit_farith( 0xDC, 0xE8, i );
}


void Assembler::fmul( int i ) {
    emit_farith( 0xDC, 0xC8, i );
}


void Assembler::fdiv( int i ) {
    emit_farith( 0xDC, 0xF8, i );
}


void Assembler::faddp( int i ) {
    emit_farith( 0xDE, 0xC0, i );
}


void Assembler::fsubp( int i ) {
    emit_farith( 0xDE, 0xE8, i );
}


void Assembler::fsubrp( int i ) {
    emit_farith( 0xDE, 0xE0, i );
}


void Assembler::fmulp( int i ) {
    emit_farith( 0xDE, 0xC8, i );
}


void Assembler::fdivp( int i ) {
    emit_farith( 0xDE, 0xF8, i );
}


void Assembler::fprem() {
    emit_byte( 0xD9 );
    emit_byte( 0xF8 );
}


void Assembler::fprem1() {
    emit_byte( 0xD9 );
    emit_byte( 0xF5 );
}


void Assembler::fxch( int i ) {
    emit_farith( 0xD9, 0xC8, i );
}


void Assembler::fincstp() {
    emit_byte( 0xD9 );
    emit_byte( 0xF7 );
}


void Assembler::ffree( int i ) {
    emit_farith( 0xDD, 0xC0, i );
}


void Assembler::ftst() {
    emit_byte( 0xD9 );
    emit_byte( 0xE4 );
}


void Assembler::fcompp() {
    emit_byte( 0xDE );
    emit_byte( 0xD9 );
}


void Assembler::fnstsw_ax() {
    emit_byte( 0xdF );
    emit_byte( 0xE0 );
}


void Assembler::fwait() {
    emit_byte( 0x9B );
}
