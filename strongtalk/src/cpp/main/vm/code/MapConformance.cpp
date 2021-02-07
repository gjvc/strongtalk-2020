//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/code/MapConformance.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


void Variable::print() {
    if ( in_register() ) {
        _console->print( "R%d", offset() );
    } else if ( on_stack() ) {
        _console->print( "S[%d]", offset() );
    } else if ( is_unused() ) {
        _console->print( "UN" );
    } else if ( is_top_of_stack() ) {
        _console->print( "tos" );
    } else {
        st_fatal( "invalid variable" );
    }
}


Variable Variable::new_register( std::int32_t offset ) {
    return new_variable( reg_type, offset );
}


Variable Variable::new_stack( std::int32_t offset ) {
    return new_variable( stack_type, offset );
}


Variable Variable::unused() {
    return new_variable( special_type, 0 );
}


Variable Variable::top_of_stack() {
    return new_variable( special_type, 1 );
}


bool Variable::in_register() const {
    return type() == reg_type;
}


Variable MapConformance::pop_temporary() {
    Variable result = Variable::top_of_stack();
    if ( not _free_register.is_unused() ) {
        result         = _free_register;
        _free_register = Variable::unused();
    }
    return result;
}


void MapConformance::push_temporary( Variable var ) {
    if ( var.in_register() ) {
        _free_register = var;
    }
}


void MapConformance::push( Variable src, std::int32_t n ) {
    for ( std::int32_t i = 0; i < n; i++ ) {
        push( src );
    }
}


void MappingEntry::print() {
    if ( not reg().is_unused() )
        reg().print();
    if ( not stack().is_unused() )
        stack().print();
}


bool operator==( MappingEntry x, MappingEntry y ) {
    return x.reg() == y.reg() and x.stack() == y.stack();
}


std::int32_t MappingTask::number_of_targets() const {

    std::int32_t result = 0;

    for ( const MappingTask *current = this; current; current = current->next() ) {
        if ( current->dst.has_reg() )
            result++;
        if ( current->dst.has_stack() )
            result++;
    }

    return result;
}


bool MappingTask::in_parent_chain( MappingTask *task ) const {
    for ( const MappingTask *current = this; current; current = current->parent() ) {
        if ( current == task )
            return true;
    }
    return false;
}


bool MappingTask::target_includes( Variable var ) const {
    for ( const MappingTask *current = this; current; current = current->next() ) {
        if ( current->dst.reg() == var or current->dst.stack() == var )
            return true;
    }
    return false;
}


bool MappingTask::is_dependent( MapConformance *mc, MappingTask *task ) const {
    // Do we have to process task before this?
    // => do task->results overlap with src?
    if ( this == task )
        return false;

    bool is_reg_dependent   = false;
    bool is_stack_dependent = false;

    if ( task->src.has_reg() ) {
        is_reg_dependent = target_includes( task->src.reg() );
    }

    if ( task->src.has_stack() ) {
        is_stack_dependent = target_includes( task->src.stack() );
    }

    if ( is_reg_dependent or is_stack_dependent ) {
        if ( in_parent_chain( task ) ) {
            // A....task......this
            if ( task->src.has_reg() ) {
                if ( task->src.has_stack() ) {
                    task->src.set_stack( Variable::unused() );
                }
                Variable temp = mc->pop_temporary();
                if ( temp.is_top_of_stack() ) {
                    mc->push( task->src.reg(), number_of_targets() );
                    task->src.set_reg( temp );
                    task->set_uses_top_of_stack( true );
                } else {
                    mc->move( task->src.reg(), temp );
                    task->src.set_reg( temp );
                    task->set_variable_to_free( temp );
                }
            } else {
                Variable temp = mc->pop_temporary();
                if ( temp.is_top_of_stack() ) {
                    mc->push( task->src.stack(), number_of_targets() );
                    task->src.set_reg( temp );
                    task->set_uses_top_of_stack( true );
                } else {
                    mc->move( task->src.stack(), temp );
                    task->src.set_reg( temp );
                    task->src.set_stack( Variable::unused() );
                    task->set_variable_to_free( temp );
                }
            }
            return false;
        }
    }

    return is_reg_dependent or is_stack_dependent;
}


void MappingTask::process_task( MapConformance *mc, MappingTask *p ) {
    if ( is_processed() )
        return;

    // Is anybody dependent on source?
    set_parent( p );
    for ( std::int32_t i = 0; i < mc->_mappings->length(); i++ ) {
        MappingTask *task = mc->_mappings->at( i );
        if ( not task->is_processed() and is_dependent( mc, task ) ) {
            task->process_task( mc, this );
        }
    }
    set_parent( nullptr );

    // Generate code for task
    generate_code( mc );
    mc->push_temporary( variable_to_free() );
    set_processed( "Code has been generated" );
}


void MappingTask::generate_code( MapConformance *mc ) {
    bool uses_temp = false;

    if ( uses_top_of_stack() ) {
        //Use source register for moves
        for ( MappingTask *current = this; current; current = current->next() ) {
            if ( current->dst.has_reg() ) {
                mc->pop( current->dst.reg() );
            }
            if ( dst.has_stack() and not( src.stack() == dst.stack() ) ) {
                mc->pop( current->dst.stack() );
            }
        }
    } else if ( src.has_reg() ) {
        //Use source register for moves
        for ( MappingTask *current = this; current; current = current->next() ) {
            if ( current->dst.has_reg() ) {
                mc->move( src.reg(), current->dst.reg() );
            }
            if ( current->dst.has_stack() and not( src.stack() == current->dst.stack() ) ) {
                mc->move( src.reg(), current->dst.stack() );
            }
        }
    } else {
        //Use register in target or free register
        Variable          temp;
        for ( MappingTask *current = this; current; current = current->next() ) {
            if ( current->dst.has_reg() )
                temp = current->dst.reg();
        }
        if ( temp.is_unused() ) {
            uses_temp = true;
            temp      = mc->pop_temporary();
        }
        if ( temp.in_register() ) {
            //We found a temporary register
            mc->move( src.stack(), temp );
            for ( MappingTask *current = this; current; current = current->next() ) {
                if ( current->dst.has_reg() and not( current->dst.reg() == temp ) ) {
                    mc->move( temp, current->dst.reg() );
                }
                if ( current->dst.has_stack() and not( src.stack() == current->dst.stack() ) ) {
                    mc->move( temp, current->dst.stack() );
                }
            }
            if ( uses_temp ) {
                mc->push_temporary( temp );
            }
        } else {
            for ( MappingTask *current = this; current; current = current->next() ) {
                if ( current->dst.has_reg() ) {
                    mc->push( src.stack() );
                    mc->pop( current->dst.reg() );
                }
                if ( current->dst.has_stack() and not( src.stack() == current->dst.stack() ) ) {
                    mc->push( src.stack() );
                    mc->pop( current->dst.stack() );
                }
            }
        }
    }
}


void MappingTask::print( std::int32_t index ) {
    _console->print( "  %2d: ", index );
    src.print();
    _console->print( " -> " );
    dst.print();
    if ( _what_happened not_eq nullptr ) {
        _console->set_indentation( 35 );
        _console->indent();
        spdlog::info( "{}", _what_happened );
    }
    _console->cr();
}


MapConformance::MapConformance() :
    _mappings{ new GrowableArray<MappingTask *>( 20 ) },
    _usedVariables{ nullptr },
    _numberOfUsedVariables{ 0 },
    _free_register{} {
    _free_register.set_unused();
}


void MapConformance::append_mapping( Variable src_register, Variable src_stack, Variable dst_register, Variable dst_stack ) {
    _mappings->append( new MappingTask( src_register, src_stack, dst_register, dst_stack ) );
}


void MapConformance::generate( Variable free_register1, Variable free_register2 ) {
    static_cast<void>(free_register2); // unused

    _free_register         = free_register1;
    // There is max. 2 used variables per mapping.
    _usedVariables         = new_resource_array<Variable>( _mappings->length() * 2 );
    _numberOfUsedVariables = 0;

    simplify();
    process_tasks();

    _free_register.set_unused();
    _usedVariables = nullptr;
}


void MapConformance::move( Variable src, Variable dst ) {
    spdlog::info( "move {:d} {:d}", src.value(), dst.value() );
//    _console->print( "	move  " );
//    src.print();
//    _console->print( ",	" );
//    dst.print();
//    _console->cr();
}


void MapConformance::push( Variable src ) {
    spdlog::info( "push {:d}", src.value() );
//    _console->print( "	push  " );
//    src.print();
//    _console->cr();
}


void MapConformance::pop( Variable dst ) {
    spdlog::info( "pop {:d}", dst.value() );
//    _console->print( "	pop  " );
//    dst.print();
//    _console->cr();
}


void MapConformance::print() {
    spdlog::info( "MapConformance" );
    for ( std::int32_t i = 0; i < _mappings->length(); i++ ) {
        _mappings->at( i )->print( i );
    }
}


bool MapConformance::reduce_noop_task( MappingTask *task ) {
    // A noop task if destination is a subset of source
    bool result = true;

    if ( task->dst.has_reg() and task->src.reg() not_eq task->dst.reg() ) {
        result = false;
    }

    if ( task->dst.has_stack() and task->src.stack() not_eq task->dst.stack() ) {
        result = false;
    }

    if ( result )
        task->set_processed( "Noop" );
    return result;
}


void MapConformance::simplify() {
    // Links tasks with identical source.
    for ( std::int32_t x = 0; x < _mappings->length(); x++ ) {
        MappingTask *x_task = _mappings->at( x );
        if ( not x_task->is_processed() and not reduce_noop_task( x_task ) ) {
            for ( std::int32_t y = x + 1; y < _mappings->length(); y++ ) {
                MappingTask *y_task = _mappings->at( y );
                if ( x_task->src == y_task->src )
                    x_task->append( y_task );
            }
        }
    }
}


void MapConformance::process_tasks() {
    for ( std::int32_t i = 0; i < _mappings->length(); i++ ) {
        _mappings->at( i )->process_task( this, nullptr );
    }
}
