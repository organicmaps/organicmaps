//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ADAPT_ADT_ATTRIBUTES_SEP_15_2010_1219PM)
#define BOOST_SPIRIT_ADAPT_ADT_ATTRIBUTES_SEP_15_2010_1219PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/utility/enable_if.hpp>

///////////////////////////////////////////////////////////////////////////////
// customization points allowing to use adapted classes with spirit
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, bool Const>
    struct is_container<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : is_container<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
        >
    {};

    template <typename T, int N, bool Const>
    struct container_value<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : container_value<
            typename fusion::extension::adt_attribute_proxy<
                T, N, Const
            >::type
        >
    {};

    template <typename T, int N, typename Val>
    struct push_back_container<
        fusion::extension::adt_attribute_proxy<T, N, false>
      , Val
      , typename enable_if<is_reference<
            typename fusion::extension::adt_attribute_proxy<T, N, false>::type
        > >::type>
    {
        static bool call(
            fusion::extension::adt_attribute_proxy<T, N, false>& p
          , Val const& val)
        {
            typedef typename 
                fusion::extension::adt_attribute_proxy<T, N, false>::type
            type;
            return push_back(type(p), val);
        }
    };

    template <typename T, int N, bool Const>
    struct container_iterator<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : container_iterator<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
        >
    {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, typename Val>
    struct assign_to_attribute_from_value<
        fusion::extension::adt_attribute_proxy<T, N, false>
      , Val>
    {
        static void 
        call(Val const& val
          , fusion::extension::adt_attribute_proxy<T, N, false>& attr)
        {
            attr = val;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, bool Const>
    struct attribute_type<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : fusion::extension::adt_attribute_proxy<T, N, Const>
    {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, typename Attribute, typename Domain>
    struct transform_attribute<
        fusion::extension::adt_attribute_proxy<T, N, false>
      , Attribute
      , Domain
      , typename disable_if<is_reference<
            typename fusion::extension::adt_attribute_proxy<T, N, false>::type
        > >::type>
    {
        typedef Attribute type;

        static Attribute 
        pre(fusion::extension::adt_attribute_proxy<T, N, false>& val)
        { 
            return val; 
        }
        static void 
        post(
            fusion::extension::adt_attribute_proxy<T, N, false>& val
          , Attribute const& attr) 
        {
            val = attr;
        }
        static void 
        fail(fusion::extension::adt_attribute_proxy<T, N, false>&)
        {
        }
    };

    template <
        typename T, int N, bool Const, typename Attribute, typename Domain>
    struct transform_attribute<
        fusion::extension::adt_attribute_proxy<T, N, Const>
      , Attribute
      , Domain
      , typename enable_if<is_reference<
            typename fusion::extension::adt_attribute_proxy<
                T, N, Const
            >::type
        > >::type>
    {
        typedef Attribute& type;

        static Attribute& 
        pre(fusion::extension::adt_attribute_proxy<T, N, Const>& val)
        { 
            return val; 
        }
        static void 
        post(
            fusion::extension::adt_attribute_proxy<T, N, Const>&
          , Attribute const&)
        {
        }
        static void 
        fail(fusion::extension::adt_attribute_proxy<T, N, Const>&)
        {
        }
    };

    template <typename T, int N, bool Const>
    struct clear_value<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        static void call(
            fusion::extension::adt_attribute_proxy<T, N, Const>& val)
        {
            typedef typename 
                fusion::extension::adt_attribute_proxy<T, N, Const>::type
            type;
            clear(type(val));
        }
    };
}}}

#endif
