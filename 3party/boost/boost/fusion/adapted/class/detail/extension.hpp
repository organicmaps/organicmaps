/*=============================================================================
    Copyright (c) 2001-2009 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_CLASS_DETAIL_EXTENSION_HPP
#define BOOST_FUSION_ADAPTED_CLASS_DETAIL_EXTENSION_HPP

#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace fusion
{
    namespace detail
    {
        template <typename T, typename Dummy>
        struct get_identity
          : remove_const<typename remove_reference<T>::type>
        {};
    }

    namespace extension
    {
        template <typename T, int N>
        struct class_member_proxy;
    }
}}

#endif
