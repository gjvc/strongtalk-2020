//
//
//
//



#include <type_traits>

// The default is /Gd __cdecl
// The alternative is /Gz __stdcall


template<class R, class T0, class T1>
class TFunction<R( __cdecl * )( T0, T1 )> {
    typedef R (__cdecl *func_type)( T0, T1 );
};


template<class R, class T0, class T1>
class TFunction<R( * )( T0, T1 )> {
    typedef R (*func_type)( T0, T1 );
};


#if defined(__GNUC__) || defined(__clang__)
#define CC_CDECL __attribute__((cdecl))
#define CC_STDCALL __attribute__((stdcall))
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define CC_CDECL __cdecl
#define CC_STDCALL __stdcall
#else
#define CC_CDECL
#define CC_STDCALL
#endif


struct CallingConventions {
    using Cdecl = void ( CC_CDECL * )();
    using Stdcall = void ( CC_STDCALL * )();

    template<typename T>
    static constexpr bool IsCdecl = std::is_same_v<Cdecl, T>;

    static constexpr bool HasStdcall = !IsCdecl<Stdcall>;

};


template<typename R, typename... Args>
struct FunctionTraits<R( CC_CDECL * )( Args... )>
    : FunctionTraitsBase<R, Args...> {
    using Pointer = R( CC_CDECL * )( Args... );
    using CallingConvention = CallingConventions::Cdecl;
};


template<typename R, typename... Args>
struct FunctionTraits<R( CC_STDCALL * )( Args... )>
    : FunctionTraitsBase<R, Args...> {
    using Pointer = R( CC_STDCALL * )( Args... );
    using CallingConvention = CallingConventions::Stdcall;
};


template<typename F, typename = void>
struct FunctionTraits;


template<typename R, typename... Args>
struct FunctionTraits<R( CC_CDECL * )( Args... )>
    : FunctionTraitsBase<R, Args...> {
    using Pointer =
    R( CC_CDECL * )( Args... );
    using CallingConvention = CallingConventions::Cdecl;
};


template<typename R, typename... Args>
struct FunctionTraits<R( CC_STDCALL * )( Args... ),
    std::enable_if_t<!std::is_same_v<CallingConventions::Cdecl, CallingConventions::Stdcall> > > :
    FunctionTraitsBase<R, Args...> {
    using Pointer = R( CC_STDCALL * )( Args... );
    using CallingConvention = CallingConventions::Stdcall;
};
