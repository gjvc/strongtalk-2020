//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"


#define VMSYMBOLS( s ) \
\
    s( smi_overflow,                        "SmallIntegerOverflow" )             \
    s( division_by_zero,                    "DivisionByZero" )                   \
    s( receiver_has_wrong_type,             "ReceiverHasWrongType" )             \
    s( division_not_exact,                  "DivisionNotExact" )                 \
    s( first_argument_has_wrong_type,       "FirstArgumentHasWrongType" )        \
    s( second_argument_has_wrong_type,      "SecondArgumentHasWrongType" )       \
    s( third_argument_has_wrong_type,       "ThirdArgumentHasWrongType" )        \
    s( fourth_argument_has_wrong_type,      "FourthArgumentHasWrongType" )       \
    s( fifth_argument_has_wrong_type,       "FifthArgumentHasWrongType" )        \
    s( sixth_argument_has_wrong_type,       "SixthArgumentHasWrongType" )        \
    s( seventh_argument_has_wrong_type,     "SixthArgumentHasWrongType" )        \
    s( eighth_argument_has_wrong_type,      "SeventhArgumentHasWrongType" )      \
    s( ninth_argument_has_wrong_type,       "EighthArgumentHasWrongType" )       \
    s( tenth_argument_has_wrong_type,       "NinthArgumentHasWrongType" )        \
    s( argument_has_wrong_type,             "ArgumentHasWrongType" )             \
    s( argument_is_invalid,                 "ArgumentIsInvalid" )                \
    s( conversion_failed,                   "ConversionFailed" )                 \
    s( not_found,                           "NotFound" )                         \
    s( plus,                                "+" )                                \
    s( minus,                               "-" )                                \
    s( multiply,                            "*" )                                \
    s( divide,                              "//" )                               \
    s( mod,                                 "\\\\" )                             \
    s( point_at,                            "@" )                                \
    s( equal,                               "=" )                                \
    s( not_equal,                           "~=" )                               \
    s( less_than,                           "<" )                                \
    s( less_than_or_equal,                  "<=" )                               \
    s( greater_than,                        ">" )                                \
    s( greater_than_or_equal,               ">=" )                               \
    s( bitAnd_,                             "bitAnd:" )                          \
    s( bitOr_,                              "bitOr:" )                           \
    s( bitXor_,                             "bitXor:" )                          \
    s( bitShift_,                           "bitShift:" )                        \
    s( bitInvert,                           "bitInvert" )                        \
    s( and_,                                "and:" )                             \
    s( or_,                                 "or:" )                              \
    s( _and,                                "and" )                              \
    s( _or,                                 "or" )                               \
    s( and1,                                "&" )                                \
    s( or1,                                 "|" )                                \
    s( _not,                                "not" )                              \
    s( xor_,                                "xor:" )                             \
    s( eqv_,                                "eqv:" )                             \
    s( at,                                  "at:" )                              \
    s( at_put,                              "at:put:" )                          \
    s( size  ,                              "size" )                             \
    s( double_equal,                        "==" )                               \
    s( double_tilde,                        "~~" )                               \
    s( out_of_bounds,                       "OutOfBounds" )                      \
    s( index_not_valid,                     "IndexNotValid" )                    \
    s( not_yet_implemented,                 "NotYetImplemented" )                \
    s( indexable,                           "Indexable" )                        \
    s( not_oops,                            "ReceiverNotOops" )                  \
    s( not_clonable,                        "ReceiverNotClonable" )              \
    s( value_out_of_range,                  "ValueOutOfRange" )                  \
    s( method_construction_failed,          "MethodConstructionFailed" )         \
    s( primitive_not_defined,               "PrimitiveNotDefined" )              \
    s( smi_conversion_failed,               "SmallIntegerConversionFailed" )     \
    s( not_klass,                           "NotAClass" )                        \
    s( null_proxy_access,                   "NullProxyAccess" )                  \
    s( selector_for_blockMethod,            "block" )                            \
    s( not_indexable,                       "NotIndexable" )                     \
    s( to_by_do,                            "to:by:do:" )                        \
    s( value,                               "value" )                            \
    s( value_,                              "value:" )                           \
    s( if_true,                              "ifTrue:" )                         \
    s( if_true_false,                       "ifTrue:ifFalse:" )                  \
    s( if_false,                            "ifFalse:" )                         \
    s( if_false_true,                       "ifFalse:ifTrue:" )                  \
    s( while_true,                          "whileTrue" )                        \
    s( while_true_,                         "whileTrue:" )                       \
    s( while_false,                         "whileFalse" )                       \
    s( while_false_,                        "whileFalse:" )                      \
    s( empty_queue,                         "EmptyQueue:" )                      \
    s( is_absent,                           "IsAbsent" )                         \
    s( in_scheduler,                        "InScheduler" )                      \
    s( in_async_dll,                        "InAsyncDLL" )                       \
    s( not_in_scheduler,                    "NotInScheduler" )                   \
    s( initialized,                         "Initialized" )                      \
    s( yielded,                             "Yielded" )                          \
    s( running,                             "Running" )                          \
    s( stopped,                             "Stopped" )                          \
    s( preempted,                           "Preempted" )                        \
    s( completed,                           "Completed" )                        \
    s( dead,                                "Dead" )                             \
    s( aborted,                             "Aborted" )                          \
    s( boolean_error,                       "BooleanError" )                     \
    s( lookup_error,                        "LookupError" )                      \
    s( primitive_lookup_error,              "PrimitiveLookupError" )             \
    s( DLL_lookup_error,                    "DLLLookupError" )                   \
    s( NonLocalReturn_error,                "NonLocalReturnError" )              \
    s( float_error,                         "FloatError" )                       \
    s( stack_overflow,                      "StackOverflow" )                    \
    s( is_installed,                        "IsInstalled" )                      \
    s( failed,                              "Failed" )                           \
    s( abs,                                 "abs" )                              \
    s( negated,                             "negated" )                          \
    s( squared,                             "squared" )                          \
    s( sqrt,                                "sqrt" )                             \
    s( sin,                                 "sin" )                              \
    s( cos,                                 "cos" )                              \
    s( tan,                                 "tan" )                              \
    s( exp,                                 "exp" )                              \
    s( ln,                                  "ln" )                               \
    s( as_float,                            "asFloat" )                          \
    s( as_float_value,                      "asFloatValue" )                     \
    s( negative_size,                       "NegativeSize" )                     \
    s( not_active,                          "NotActive" )                        \
    s( primitive_disabled_in_product,       "PrimitiveDisabledInProduct" )       \
    s( activation_is_invalid,               "ActivationIsInvalid" )              \
    s( external_activation,                 "ExternalActivation" )               \
    s( mem_klass,                           "Oops" )                             \
    s( association_klass,                   "GlobalAssociation" )                \
    s( blockClosure_klass,                  "Block" )                            \
    s( byteArray_klass,                     "ByteArray" )                        \
    s( symbol_klass,                        "Symbol" )                           \
    s( context_klass,                       "Context" )                          \
    s( doubleByteArray_klass,               "DoubleByteArray" )                  \
    s( doubleValueArray_klass,              "FloatValueArray" )                  \
    s( double_klass,                        "Float" )                            \
    s( klass_klass,                         "Class" )                            \
    s( method_klass,                        "Method" )                           \
    s( mixin_klass,                         "Mixin" )                            \
    s( objArray_klass,                      "Array" )                            \
    s( weakArray_klass,                     "WeakArray" )                        \
    s( process_klass,                       "Process" )                          \
    s( vframe_klass,                        "Activation" )                       \
    s( proxy_klass,                         "Proxy" )                            \
    s( smi_klass,                           "SmallInteger" )                     \
    s( failed_allocation,                   "FailedAllocation" )                 \
    s( invalid_klass,                       "InvalidKlass" )                     \
    s( illegal_state,                       "IllegalState" )                     \
    s( primitive_trap,                      "PrimitiveTrap" )                    \
    s( error,                               "error" )                            \
    s( error_,                              "error:" )                           \
    s( subclassResponsibility,              "subclassResponsibility" )           \


/* error and error_ are for compiler's type prediction */

#define VMSYMBOL_SUFFIX  _enum
#define VMSYMBOL_ENUM_NAME( name ) name##VMSYMBOL_SUFFIX

#define VMSYMBOL_ENUM( name, string ) VMSYMBOL_ENUM_NAME(name),

enum {
    VMSYMBOLS( VMSYMBOL_ENUM ) terminating_enum
};

#define VMSYMBOL_DECL( name, string ) \
    static SymbolOop name () { return vm_symbols[ VMSYMBOL_ENUM_NAME( name ) ]; }

extern "C" SymbolOop vm_symbols[];

class vmSymbols : AllStatic {

    private:
        friend void vmSymbols_init();

    public:
        static void initialize();


        VMSYMBOLS( VMSYMBOL_DECL )


        // operations for memory management.
        static void follow_contents();

        static void switch_pointers( Oop from, Oop to );

        static void relocate();

        static void verify();
};

#undef VMSYMBOL_DECL
#undef VMSYMBOL_ENUM


// for primitive failures
inline Oop markSymbol( SymbolOop sym ) {
    st_assert( sym->is_mem(), "must have MEMOOP_TAG tag" );
    return Oop( ( const char * ) sym - MEMOOP_TAG + MARK_TAG );
}


inline SymbolOop unmarkSymbol( Oop sym ) {
    st_assert( sym->is_mark(), "must have MARK_TAG tag" );
    Oop res = Oop( ( const char * ) sym - MARK_TAG + MEMOOP_TAG );
    st_assert( res->is_symbol(), "must be symbol" );
    return SymbolOop( res );
}


