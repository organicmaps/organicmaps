/*
Copyright 2012-2017 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_SMART_PTR_ALLOCATE_SHARED_ARRAY_HPP
#define BOOST_SMART_PTR_ALLOCATE_SHARED_ARRAY_HPP

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/type_with_alignment.hpp>

namespace boost {
namespace detail {

template<class>
struct sp_if_array { };

template<class T>
struct sp_if_array<T[]> {
    typedef boost::shared_ptr<T[]> type;
};

template<class>
struct sp_if_size_array { };

template<class T, std::size_t N>
struct sp_if_size_array<T[N]> {
    typedef boost::shared_ptr<T[N]> type;
};

template<class>
struct sp_array_element { };

template<class T>
struct sp_array_element<T[]> {
    typedef T type;
};

template<class T, std::size_t N>
struct sp_array_element<T[N]> {
    typedef T type;
};

template<class T>
struct sp_array_scalar {
    typedef T type;
};

template<class T, std::size_t N>
struct sp_array_scalar<T[N]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T, std::size_t N>
struct sp_array_scalar<const T[N]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T, std::size_t N>
struct sp_array_scalar<volatile T[N]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T, std::size_t N>
struct sp_array_scalar<const volatile T[N]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T>
struct sp_array_scalar<T[]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T>
struct sp_array_scalar<const T[]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T>
struct sp_array_scalar<volatile T[]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T>
struct sp_array_scalar<const volatile T[]> {
    typedef typename sp_array_scalar<T>::type type;
};

template<class T>
struct sp_array_count {
    enum {
        value = 1
    };
};

template<class T, std::size_t N>
struct sp_array_count<T[N]> {
    enum {
        value = N * sp_array_count<T>::value
    };
};

template<class T>
struct sp_array_count<T[]> { };

template<class D, class T>
inline D*
sp_get_deleter(const
    boost::shared_ptr<T>& value) BOOST_NOEXCEPT_OR_NOTHROW
{
    return static_cast<D*>(value._internal_get_untyped_deleter());
}

template<std::size_t N, std::size_t A>
struct sp_array_storage {
    union type {
        char value[N];
        typename boost::type_with_alignment<A>::type other;
    };
};

template<std::size_t N, std::size_t M>
struct sp_max_size {
    enum {
        value = N < M ? M : N
    };
};

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T>
struct sp_bind_allocator {
    typedef typename std::allocator_traits<A>::template
        rebind_alloc<T> type;
};
#else
template<class A, class T>
struct sp_bind_allocator {
    typedef typename A::template rebind<T>::other type;
};
#endif

template<bool, class = void>
struct sp_enable { };

template<class T>
struct sp_enable<true, T> {
    typedef T type;
};

template<class T>
inline
typename sp_enable<boost::has_trivial_destructor<T>::value>::type
sp_array_destroy(T*, std::size_t) BOOST_NOEXCEPT { }

template<class T>
inline
typename sp_enable<!boost::has_trivial_destructor<T>::value>::type
sp_array_destroy(T* storage, std::size_t size)
{
    while (size > 0) {
        storage[--size].~T();
    }
}

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T>
inline void
sp_array_destroy(A& allocator, T* storage, std::size_t size)
{
    while (size > 0) {
        std::allocator_traits<A>::destroy(allocator, &storage[--size]);
    }
}
#endif

#if !defined(BOOST_NO_EXCEPTIONS)
template<class T>
inline
typename sp_enable<boost::has_trivial_constructor<T>::value ||
    boost::has_trivial_destructor<T>::value>::type
sp_array_construct(T* storage, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        ::new(static_cast<void*>(storage + i)) T();
    }
}

template<class T>
inline
typename sp_enable<!boost::has_trivial_constructor<T>::value &&
    !boost::has_trivial_destructor<T>::value>::type
sp_array_construct(T* storage, std::size_t size)
{
    std::size_t i = 0;
    try {
        for (; i < size; ++i) {
            ::new(static_cast<void*>(storage + i)) T();
        }
    } catch (...) {
        while (i > 0) {
            storage[--i].~T();
        }
        throw;
    }
}

template<class T>
inline void
sp_array_construct(T* storage, std::size_t size, const T* list,
    std::size_t count)
{
    std::size_t i = 0;
    try {
        for (; i < size; ++i) {
            ::new(static_cast<void*>(storage + i)) T(list[i % count]);
        }
    } catch (...) {
        while (i > 0) {
            storage[--i].~T();
        }
        throw;
    }
}
#else
template<class T>
inline void
sp_array_construct(T* storage, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        ::new(static_cast<void*>(storage + i)) T();
    }
}

template<class T>
inline void
sp_array_construct(T* storage, std::size_t size, const T* list,
    std::size_t count)
{
    for (std::size_t i = 0; i < size; ++i) {
        ::new(static_cast<void*>(storage + i)) T(list[i % count]);
    }
}
#endif

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#if !defined(BOOST_NO_EXCEPTIONS)
template<class A, class T>
inline void
sp_array_construct(A& allocator, T* storage, std::size_t size)
{
    std::size_t i = 0;
    try {
        for (i = 0; i < size; ++i) {
            std::allocator_traits<A>::construct(allocator, storage + i);
        }
    } catch (...) {
        sp_array_destroy(allocator, storage, i);
        throw;
    }
}

template<class A, class T>
inline void
sp_array_construct(A& allocator, T* storage, std::size_t size,
    const T* list, std::size_t count)
{
    std::size_t i = 0;
    try {
        for (i = 0; i < size; ++i) {
            std::allocator_traits<A>::construct(allocator, storage + i,
                list[i % count]);
        }
    } catch (...) {
        sp_array_destroy(allocator, storage, i);
        throw;
    }
}
#else
template<class A, class T>
inline void
sp_array_construct(A& allocator, T* storage, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        std::allocator_traits<A>::construct(allocator, storage + i);
    }
}

template<class A, class T>
inline void
sp_array_construct(A& allocator, T* storage, std::size_t size,
    const T* list, std::size_t count)
{
    for (std::size_t i = 0; i < size; ++i) {
        std::allocator_traits<A>::construct(allocator, storage + i,
            list[i % count]);
    }
}
#endif
#endif

template<class T>
inline
typename sp_enable<boost::has_trivial_constructor<T>::value>::type
sp_array_default(T*, std::size_t) BOOST_NOEXCEPT { }

#if !defined(BOOST_NO_EXCEPTIONS)
template<class T>
inline
typename sp_enable<!boost::has_trivial_constructor<T>::value &&
    boost::has_trivial_destructor<T>::value>::type
sp_array_default(T* storage, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        ::new(static_cast<void*>(storage + i)) T;
    }
}

template<class T>
inline
typename sp_enable<!boost::has_trivial_constructor<T>::value &&
    !boost::has_trivial_destructor<T>::value>::type
sp_array_default(T* storage, std::size_t size)
{
    std::size_t i = 0;
    try {
        for (; i < size; ++i) {
            ::new(static_cast<void*>(storage + i)) T;
        }
    } catch (...) {
        while (i > 0) {
            storage[--i].~T();
        }
        throw;
    }
}
#else
template<class T>
inline
typename sp_enable<!boost::has_trivial_constructor<T>::value>::type
sp_array_default(T* storage, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        ::new(static_cast<void*>(storage + i)) T;
    }
}
#endif

template<class T, std::size_t N>
struct sp_less_align {
    enum {
        value = (boost::alignment_of<T>::value) < N
    };
};

template<class T, std::size_t N>
BOOST_CONSTEXPR inline
typename sp_enable<sp_less_align<T, N>::value, std::size_t>::type
sp_align(std::size_t size) BOOST_NOEXCEPT
{
    return (sizeof(T) * size + N - 1) & ~(N - 1);
}

template<class T, std::size_t N>
BOOST_CONSTEXPR inline
typename sp_enable<!sp_less_align<T, N>::value, std::size_t>::type
sp_align(std::size_t size) BOOST_NOEXCEPT
{
    return sizeof(T) * size;
}

template<class T>
BOOST_CONSTEXPR inline std::size_t
sp_types(std::size_t size) BOOST_NOEXCEPT
{
    return (size + sizeof(T) - 1) / sizeof(T);
}

template<class T, std::size_t N>
class sp_size_array_deleter {
public:
    template<class U>
    static void operator_fn(U) BOOST_NOEXCEPT { }

    sp_size_array_deleter() BOOST_NOEXCEPT
        : enabled_(false) { }

    template<class A>
    sp_size_array_deleter(const A&) BOOST_NOEXCEPT
        : enabled_(false) { }

    sp_size_array_deleter(const sp_size_array_deleter&) BOOST_NOEXCEPT
        : enabled_(false) { }

    ~sp_size_array_deleter() {
        if (enabled_) {
            sp_array_destroy(reinterpret_cast<T*>(&storage_), N);
        }
    }

    template<class U>
    void operator()(U) {
        if (enabled_) {
            sp_array_destroy(reinterpret_cast<T*>(&storage_), N);
            enabled_ = false;
        }
    }

    void* construct() {
        sp_array_construct(reinterpret_cast<T*>(&storage_), N);
        enabled_ = true;
        return &storage_;
    }

    void* construct(const T* list, std::size_t count) {
        sp_array_construct(reinterpret_cast<T*>(&storage_), N,
            list, count);
        enabled_ = true;
        return &storage_;
    }

    void* construct_default() {
        sp_array_default(reinterpret_cast<T*>(&storage_), N);
        enabled_ = true;
        return &storage_;
    }

private:
    bool enabled_;
    typename sp_array_storage<sizeof(T) * N,
        boost::alignment_of<T>::value>::type storage_;
};

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class T, std::size_t N, class A>
class sp_size_array_destroyer {
public:
    template<class U>
    static void operator_fn(U) BOOST_NOEXCEPT { }

    template<class U>
    sp_size_array_destroyer(const U& allocator) BOOST_NOEXCEPT
        : allocator_(allocator),
          enabled_(false) { }

    sp_size_array_destroyer(const sp_size_array_destroyer& other)
        BOOST_NOEXCEPT
        : allocator_(other.allocator_),
          enabled_(false) { }

    ~sp_size_array_destroyer() {
        if (enabled_) {
            sp_array_destroy(allocator_,
                reinterpret_cast<T*>(&storage_), N);
        }
    }

    template<class U>
    void operator()(U) {
        if (enabled_) {
            sp_array_destroy(allocator_,
                reinterpret_cast<T*>(&storage_), N);
            enabled_ = false;
        }
    }

    void* construct() {
        sp_array_construct(allocator_,
            reinterpret_cast<T*>(&storage_), N);
        enabled_ = true;
        return &storage_;
    }

    void* construct(const T* list, std::size_t count) {
        sp_array_construct(allocator_,
            reinterpret_cast<T*>(&storage_), N, list, count);
        enabled_ = true;
        return &storage_;
    }

    const A& allocator() const BOOST_NOEXCEPT {
        return allocator_;
    }

private:
    typename sp_array_storage<sizeof(T) * N,
        boost::alignment_of<T>::value>::type storage_;
    A allocator_;
    bool enabled_;
};
#endif

template<class T>
class sp_array_deleter {
public:
    template<class U>
    static void operator_fn(U) BOOST_NOEXCEPT { }

    sp_array_deleter(std::size_t size) BOOST_NOEXCEPT
        : address_(0),
          size_(size) { }

    template<class A>
    sp_array_deleter(const A& allocator) BOOST_NOEXCEPT
        : address_(0),
          size_(allocator.size()) { }

    template<class A>
    sp_array_deleter(const A&, std::size_t size) BOOST_NOEXCEPT
        : address_(0),
          size_(size) { }

    sp_array_deleter(const sp_array_deleter& other) BOOST_NOEXCEPT
        : address_(0),
          size_(other.size_) { }

    ~sp_array_deleter() {
        if (address_) {
            sp_array_destroy(static_cast<T*>(address_), size_);
        }
    }

    template<class U>
    void operator()(U) {
        if (address_) {
            sp_array_destroy(static_cast<T*>(address_), size_);
            address_ = 0;
        }
    }

    void construct(T* address) {
        sp_array_construct(address, size_);
        address_ = address;
    }

    void construct(T* address, const T* list, std::size_t count) {
        sp_array_construct(address, size_, list, count);
        address_ = address;
    }

    void construct_default(T* address) {
        sp_array_default(address, size_);
        address_ = address;
    }

    std::size_t size() const BOOST_NOEXCEPT {
        return size_;
    }

private:
    void* address_;
    std::size_t size_;
};

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class T, class A>
class sp_array_destroyer {
public:
    template<class U>
    static void operator_fn(U) BOOST_NOEXCEPT { }

    template<class U>
    sp_array_destroyer(const U& allocator, std::size_t size)
        BOOST_NOEXCEPT
        : allocator_(allocator),
          size_(size),
          address_(0) { }

    template<class U>
    sp_array_destroyer(const U& allocator) BOOST_NOEXCEPT
        : allocator_(allocator.allocator()),
          size_(allocator.size()),
          address_(0) { }

    sp_array_destroyer(const sp_array_destroyer& other) BOOST_NOEXCEPT
        : allocator_(other.allocator_),
          size_(other.size_),
          address_(0) { }

    ~sp_array_destroyer() {
        if (address_) {
            sp_array_destroy(allocator_, static_cast<T*>(address_),
                size_);
        }
    }

    template<class U>
    void operator()(U) {
        if (address_) {
            sp_array_destroy(allocator_, static_cast<T*>(address_),
                size_);
            address_ = 0;
        }
    }

    void construct(T* address) {
        sp_array_construct(allocator_, address, size_);
        address_ = address;
    }

    void construct(T* address, const T* list, std::size_t count) {
        sp_array_construct(allocator_, address, size_, list, count);
        address_ = address;
    }

    const A& allocator() const BOOST_NOEXCEPT {
        return allocator_;
    }

    std::size_t size() const BOOST_NOEXCEPT {
        return size_;
    }

private:
    A allocator_;
    std::size_t size_;
    void* address_;
};
#endif

template<class T, class A>
class sp_array_allocator {
    template<class U, class V>
    friend class sp_array_allocator;

public:
    typedef typename A::value_type value_type;

private:
    enum {
        alignment = sp_max_size<boost::alignment_of<T>::value,
            boost::alignment_of<value_type>::value>::value
    };

    typedef typename boost::type_with_alignment<alignment>::type type;
    typedef typename sp_bind_allocator<A, type>::type type_allocator;

public:
    template<class U>
    struct rebind {
        typedef sp_array_allocator<T,
            typename sp_bind_allocator<A, U>::type> other;
    };

    sp_array_allocator(const A& allocator, std::size_t size,
        void** result) BOOST_NOEXCEPT
        : allocator_(allocator),
          size_(size),
          result_(result) { }

    sp_array_allocator(const A& allocator, std::size_t size)
        BOOST_NOEXCEPT
        : allocator_(allocator),
          size_(size) { }

    template<class U>
    sp_array_allocator(const sp_array_allocator<T, U>& other)
        BOOST_NOEXCEPT
        : allocator_(other.allocator_),
          size_(other.size_),
          result_(other.result_) { }

    value_type* allocate(std::size_t count) {
        type_allocator allocator(allocator_);
        std::size_t node = sp_align<value_type, alignment>(count);
        std::size_t size = sp_types<type>(node + sizeof(T) * size_);
        type* address = allocator.allocate(size);
        *result_ = reinterpret_cast<char*>(address) + node;
        return reinterpret_cast<value_type*>(address);
    }

    void deallocate(value_type* value, std::size_t count) {
        type_allocator allocator(allocator_);
        std::size_t node = sp_align<value_type, alignment>(count);
        std::size_t size = sp_types<type>(node + sizeof(T) * size_);
        allocator.deallocate(reinterpret_cast<type*>(value), size);
    }

    const A& allocator() const BOOST_NOEXCEPT {
        return allocator_;
    }

    std::size_t size() const BOOST_NOEXCEPT {
        return size_;
    }

private:
    A allocator_;
    std::size_t size_;
    void** result_;
};

template<class T, class U, class V>
inline bool
operator==(const sp_array_allocator<T, U>& first,
    const sp_array_allocator<T, V>& second) BOOST_NOEXCEPT
{
    return first.allocator() == second.allocator() &&
        first.size() == second.size();
}

template<class T, class U, class V>
inline bool
operator!=(const sp_array_allocator<T, U>& first,
    const sp_array_allocator<T, V>& second) BOOST_NOEXCEPT
{
    return !(first == second);
}

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T, std::size_t N>
struct sp_select_size_deleter {
    typedef sp_size_array_destroyer<T, N,
        typename sp_bind_allocator<A, T>::type> type;
};

template<class U, class T, std::size_t N>
struct sp_select_size_deleter<std::allocator<U>, T, N> {
    typedef sp_size_array_deleter<T, N> type;
};

template<class A, class T>
struct sp_select_deleter {
    typedef sp_array_destroyer<T,
        typename sp_bind_allocator<A, T>::type> type;
};

template<class U, class T>
struct sp_select_deleter<std::allocator<U>, T> {
    typedef sp_array_deleter<T> type;
};
#else
template<class, class T, std::size_t N>
struct sp_select_size_deleter {
    typedef sp_size_array_deleter<T, N> type;
};

template<class, class T>
struct sp_select_deleter {
    typedef sp_array_deleter<T> type;
};
#endif

template<class P, class T, std::size_t N, class A>
class sp_counted_impl_pda<P, sp_size_array_deleter<T, N>, A>
    : public sp_counted_base {
public:
    typedef sp_size_array_deleter<T, N> deleter_type;

private:
    typedef sp_counted_impl_pda<P, deleter_type, A> type;
    typedef typename sp_bind_allocator<A, type>::type deallocator;

public:
    sp_counted_impl_pda(P, const deleter_type&, const A& allocator)
        : deleter_(allocator),
          allocator_(allocator) { }

    sp_counted_impl_pda(P, const A& allocator)
        : deleter_(allocator) { }

    void dispose() {
        deleter_(0);
    }

    void destroy() {
        deallocator allocator(allocator_);
        this->~type();
        allocator.deallocate(this, 1);
    }

    void* get_deleter(const sp_typeinfo&) {
        return &reinterpret_cast<char&>(deleter_);
    }

    void* get_untyped_deleter() {
        return &reinterpret_cast<char&>(deleter_);
    }

private:
    deleter_type deleter_;
    A allocator_;
};

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class P, class T, std::size_t N, class U, class A>
class sp_counted_impl_pda<P, sp_size_array_destroyer<T, N, U>, A>
    : public sp_counted_base {
public:
    typedef sp_size_array_destroyer<T, N, U> deleter_type;

private:
    typedef sp_counted_impl_pda<P, deleter_type, A> type;
    typedef typename sp_bind_allocator<A, type>::type deallocator;

public:
    sp_counted_impl_pda(P, const deleter_type&, const A& allocator)
        : deleter_(allocator) { }

    sp_counted_impl_pda(P, const A& allocator)
        : deleter_(allocator) { }

    void dispose() {
        deleter_(0);
    }

    void destroy() {
        deallocator allocator(deleter_.allocator());
        this->~type();
        allocator.deallocate(this, 1);
    }

    void* get_deleter(const sp_typeinfo&) {
        return &reinterpret_cast<char&>(deleter_);
    }

    void* get_untyped_deleter() {
        return &reinterpret_cast<char&>(deleter_);
    }

private:
    deleter_type deleter_;
};
#endif

template<class P, class T, class A>
class sp_counted_impl_pda<P, sp_array_deleter<T>,
    sp_array_allocator<T, A> >
    : public sp_counted_base {
public:
    typedef sp_array_deleter<T> deleter_type;
    typedef sp_array_allocator<T, A> allocator_type;

private:
    typedef sp_counted_impl_pda<P, deleter_type, allocator_type> type;
    typedef sp_array_allocator<T,
        typename sp_bind_allocator<A, type>::type> deallocator;

public:
    sp_counted_impl_pda(P, const deleter_type&,
        const allocator_type& allocator)
        : deleter_(allocator),
          allocator_(allocator.allocator()) { }

    sp_counted_impl_pda(P, const allocator_type& allocator)
        : deleter_(allocator),
          allocator_(allocator.allocator()) { }

    void dispose() {
        deleter_(0);
    }

    void destroy() {
        deallocator allocator(allocator_, deleter_.size());
        this->~type();
        allocator.deallocate(this, 1);
    }

    void* get_deleter(const sp_typeinfo&) {
        return &reinterpret_cast<char&>(deleter_);
    }

    void* get_untyped_deleter() {
        return &reinterpret_cast<char&>(deleter_);
    }

private:
    deleter_type deleter_;
    A allocator_;
};

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
template<class P, class T, class U, class A>
class sp_counted_impl_pda<P, sp_array_destroyer<T, U>,
    sp_array_allocator<T, A> >
    : public sp_counted_base {
public:
    typedef sp_array_destroyer<T, U> deleter_type;
    typedef sp_array_allocator<T, A> allocator_type;

private:
    typedef sp_counted_impl_pda<P, deleter_type, allocator_type> type;
    typedef sp_array_allocator<T,
        typename sp_bind_allocator<A, type>::type> deallocator;

public:
    sp_counted_impl_pda(P, const deleter_type&,
        const allocator_type& allocator)
        : deleter_(allocator) { }

    sp_counted_impl_pda(P, const allocator_type& allocator)
        : deleter_(allocator) { }

    void dispose() {
        deleter_(0);
    }

    void destroy() {
        deallocator allocator(deleter_.allocator(), deleter_.size());
        this->~type();
        allocator.deallocate(this, 1);
    }

    void* get_deleter(const sp_typeinfo&) {
        return &reinterpret_cast<char&>(deleter_);
    }

    void* get_untyped_deleter() {
        return &reinterpret_cast<char&>(deleter_);
    }

private:
    deleter_type deleter_;
};
#endif

} /* detail */

template<class T, class A>
inline typename detail::sp_if_size_array<T>::type
allocate_shared(const A& allocator)
{
    typedef typename detail::sp_array_element<T>::type type;
    typedef typename detail::sp_array_scalar<T>::type scalar;
    typedef typename detail::sp_select_size_deleter<A, scalar,
        detail::sp_array_count<T>::value>::type deleter;
    shared_ptr<T> result(static_cast<type*>(0),
        detail::sp_inplace_tag<deleter>(), allocator);
    deleter* state = detail::sp_get_deleter<deleter>(result);
    void* start = state->construct();
    return shared_ptr<T>(result, static_cast<type*>(start));
}

template<class T, class A>
inline typename detail::sp_if_size_array<T>::type
allocate_shared(const A& allocator,
    const typename detail::sp_array_element<T>::type& value)
{
    typedef typename detail::sp_array_element<T>::type type;
    typedef typename detail::sp_array_scalar<T>::type scalar;
    typedef typename detail::sp_select_size_deleter<A, scalar,
        detail::sp_array_count<T>::value>::type deleter;
    shared_ptr<T> result(static_cast<type*>(0),
        detail::sp_inplace_tag<deleter>(), allocator);
    deleter* state = detail::sp_get_deleter<deleter>(result);
    void* start = state->construct(reinterpret_cast<const
        scalar*>(&value), detail::sp_array_count<type>::value);
    return shared_ptr<T>(result, static_cast<type*>(start));
}

template<class T, class A>
inline typename detail::sp_if_size_array<T>::type
allocate_shared_noinit(const A& allocator)
{
    typedef typename detail::sp_array_element<T>::type type;
    typedef typename detail::sp_array_scalar<T>::type scalar;
    typedef detail::sp_size_array_deleter<scalar,
        detail::sp_array_count<T>::value> deleter;
    shared_ptr<T> result(static_cast<type*>(0),
        detail::sp_inplace_tag<deleter>(), allocator);
    deleter* state = detail::sp_get_deleter<deleter>(result);
    void* start = state->construct_default();
    return shared_ptr<T>(result, static_cast<type*>(start));
}

template<class T, class A>
inline typename detail::sp_if_array<T>::type
allocate_shared(const A& allocator, std::size_t count)
{
    typedef typename detail::sp_array_element<T>::type type;
    typedef typename detail::sp_array_scalar<T>::type scalar;
    typedef typename detail::sp_select_deleter<A, scalar>::type deleter;
    std::size_t size = count * detail::sp_array_count<type>::value;
    void* start;
    shared_ptr<T> result(static_cast<type*>(0),
        detail::sp_inplace_tag<deleter>(),
        detail::sp_array_allocator<scalar, A>(allocator, size, &start));
    deleter* state = detail::sp_get_deleter<deleter>(result);
    state->construct(static_cast<scalar*>(start));
    return shared_ptr<T>(result, static_cast<type*>(start));
}

template<class T, class A>
inline typename detail::sp_if_array<T>::type
allocate_shared(const A& allocator, std::size_t count,
    const typename detail::sp_array_element<T>::type& value)
{
    typedef typename detail::sp_array_element<T>::type type;
    typedef typename detail::sp_array_scalar<T>::type scalar;
    typedef typename detail::sp_select_deleter<A, scalar>::type deleter;
    std::size_t size = count * detail::sp_array_count<type>::value;
    void* start;
    shared_ptr<T> result(static_cast<type*>(0),
        detail::sp_inplace_tag<deleter>(),
        detail::sp_array_allocator<scalar, A>(allocator, size, &start));
    deleter* state = detail::sp_get_deleter<deleter>(result);
    state->construct(static_cast<scalar*>(start),
        reinterpret_cast<const scalar*>(&value),
        detail::sp_array_count<type>::value);
    return shared_ptr<T>(result, static_cast<type*>(start));
}

template<class T, class A>
inline typename detail::sp_if_array<T>::type
allocate_shared_noinit(const A& allocator, std::size_t count)
{
    typedef typename detail::sp_array_element<T>::type type;
    typedef typename detail::sp_array_scalar<T>::type scalar;
    typedef detail::sp_array_deleter<scalar> deleter;
    std::size_t size = count * detail::sp_array_count<type>::value;
    void* start;
    shared_ptr<T> result(static_cast<type*>(0),
        detail::sp_inplace_tag<deleter>(),
        detail::sp_array_allocator<scalar, A>(allocator, size, &start));
    deleter* state = detail::sp_get_deleter<deleter>(result);
    state->construct_default(static_cast<scalar*>(start));
    return shared_ptr<T>(result, static_cast<type*>(start));
}

} /* boost */

#endif
