/*=============================================================================
    Copyright (c) 2001-2009 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_CLASS_DETAIL_ADAPT_BASE_HPP
#define BOOST_FUSION_ADAPTED_CLASS_DETAIL_ADAPT_BASE_HPP

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>

//cschmidt: Spirit relies on Fusion defining class_member_proxy in the
//boost::fusion::extension namespace, with two nested types named lvalue and
//rvalue.

#define BOOST_FUSION_ADAPT_CLASS_GET_IDENTITY_TEMPLATE_IMPL(TEMPLATE_PARAMS_SEQ)\
    typename detail::get_identity<                                              \
        lvalue                                                                  \
      , BOOST_PP_SEQ_ELEM(1,TEMPLATE_PARAMS_SEQ)                                \
    >::type

#define BOOST_FUSION_ADAPT_CLASS_GET_IDENTITY_NON_TEMPLATE_IMPL(                \
    TEMPLATE_PARAMS_SEQ)                                                        \
                                                                                \
    lvalue

#define BOOST_FUSION_ADAPT_CLASS_C_BASE(\
    TEMPLATE_PARAMS_SEQ,NAME_SEQ,I,ATTRIBUTE,ATTRIBUTE_TUPEL_SIZE)              \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct class_member_proxy<                                                  \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)                         \
      , I                                                                       \
    >                                                                           \
    {                                                                           \
        typedef BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 0, ATTRIBUTE) lvalue; \
        typedef BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 1, ATTRIBUTE) rvalue; \
                                                                                \
        class_member_proxy(BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)& o)  \
          : obj(o)                                                              \
        {}                                                                      \
                                                                                \
        template<class Arg>                                                     \
        class_member_proxy&                                                     \
        operator=(Arg const& val)                                               \
        {                                                                       \
            BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 3, ATTRIBUTE);            \
            return *this;                                                       \
        }                                                                       \
                                                                                \
        operator lvalue()                                                       \
        {                                                                       \
            return BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 2, ATTRIBUTE);     \
        }                                                                       \
                                                                                \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)& obj;                   \
                                                                                \
    private:                                                                    \
        class_member_proxy& operator= (class_member_proxy const&);              \
    };                                                                          \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct struct_member<BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ), I>    \
    {                                                                           \
        typedef BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 0, ATTRIBUTE) lvalue; \
                                                                                \
        typedef                                                                 \
            BOOST_PP_IF(                                                        \
                BOOST_PP_SEQ_HEAD(TEMPLATE_PARAMS_SEQ),                         \
                BOOST_FUSION_ADAPT_CLASS_GET_IDENTITY_TEMPLATE_IMPL,            \
                BOOST_FUSION_ADAPT_CLASS_GET_IDENTITY_NON_TEMPLATE_IMPL)(       \
                    TEMPLATE_PARAMS_SEQ)                                        \
        type;                                                                   \
                                                                                \
        template<typename Seq>                                                  \
        struct apply                                                            \
        {                                                                       \
            typedef                                                             \
                class_member_proxy<                                             \
                    BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)             \
                  , I                                                           \
                >                                                               \
            proxy;                                                              \
                                                                                \
            typedef typename                                                    \
                mpl::if_<                                                       \
                    is_const<Seq>                                               \
                  , BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 1, ATTRIBUTE)     \
                  , proxy                                                       \
                >::type                                                         \
            type;                                                               \
                                                                                \
            static proxy                                                        \
            call(BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)& obj)          \
            {                                                                   \
                return proxy(obj);                                              \
            }                                                                   \
                                                                                \
            static BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 1, ATTRIBUTE)      \
            call(BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ) const& obj)    \
            {                                                                   \
                return BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPEL_SIZE, 2, ATTRIBUTE); \
            }                                                                   \
        };                                                                      \
    };

#endif
