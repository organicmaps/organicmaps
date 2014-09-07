//-----------------------------------------------------------------------------
// boost variant/polymorphic_get.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2013 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_POLYMORPHIC_GET_HPP
#define BOOST_VARIANT_POLYMORPHIC_GET_HPP

#include <exception>

#include "boost/config.hpp"
#include "boost/detail/workaround.hpp"
#include "boost/throw_exception.hpp"
#include "boost/utility/addressof.hpp"
#include "boost/variant/variant_fwd.hpp"
#include "boost/variant/get.hpp"

#include "boost/type_traits/add_reference.hpp"
#include "boost/type_traits/add_pointer.hpp"
#include "boost/type_traits/is_base_of.hpp"

namespace boost {

//////////////////////////////////////////////////////////////////////////
// class bad_polymorphic_get
//
// The exception thrown in the event of a failed get of a value.
//
class BOOST_SYMBOL_VISIBLE bad_polymorphic_get
    : public bad_get
{
public: // std::exception implementation

    virtual const char * what() const BOOST_NOEXCEPT_OR_NOTHROW
    {
        return "boost::bad_polymorphic_get: "
               "failed value get using boost::polymorphic_get";
    }

};

//////////////////////////////////////////////////////////////////////////
// function template get<T>
//
// Retrieves content of given variant object if content is of type T.
// Otherwise: pointer ver. returns 0; reference ver. throws bad_get.
//

namespace detail { namespace variant {

// (detail) class template get_polymorphic_visitor
//
// Generic static visitor that: if the value is of the specified
// type or of a type derived from specified, returns a pointer
// to the value it visits; else a null pointer.
//
template <typename Base>
struct get_polymorphic_visitor
{
private: // private typedefs
    typedef typename add_pointer<Base>::type pointer;
    typedef typename add_reference<Base>::type reference;

    pointer get(reference operand, boost::true_type) const BOOST_NOEXCEPT
    {
        return boost::addressof(operand);
    }

    template <class T>
    pointer get(T&, boost::false_type) const BOOST_NOEXCEPT
    {
        return static_cast<pointer>(0);
    }

public: // visitor interfaces
    typedef pointer result_type;

    template <typename U>
    pointer operator()(U& operand) const BOOST_NOEXCEPT
    {
        typedef boost::integral_constant<
            bool,
            boost::is_base_of<Base, U>::value || boost::is_same<Base, U>::value
        > tag_t;
        return get(operand, tag_t());
    }
};

}} // namespace detail::variant

#ifndef BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE
#   if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0551))
#       define BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE(t)
#   else
#       define BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE(t)  \
        , t* = 0
#   endif
#endif

template <typename U, BOOST_VARIANT_ENUM_PARAMS(typename T) >
inline
    typename add_pointer<U>::type
polymorphic_get(
      boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >* operand
      BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE(U)
    ) BOOST_NOEXCEPT
{
    typedef typename add_pointer<U>::type U_ptr;
    if (!operand) return static_cast<U_ptr>(0);

    detail::variant::get_polymorphic_visitor<U> v;
    return operand->apply_visitor(v);
}

template <typename U, BOOST_VARIANT_ENUM_PARAMS(typename T) >
inline
    typename add_pointer<const U>::type
polymorphic_get(
      const boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >* operand
      BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE(U)
    ) BOOST_NOEXCEPT
{
    typedef typename add_pointer<const U>::type U_ptr;
    if (!operand) return static_cast<U_ptr>(0);

    detail::variant::get_polymorphic_visitor<const U> v;
    return operand->apply_visitor(v);
}

template <typename U, BOOST_VARIANT_ENUM_PARAMS(typename T) >
inline
    typename add_reference<U>::type
polymorphic_get(
      boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >& operand
      BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE(U)
    )
{
    typedef typename add_pointer<U>::type U_ptr;
    U_ptr result = polymorphic_get<U>(&operand);

    if (!result)
        boost::throw_exception(bad_polymorphic_get());
    return *result;
}

template <typename U, BOOST_VARIANT_ENUM_PARAMS(typename T) >
inline
    typename add_reference<const U>::type
polymorphic_get(
      const boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >& operand
      BOOST_VARIANT_AUX_GET_EXPLICIT_TEMPLATE_TYPE(U)
    )
{
    typedef typename add_pointer<const U>::type U_ptr;
    U_ptr result = polymorphic_get<const U>(&operand);

    if (!result)
        boost::throw_exception(bad_polymorphic_get());
    return *result;
}

} // namespace boost

#endif // BOOST_VARIANT_POLYMORPHIC_GET_HPP
