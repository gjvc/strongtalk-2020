
// -----------------------------------------------------------------------------


#include <cassert>


template<std::int32_t V>
struct LOG2 {
    enum {
        value = LOG2<V / 2>::value + 1
    };
};

template<>
struct LOG2<1> {
    enum {
        value = 0
    };
};


template<typename T>
class tagged_ptr {

public:

    static constexpr std::int32_t   alignment = alignof( T );
    static constexpr std::uintptr_t tag_bits  = LOG2<alignment>::value;
    static constexpr std::uintptr_t tag_mask  = alignment - static_cast<std::uintptr_t>(1);

    class proxy {
    private:
        proxy() : p( 0UL ), i( 0 ) {}


        proxy( const std::uintptr_t *p, const std::int32_t i ) : p( const_cast<std::uintptr_t *>(p) ), i( i ) {}


        std::uintptr_t *p;
        std::int32_t   i;


    public:
        proxy( const proxy &p ) : p( p.p ), i( p.i ) {}


        proxy &operator=( const proxy &proxy ) {
            this->p = proxy.p;
            i = proxy.i;
            return *this;
        }


        proxy &operator=( const bool b ) {
            ( *p ) ^= ( ( b ? -1 : 0 ) ^ ( *p ) ) & ( 1UL << i );
            return *this;
        }


        explicit operator bool() const { return ( ( 1UL << i ) & *p ) > 0; }


        friend class tagged_ptr<T>;
    };


private:
    union {
        T              *_ptr;
        std::uintptr_t _bits{};
    };

public:

    tagged_ptr() : _ptr( nullptr ) {}


    explicit tagged_ptr( T *p, std::uintptr_t t = 0 ) : _ptr( p ) { tags( t ); }


    tagged_ptr( const tagged_ptr &o ) : _ptr( o._ptr ) {}


    ~tagged_ptr() = default;


    tagged_ptr &operator=( const tagged_ptr &rhs ) {
        _ptr = rhs._ptr;
        return *this;
    }


    tagged_ptr &operator=( const T *p ) {
        std::uintptr_t ttags = tags();
        _ptr = p;
        tags( ttags );
        return *this;
    }


    T &operator*() const { return *ptr(); }


    T *operator->() const { return ptr(); }


    explicit operator bool() const { return ( _bits & ~tag_mask ) != 0; }


    T *ptr() const {
        return reinterpret_cast<T *>( _bits & ~tag_mask );
    }


    [[nodiscard]] std::uintptr_t tags() const { return _bits & tag_mask; }


    void tags( std::uintptr_t t ) {
        assert( ( t & tag_mask ) == t );
        _bits = ( _bits & ~tag_mask ) | ( t & tag_mask );
    }


    void reset() { tags( 0UL ); }


    template<std::int32_t I>
    const proxy tag() const {
        static_assert( I < tag_bits, "Index too big for tagged_ptr operation, it will overwrite part of pointer" );
        return proxy( &_bits, I );
    }


    template<std::int32_t I>
    proxy tag() {
        static_assert( I < tag_bits, "Index too big for tagged_ptr operation, it will overwrite part of pointer" );
        return proxy( &_bits, I );
    }


    template<std::int32_t I>
    void set() {
        static_assert( I < tag_bits, "Index too big for tagged_ptr operation, it will overwrite part of pointer" );
        _bits |= ( 1UL << I );
    }


    template<std::int32_t I>
    void flip() {
        static_assert( I < tag_bits, "Index too big for tagged_ptr operation, it will overwrite part of pointer" );
        _bits ^= ( 1UL << I );
    }


    template<std::int32_t I>
    void clear() {
        static_assert( I < tag_bits, "Index too big for tagged_ptr operation, it will overwrite part of pointer" );
        _bits &= ~( 1UL << I );
    }


    const proxy tag( const std::int32_t i ) const {
        assert( i < tag_bits );
        return proxy( &_bits, i );
    }


    proxy tag( const std::int32_t i ) {
        assert( i < tag_bits );
        return proxy( &_bits, i );
    }


    void set( const std::int32_t i ) {
        assert( i < tag_bits );
        _bits |= ( 1UL << i );
    }


    void flip( const std::int32_t i ) {
        assert( i < tag_bits );
        _bits ^= ( 1UL << i );
    }


    void clear( const std::int32_t i ) {
        assert( i < tag_bits );
        _bits &= ~( 1UL << i );
    }

};
