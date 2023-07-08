#ifndef MAPBOX_UTIL_OPTIONAL_HPP
#define MAPBOX_UTIL_OPTIONAL_HPP

#include <type_traits>

#include "variant.hpp"

namespace mapbox
{
namespace util
{

template <typename T> class optional
{
    static_assert(!std::is_reference<T>::value, "optional doesn't support references");

    struct none_type
    {
    };

    variant<none_type, T> variant_;

  public:
    optional() = default;

    optional(optional const &rhs)
    {
        if (this != &rhs)
        { // protect against invalid self-assignment
            variant_ = rhs.variant_;
        }
    }

    optional(T const &v) { variant_ = v; }

    explicit operator bool() const noexcept { return variant_.template is<T>(); }

    T const &get() const { return variant_.template get<T>(); }
    T &get() { return variant_.template get<T>(); }

    T const &operator*() const { return this->get(); }
    T operator*() { return this->get(); }

    optional &operator=(T const &v)
    {
        variant_ = v;
        return *this;
    }

    optional &operator=(optional const &rhs)
    {
        if (this != &rhs)
        {
            variant_ = rhs.variant_;
        }
        return *this;
    }

    template <typename... Args> void emplace(Args &&... args)
    {
        variant_ = T{std::forward<Args>(args)...};
    }

    void reset() { variant_ = none_type{}; }
};
}
}

#endif
