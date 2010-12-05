//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_EXTRACT_FROM_SEP_30_2009_0732AM)
#define BOOST_SPIRIT_KARMA_EXTRACT_FROM_SEP_30_2009_0732AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/attributes_fwd.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>

#include <boost/ref.hpp>
#include <boost/optional.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  This file contains attribute extraction utilities. The utilities 
    //  provided also accept spirit's unused_type; all no-ops. Compiler 
    //  optimization will easily strip these away.
    ///////////////////////////////////////////////////////////////////////////

    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        // extract first and second element of a fusion sequence
        template <typename T>
        struct add_const_ref 
          : add_reference<typename add_const<T>::type> 
        {};

        template <typename T, int N>
        struct value_at_c 
          : add_const_ref<typename fusion::result_of::value_at_c<T, N>::type> 
        {};
    }

    // This is the default case: the plain attribute values
    template <typename Attribute, typename Exposed, typename Enable/*= void*/>
    struct extract_from_attribute
    {
        typedef typename traits::one_element_sequence<Attribute>::type 
            is_one_element_sequence;

        typedef typename mpl::eval_if<
            is_one_element_sequence
          , detail::value_at_c<Attribute, 0>
          , mpl::identity<Attribute const&>
        >::type type;

        template <typename Context>
        static type call(Attribute const& attr, Context&, mpl::false_)
        {
            return attr;
        }

        // This handles the case where the attribute is a single element fusion
        // sequence. We silently extract the only element and treat it as the 
        // attribute to generate output from.
        template <typename Context>
        static type call(Attribute const& attr, Context& ctx, mpl::true_)
        {
            return extract_from<Exposed>(fusion::at_c<0>(attr), ctx);
        }

        template <typename Context>
        static type call(Attribute const& attr, Context& ctx)
        {
            return call(attr, ctx, is_one_element_sequence());
        }
    };

    // This handles optional attributes. 
    template <typename Attribute, typename Exposed>
    struct extract_from_attribute<optional<Attribute>, Exposed>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(optional<Attribute> const& attr, Context& ctx)
        {
            return extract_from<Exposed>(boost::get<Attribute>(attr), ctx);
        }
    };

    template <typename Attribute, typename Exposed>
    struct extract_from_attribute<optional<Attribute const>, Exposed>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(optional<Attribute const> const& attr, Context& ctx)
        {
            return extract_from<Exposed>(boost::get<Attribute const>(attr), ctx);
        }
    };

    // This handles attributes wrapped inside a boost::ref(). 
    template <typename Attribute, typename Exposed>
    struct extract_from_attribute<reference_wrapper<Attribute>, Exposed>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(reference_wrapper<Attribute> const& attr, Context& ctx)
        {
            return extract_from<Exposed>(attr.get(), ctx);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Exposed, typename Attribute, typename Context>
    typename spirit::result_of::extract_from<Exposed, Attribute>::type
    extract_from(Attribute const& attr, Context& ctx
#if (defined(__GNUC__) && (__GNUC__ < 4)) || \
    (defined(__APPLE__) && defined(__INTEL_COMPILER))
      , typename enable_if<traits::not_is_unused<Attribute> >::type*
#endif
    )
    {
        return extract_from_attribute<Attribute, Exposed>::call(attr, ctx);
    }

    template <typename Exposed, typename Context>
    unused_type extract_from(unused_type, Context&)
    {
        return unused;
    }
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace result_of
{
    template <typename Exposed, typename Attribute>
    struct extract_from
      : traits::extract_from_attribute<Attribute, Exposed>
    {};

    template <typename Exposed>
    struct extract_from<Exposed, unused_type>
    {
        typedef unused_type type;
    };

    template <typename Exposed>
    struct extract_from<Exposed, unused_type const>
    {
        typedef unused_type type;
    };
}}}

#endif
