/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_CLASS_ADAPT_ASSOC_CLASS_NAMED_HPP
#define BOOST_FUSION_ADAPTED_CLASS_ADAPT_ASSOC_CLASS_NAMED_HPP

#include <boost/fusion/adapted/class/adapt_assoc_class.hpp>
#include <boost/fusion/adapted/struct/detail/proxy_type.hpp>

#define BOOST_FUSION_ADAPT_ASSOC_CLASS_NAMED_NS(                                \
    WRAPPED_TYPE, NAMESPACE_SEQ, NAME, ATTRIBUTES)                              \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_DEFINE_PROXY_TYPE_IMPL(                           \
        WRAPPED_TYPE,(0)NAMESPACE_SEQ,NAME)                                     \
                                                                                \
    BOOST_FUSION_ADAPT_ASSOC_CLASS_AS_VIEW(                                     \
        BOOST_FUSION_ADAPT_STRUCT_NAMESPACE_DECLARATION((0)NAMESPACE_SEQ)NAME,  \
        ATTRIBUTES)

#define BOOST_FUSION_ADAPT_ASSOC_CLASS_NAMED(WRAPPED_TYPE, NAME, ATTRIBUTES)    \
    BOOST_FUSION_ADAPT_ASSOC_CLASS_NAMED_NS(                                    \
        WRAPPED_TYPE,(boost)(fusion)(adapted),NAME,ATTRIBUTES)

#endif
