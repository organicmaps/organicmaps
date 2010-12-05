/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_PASS_CONTAINER_JANUARY_06_2009_0802PM)
#define SPIRIT_PASS_CONTAINER_JANUARY_06_2009_0802PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace spirit { namespace qi { namespace detail
{
    // has_same_elements: utility to check if the RHS attribute
    // is an STL container and that its value_type is convertible
    // to the LHS.

    template <typename LHS, typename RHSAttribute
      , bool IsContainer = traits::is_container<RHSAttribute>::value>
    struct has_same_elements : mpl::false_ {};

    template <typename LHS, typename RHSAttribute>
    struct has_same_elements<LHS, RHSAttribute, true>
      : is_convertible<typename RHSAttribute::value_type, LHS> {};

    template <typename LHS, typename T>
    struct has_same_elements<LHS, optional<T>, true>
      : has_same_elements<LHS, T> {};

#define BOOST_SPIRIT_IS_CONVERTIBLE(z, N, data)                               \
        has_same_elements<LHS, BOOST_PP_CAT(T, N)>::value ||                  \
    /***/

    // Note: variants are treated as containers if one of the held types is a
    //       container (see support/container.hpp).
    template <typename LHS, BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct has_same_elements<
            LHS, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, true>
      : mpl::bool_<BOOST_PP_REPEAT(BOOST_VARIANT_LIMIT_TYPES
          , BOOST_SPIRIT_IS_CONVERTIBLE, _) false> {};

#undef BOOST_SPIRIT_IS_CONVERTIBLE

    // This function handles the case where the attribute (Attr) given
    // the sequence is an STL container. This is a wrapper around F.
    // The function F does the actual parsing.
    template <typename F, typename Attr>
    struct pass_container
    {
        typedef typename F::context_type context_type;
        typedef typename F::iterator_type iterator_type;

        pass_container(F const& f, Attr& attr)
          : f(f), attr(attr) {}

        // this is for the case when the current element exposes an attribute
        // which is pushed back onto the container
        template <typename Component>
        bool dispatch_attribute_element(Component const& component, mpl::false_) const
        {
            // synthesized attribute needs to be default constructed
            typename traits::container_value<Attr>::type val =
                typename traits::container_value<Attr>::type();

            iterator_type save = f.first;
            bool r = f(component, val);
            if (!r)
            {
                // push the parsed value into our attribute
                r = !traits::push_back(attr, val);
                if (r)
                    f.first = save;
            }
            return r;
        }

        // this is for the case when the current element expects an attribute
        // which is a container itself, this element will push its data 
        // directly into the attribute container
        template <typename Component>
        bool dispatch_attribute_element(Component const& component, mpl::true_) const
        {
            return f(component, attr);
        }

        // This handles the distinction between elements in a sequence expecting
        // containers themselves and elements expecting non-containers as their 
        // attribute. Note: is_container treats optional<T>, where T is a 
        // container as a container as well.
        template <typename Component>
        bool dispatch_attribute(Component const& component, mpl::true_) const
        {
            typedef traits::is_container<
                typename traits::attribute_of<
                    Component, context_type, iterator_type
                >::type
            > predicate;

            return dispatch_attribute_element(component, predicate());
        }

        // this is for the case when the current element doesn't expect an 
        // attribute
        template <typename Component>
        bool dispatch_attribute(Component const& component, mpl::false_) const
        {
            return f(component, unused);
        }

        // This handles the case where the attribute of the component
        // is not an STL container or its element is not convertible
        // to the target attribute's (Attr) value_type.
        template <typename Component>
        bool dispatch_main(Component const& component, mpl::false_) const
        {
            // we need to dispatch again depending on the type of the attribute
            // of the current element (component). If this is has no attribute
            // we shouldn't push an element into the container.
            typedef traits::not_is_unused<
                typename traits::attribute_of<
                    Component, context_type, iterator_type
                >::type
            > predicate;

            return dispatch_attribute(component, predicate());
        }

        // This handles the case where the attribute of the component is
        // an STL container *and* its value_type is convertible to the
        // target attribute's (Attr) value_type.
        template <typename Component>
        bool dispatch_main(Component const& component, mpl::true_) const
        {
            return f(component, attr);
        }

        // Dispatches to dispatch_main depending on the attribute type
        // of the Component
        template <typename Component>
        bool operator()(Component const& component) const
        {
            typedef typename traits::container_value<Attr>::type lhs;
            typedef typename traits::attribute_of<
                Component, context_type, iterator_type>::type
            rhs_attribute;

            return dispatch_main(component
              , has_same_elements<lhs, rhs_attribute>());
        }

        F f;
        Attr& attr;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        pass_container& operator= (pass_container const&);
    };

    // Utility function to make a pass_container
    template <typename F, typename Attr>
    pass_container<F, Attr>
    inline make_pass_container(F const& f, Attr& attr)
    {
        return pass_container<F, Attr>(f, attr);
    }
}}}}

#endif
