#ifndef MAPBOX_UTIL_VARIANT_HPP
#define MAPBOX_UTIL_VARIANT_HPP

#include <utility>
#include <typeinfo>
#include <type_traits>
#include <stdexcept> // runtime_error
#include <new> // operator new
#include <cstddef> // size_t
#include <iosfwd>
#include <string>

#include "recursive_wrapper.hpp"

#ifdef _MSC_VER
 // http://msdn.microsoft.com/en-us/library/z8y1yy88.aspx
 #ifdef NDEBUG
  #define VARIANT_INLINE __forceinline
 #else
  #define VARIANT_INLINE __declspec(noinline)
 #endif
#else
 #ifdef NDEBUG
  #define VARIANT_INLINE inline __attribute__((always_inline))
 #else
  #define VARIANT_INLINE __attribute__((noinline))
 #endif
#endif

#define VARIANT_MAJOR_VERSION 0
#define VARIANT_MINOR_VERSION 1
#define VARIANT_PATCH_VERSION 0

// translates to 100
#define VARIANT_VERSION (VARIANT_MAJOR_VERSION*100000) + (VARIANT_MINOR_VERSION*100) + (VARIANT_PATCH_VERSION)

namespace mapbox { namespace util {

// static visitor
template <typename R = void>
struct static_visitor
{
    using result_type = R;
protected:
    static_visitor() {}
    ~static_visitor() {}
};

namespace detail {

static constexpr std::size_t invalid_value = std::size_t(-1);

template <typename T, typename...Types>
struct direct_type;

template <typename T, typename First, typename...Types>
struct direct_type<T, First, Types...>
{
    static constexpr std::size_t index = std::is_same<T, First>::value
        ? sizeof...(Types) : direct_type<T, Types...>::index;
};

template <typename T>
struct direct_type<T>
{
    static constexpr std::size_t index = invalid_value;
};

template <typename T, typename...Types>
struct convertible_type;

template <typename T, typename First, typename...Types>
struct convertible_type<T, First, Types...>
{
    static constexpr std::size_t index = std::is_convertible<T, First>::value
        ? sizeof...(Types) : convertible_type<T, Types...>::index;
};

template <typename T>
struct convertible_type<T>
{
    static constexpr std::size_t index = invalid_value;
};

template <typename T, typename...Types>
struct value_traits
{
    static constexpr std::size_t direct_index = direct_type<T, Types...>::index;
    static constexpr std::size_t index =
        (direct_index == invalid_value) ? convertible_type<T, Types...>::index : direct_index;
};

template <typename T, typename...Types>
struct is_valid_type;

template <typename T, typename First, typename... Types>
struct is_valid_type<T, First, Types...>
{
    static constexpr bool value = std::is_convertible<T, First>::value
        || is_valid_type<T, Types...>::value;
};

template <typename T>
struct is_valid_type<T> : std::false_type {};

template <std::size_t N, typename ... Types>
struct select_type
{
    static_assert(N < sizeof...(Types), "index out of bounds");
};

template <std::size_t N, typename T, typename ... Types>
struct select_type<N, T, Types...>
{
    using type = typename select_type<N - 1, Types...>::type;
};

template <typename T, typename ... Types>
struct select_type<0, T, Types...>
{
    using type = T;
};


template <typename T, typename R = void>
struct enable_if_type { using type = R; };

template <typename F, typename V, typename Enable = void>
struct result_of_unary_visit
{
    using type = typename std::result_of<F(V&)>::type;
};

template <typename F, typename V>
struct result_of_unary_visit<F, V, typename enable_if_type<typename F::result_type>::type >
{
    using type = typename F::result_type;
};

template <typename F, typename V, class Enable = void>
struct result_of_binary_visit
{
    using type = typename std::result_of<F(V&,V&)>::type;
};


template <typename F, typename V>
struct result_of_binary_visit<F, V, typename enable_if_type<typename F::result_type>::type >
{
    using type = typename F::result_type;
};


} // namespace detail


template <std::size_t arg1, std::size_t ... others>
struct static_max;

template <std::size_t arg>
struct static_max<arg>
{
    static const std::size_t value = arg;
};

template <std::size_t arg1, std::size_t arg2, std::size_t ... others>
struct static_max<arg1, arg2, others...>
{
    static const std::size_t value = arg1 >= arg2 ? static_max<arg1, others...>::value :
        static_max<arg2, others...>::value;
};

template<typename... Types>
struct variant_helper;

template<typename T, typename... Types>
struct variant_helper<T, Types...>
{
    VARIANT_INLINE static void destroy(const std::size_t id, void * data)
    {
        if (id == sizeof...(Types))
        {
            reinterpret_cast<T*>(data)->~T();
        }
        else
        {
            variant_helper<Types...>::destroy(id, data);
        }
    }

    VARIANT_INLINE static void move(const std::size_t old_id, void * old_value, void * new_value)
    {
        if (old_id == sizeof...(Types))
        {
            new (new_value) T(std::move(*reinterpret_cast<T*>(old_value)));
            //std::memcpy(new_value, old_value, sizeof(T));
            // ^^  DANGER: this should only be considered for relocatable types e.g built-in types
            // Also, I don't see any measurable performance benefit just yet
        }
        else
        {
            variant_helper<Types...>::move(old_id, old_value, new_value);
        }
    }

    VARIANT_INLINE static void copy(const std::size_t old_id, const void * old_value, void * new_value)
    {
        if (old_id == sizeof...(Types))
        {
            new (new_value) T(*reinterpret_cast<const T*>(old_value));
        }
        else
        {
            variant_helper<Types...>::copy(old_id, old_value, new_value);
        }
    }
};

template<> struct variant_helper<>
{
    VARIANT_INLINE static void destroy(const std::size_t, void *) {}
    VARIANT_INLINE static void move(const std::size_t, void *, void *) {}
    VARIANT_INLINE static void copy(const std::size_t, const void *, void *) {}
};

namespace detail {

template <typename T>
struct unwrapper
{
    T const& operator() (T const& obj) const
    {
        return obj;
    }

    T& operator() (T & obj) const
    {
        return obj;
    }
};


template <typename T>
struct unwrapper<recursive_wrapper<T>>
{
    auto operator() (recursive_wrapper<T> const& obj) const
        -> typename recursive_wrapper<T>::type const&
    {
        return obj.get();
    }
};

template <typename T>
struct unwrapper<std::reference_wrapper<T>>
{
    auto operator() (std::reference_wrapper<T> const& obj) const
        -> typename std::reference_wrapper<T>::type const&
    {
        return obj.get();
    }

};


template <typename F, typename V, typename R, typename...Types>
struct dispatcher;

template <typename F, typename V, typename R, typename T, typename...Types>
struct dispatcher<F, V, R, T, Types...>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const& v, F f)
    {
        if (v.get_type_index() == sizeof...(Types))
        {
            return f(unwrapper<T>()(v. template get<T>()));
        }
        else
        {
            return dispatcher<F, V, R, Types...>::apply_const(v, f);
        }
    }

    VARIANT_INLINE static result_type apply(V & v, F f)
    {
        if (v.get_type_index() == sizeof...(Types))
        {
            return f(unwrapper<T>()(v. template get<T>()));
        }
        else
        {
            return dispatcher<F, V, R, Types...>::apply(v, f);
        }
    }
};

template<typename F, typename V, typename R>
struct dispatcher<F, V, R>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const&, F)
    {
        throw std::runtime_error(std::string("unary dispatch: FAIL ") + typeid(V).name());
    }

    VARIANT_INLINE static result_type apply(V &, F)
    {
        throw std::runtime_error(std::string("unary dispatch: FAIL ") + typeid(V).name());
    }
};


template <typename F, typename V, typename R, typename T, typename...Types>
struct binary_dispatcher_rhs;

template <typename F, typename V, typename R, typename T0, typename T1, typename...Types>
struct binary_dispatcher_rhs<F, V, R, T0, T1, Types...>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const& lhs, V const& rhs, F f)
    {
        if (rhs.get_type_index() == sizeof...(Types)) // call binary functor
        {
            return f(unwrapper<T0>()(lhs. template get<T0>()),
                     unwrapper<T1>()(rhs. template get<T1>()));
        }
        else
        {
            return binary_dispatcher_rhs<F, V, R, T0, Types...>::apply_const(lhs, rhs, f);
        }
    }

    VARIANT_INLINE static result_type apply(V & lhs, V & rhs, F f)
    {
        if (rhs.get_type_index() == sizeof...(Types)) // call binary functor
        {
            return f(unwrapper<T0>()(lhs. template get<T0>()),
                     unwrapper<T1>()(rhs. template get<T1>()));
        }
        else
        {
            return binary_dispatcher_rhs<F, V, R, T0, Types...>::apply(lhs, rhs, f);
        }
    }

};

template<typename F, typename V, typename R, typename T>
struct binary_dispatcher_rhs<F, V, R, T>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const&, V const&, F)
    {
        throw std::runtime_error("binary dispatch: FAIL");
    }
    VARIANT_INLINE static result_type apply(V &, V &, F)
    {
        throw std::runtime_error("binary dispatch: FAIL");
    }
};


template <typename F, typename V, typename R,  typename T, typename...Types>
struct binary_dispatcher_lhs;

template <typename F, typename V, typename R, typename T0, typename T1, typename...Types>
struct binary_dispatcher_lhs<F, V, R, T0, T1, Types...>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const& lhs, V const& rhs, F f)
    {
        if (lhs.get_type_index() == sizeof...(Types)) // call binary functor
        {
            return f(lhs. template get<T1>(), rhs. template get<T0>());
        }
        else
        {
            return binary_dispatcher_lhs<F, V, R, T0, Types...>::apply_const(lhs, rhs, f);
        }
    }

    VARIANT_INLINE static result_type apply(V & lhs, V & rhs, F f)
    {
        if (lhs.get_type_index() == sizeof...(Types)) // call binary functor
        {
            return f(lhs. template get<T1>(), rhs. template get<T0>());
        }
        else
        {
            return binary_dispatcher_lhs<F, V, R, T0, Types...>::apply(lhs, rhs, f);
        }
    }

};

template<typename F, typename V, typename R, typename T>
struct binary_dispatcher_lhs<F, V, R, T>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const&, V const&, F)
    {
        throw std::runtime_error("binary dispatch: FAIL");
    }

    VARIANT_INLINE static result_type apply(V &, V &, F)
    {
        throw std::runtime_error("binary dispatch: FAIL");
    }
};

template <typename F, typename V, typename R, typename...Types>
struct binary_dispatcher;

template <typename F, typename V, typename R, typename T, typename...Types>
struct binary_dispatcher<F, V, R, T, Types...>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const& v0, V const& v1, F f)
    {
        if (v0.get_type_index() == sizeof...(Types))
        {
            if (v0.get_type_index() == v1.get_type_index())
            {
                return f(v0. template get<T>(), v1. template get<T>()); // call binary functor
            }
            else
            {
                return binary_dispatcher_rhs<F, V, R, T, Types...>::apply_const(v0, v1, f);
            }
        }
        else if (v1.get_type_index() == sizeof...(Types))
        {
            return binary_dispatcher_lhs<F, V, R, T, Types...>::apply_const(v0, v1, f);
        }
        return binary_dispatcher<F, V, R, Types...>::apply_const(v0, v1, f);
    }

    VARIANT_INLINE static result_type apply(V & v0, V & v1, F f)
    {
        if (v0.get_type_index() == sizeof...(Types))
        {
            if (v0.get_type_index() == v1.get_type_index())
            {
                return f(v0. template get<T>(), v1. template get<T>()); // call binary functor
            }
            else
            {
                return binary_dispatcher_rhs<F, V, R, T, Types...>::apply(v0, v1, f);
            }
        }
        else if (v1.get_type_index() == sizeof...(Types))
        {
            return binary_dispatcher_lhs<F, V, R, T, Types...>::apply(v0, v1, f);
        }
        return binary_dispatcher<F, V, R, Types...>::apply(v0, v1, f);
    }
};

template<typename F, typename V, typename R>
struct binary_dispatcher<F, V, R>
{
    using result_type = R;
    VARIANT_INLINE static result_type apply_const(V const&, V const&, F)
    {
        throw std::runtime_error("binary dispatch: FAIL");
    }

    VARIANT_INLINE static result_type apply(V &, V &, F)
    {
        throw std::runtime_error("binary dispatch: FAIL");
    }
};

// comparator functors
struct equal_comp
{
    template <typename T>
    bool operator()(T const& lhs, T const& rhs) const
    {
        return lhs == rhs;
    }
};

struct less_comp
{
    template <typename T>
    bool operator()(T const& lhs, T const& rhs) const
    {
        return lhs < rhs;
    }
};

template <typename Variant, typename Comp>
class comparer
{
public:
    explicit comparer(Variant const& lhs) noexcept
        : lhs_(lhs) {}
    comparer& operator=(comparer const&) = delete;
    // visitor
    template<typename T>
    bool operator()(T const& rhs_content) const
    {
        T const& lhs_content = lhs_.template get<T>();
        return Comp()(lhs_content, rhs_content);
    }
private:
    Variant const& lhs_;
};


} // namespace detail

struct no_init {};

template<typename... Types>
class variant
{
private:

    static const std::size_t data_size = static_max<sizeof(Types)...>::value;
    static const std::size_t data_align = static_max<alignof(Types)...>::value;

    using data_type = typename std::aligned_storage<data_size, data_align>::type;
    using helper_type = variant_helper<Types...>;

    std::size_t type_index;
    data_type data;

public:

    VARIANT_INLINE variant()
        : type_index(sizeof...(Types) - 1)
    {
        new (&data) typename detail::select_type<0, Types...>::type();
    }

    VARIANT_INLINE variant(no_init)
        : type_index(detail::invalid_value) {}

    // http://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers
    template <typename T, class = typename std::enable_if<
                          detail::is_valid_type<typename std::remove_reference<T>::type, Types...>::value>::type>
    VARIANT_INLINE variant(T && val) noexcept
        : type_index(detail::value_traits<typename std::remove_reference<T>::type, Types...>::index)
    {
        constexpr std::size_t index = sizeof...(Types) - detail::value_traits<typename std::remove_reference<T>::type, Types...>::index - 1;
        using target_type = typename detail::select_type<index, Types...>::type;
        new (&data) target_type(std::forward<T>(val)); // nothrow
    }

    VARIANT_INLINE variant(variant<Types...> const& old)
        : type_index(old.type_index)
    {
        helper_type::copy(old.type_index, &old.data, &data);
    }

    VARIANT_INLINE variant(variant<Types...>&& old) noexcept
        : type_index(old.type_index)
    {
        helper_type::move(old.type_index, &old.data, &data);
    }

    friend void swap(variant<Types...> & first, variant<Types...> & second)
    {
        using std::swap; //enable ADL
        swap(first.type_index, second.type_index);
        swap(first.data, second.data);
    }

    VARIANT_INLINE variant<Types...>& operator=(variant<Types...> other)
    {
        swap(*this, other);
        return *this;
    }

    // conversions
    // move-assign
    template <typename T>
    VARIANT_INLINE variant<Types...>& operator=(T && rhs) noexcept
    {
        variant<Types...> temp(std::forward<T>(rhs));
        swap(*this, temp);
        return *this;
    }

    // copy-assign
    template <typename T>
    VARIANT_INLINE variant<Types...>& operator=(T const& rhs)
    {
        variant<Types...> temp(rhs);
        swap(*this, temp);
        return *this;
    }

    template<typename T>
    VARIANT_INLINE bool is() const
    {
        return (type_index == detail::direct_type<T, Types...>::index);
    }

    VARIANT_INLINE bool valid() const
    {
        return (type_index != detail::invalid_value);
    }

    template<typename T, typename... Args>
    VARIANT_INLINE void set(Args&&... args)
    {
        helper_type::destroy(type_index, &data);
        new (&data) T(std::forward<Args>(args)...);
        type_index = detail::direct_type<T, Types...>::index;
    }

    // get<T>()
    template<typename T, typename std::enable_if<(detail::direct_type<T, Types...>::index != detail::invalid_value)>::type* = nullptr>
    VARIANT_INLINE T& get()
    {
        if (type_index == detail::direct_type<T, Types...>::index)
        {
            return *reinterpret_cast<T*>(&data);
        }
        else
        {
            throw std::runtime_error("in get<T>()");
        }
    }

    template <typename T, typename std::enable_if<
                          (detail::direct_type<T, Types...>::index != detail::invalid_value)
                          >::type* = nullptr>
    VARIANT_INLINE T const& get() const
    {
        if (type_index == detail::direct_type<T, Types...>::index)
        {
            return *reinterpret_cast<T const*>(&data);
        }
        else
        {
            throw std::runtime_error("in get<T>()");
        }
    }

    // get<T>() - T stored as recursive_wrapper<T>
    template <typename T, typename std::enable_if<
                          (detail::direct_type<recursive_wrapper<T>, Types...>::index != detail::invalid_value)
                          >::type* = nullptr>
    VARIANT_INLINE T& get()
    {
        if (type_index == detail::direct_type<recursive_wrapper<T>, Types...>::index)
        {
            return (*reinterpret_cast<recursive_wrapper<T>*>(&data)).get();
        }
        else
        {
            throw std::runtime_error("in get<T>()");
        }
    }

    template <typename T,typename std::enable_if<
                         (detail::direct_type<recursive_wrapper<T>, Types...>::index != detail::invalid_value)
                         >::type* = nullptr>
    VARIANT_INLINE T const& get() const
    {
        if (type_index == detail::direct_type<recursive_wrapper<T>, Types...>::index)
        {
            return (*reinterpret_cast<recursive_wrapper<T> const*>(&data)).get();
        }
        else
        {
            throw std::runtime_error("in get<T>()");
        }
    }

    // get<T>() - T stored as std::reference_wrapper<T>
    template <typename T, typename std::enable_if<
                          (detail::direct_type<std::reference_wrapper<T>, Types...>::index != detail::invalid_value)
                          >::type* = nullptr>
    VARIANT_INLINE T& get()
    {
        if (type_index == detail::direct_type<std::reference_wrapper<T>, Types...>::index)
        {
            return (*reinterpret_cast<std::reference_wrapper<T>*>(&data)).get();
        }
        else
        {
            throw std::runtime_error("in get<T>()");
        }
    }

    template <typename T,typename std::enable_if<
                         (detail::direct_type<std::reference_wrapper<T const>, Types...>::index != detail::invalid_value)
                         >::type* = nullptr>
    VARIANT_INLINE T const& get() const
    {
        if (type_index == detail::direct_type<std::reference_wrapper<T const>, Types...>::index)
        {
            return (*reinterpret_cast<std::reference_wrapper<T const> const*>(&data)).get();
        }
        else
        {
            throw std::runtime_error("in get<T>()");
        }
    }

    VARIANT_INLINE std::size_t get_type_index() const
    {
        return type_index;
    }

    VARIANT_INLINE int which() const noexcept
    {
        return static_cast<int>(sizeof...(Types) - type_index - 1);
    }

    // visitor
    // unary
    template <typename F, typename V>
    auto VARIANT_INLINE
    static visit(V const& v, F f)
        -> decltype(detail::dispatcher<F, V,
                    typename detail::result_of_unary_visit<F,
                    typename detail::select_type<0, Types...>::type>::type, Types...>::apply_const(v, f))
    {
        using R = typename detail::result_of_unary_visit<F, typename detail::select_type<0, Types...>::type>::type;
        return detail::dispatcher<F, V, R, Types...>::apply_const(v, f);
    }
    // non-const
    template <typename F, typename V>
    auto VARIANT_INLINE
    static visit(V & v, F f)
        -> decltype(detail::dispatcher<F, V,
                    typename detail::result_of_unary_visit<F,
                    typename detail::select_type<0, Types...>::type>::type, Types...>::apply(v, f))
    {
        using R = typename detail::result_of_unary_visit<F, typename detail::select_type<0, Types...>::type>::type;
        return detail::dispatcher<F, V, R, Types...>::apply(v, f);
    }

    // binary
    // const
    template <typename F, typename V>
    auto VARIANT_INLINE
    static binary_visit(V const& v0, V const& v1, F f)
        -> decltype(detail::binary_dispatcher<F, V,
                    typename detail::result_of_binary_visit<F,
                    typename detail::select_type<0, Types...>::type>::type, Types...>::apply_const(v0, v1, f))
    {
        using R = typename detail::result_of_binary_visit<F,typename detail::select_type<0, Types...>::type>::type;
        return detail::binary_dispatcher<F, V, R, Types...>::apply_const(v0, v1, f);
    }
    // non-const
    template <typename F, typename V>
    auto VARIANT_INLINE
    static binary_visit(V& v0, V& v1, F f)
        -> decltype(detail::binary_dispatcher<F, V,
                    typename detail::result_of_binary_visit<F,
                    typename detail::select_type<0, Types...>::type>::type, Types...>::apply(v0, v1, f))
    {
        using R = typename detail::result_of_binary_visit<F,typename detail::select_type<0, Types...>::type>::type;
        return detail::binary_dispatcher<F, V, R, Types...>::apply(v0, v1, f);
    }

    ~variant() noexcept
    {
        helper_type::destroy(type_index, &data);
    }

    // comparison operators
    // equality
    VARIANT_INLINE bool operator==(variant const& rhs) const
    {
        if (this->get_type_index() != rhs.get_type_index())
            return false;
        detail::comparer<variant, detail::equal_comp> visitor(*this);
        return visit(rhs, visitor);
    }
    // less than
    VARIANT_INLINE bool operator<(variant const& rhs) const
    {
        if (this->get_type_index() != rhs.get_type_index())
        {
            return this->get_type_index() < rhs.get_type_index();
            // ^^ borrowed from boost::variant
        }
        detail::comparer<variant, detail::less_comp> visitor(*this);
        return visit(rhs, visitor);
    }
};

// unary visitor interface

// const
template <typename V, typename F>
auto VARIANT_INLINE static apply_visitor(F f, V const& v) -> decltype(V::visit(v, f))
{
    return V::visit(v, f);
}
// non-const
template <typename V, typename F>
auto VARIANT_INLINE static apply_visitor(F f, V & v) -> decltype(V::visit(v, f))
{
    return V::visit(v, f);
}

// binary visitor interface
// const
template <typename V, typename F>
auto VARIANT_INLINE static apply_visitor(F f, V const& v0, V const& v1) -> decltype(V::binary_visit(v0, v1, f))
{
    return V::binary_visit(v0, v1, f);
}
// non-const
template <typename V, typename F>
auto VARIANT_INLINE static apply_visitor(F f, V & v0, V & v1) -> decltype(V::binary_visit(v0, v1, f))
{
    return V::binary_visit(v0, v1, f);
}

// getter interface
template<typename ResultType, typename T>
ResultType & get(T & var)
{
    return var.template get<ResultType>();
}

template<typename ResultType, typename T>
ResultType const& get(T const& var)
{
    return var.template get<ResultType>();
}


}}

#endif  // MAPBOX_UTIL_VARIANT_HPP
