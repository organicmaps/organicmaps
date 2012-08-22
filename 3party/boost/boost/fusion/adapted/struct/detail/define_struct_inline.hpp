/*=============================================================================
    Copyright (c) 2012 Nathan Ridge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DETAIL_DEFINE_STRUCT_INLINE_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DETAIL_DEFINE_STRUCT_INLINE_HPP

#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/sequence/sequence_facade.hpp>
#include <boost/fusion/iterator/iterator_facade.hpp>
#include <boost/fusion/algorithm/auxiliary/copy.hpp>
#include <boost/fusion/adapted/struct/detail/define_struct.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#define BOOST_FUSION_MAKE_DEFAULT_INIT_LIST_ENTRY(R, DATA, N, ATTRIBUTE)        \
    BOOST_PP_COMMA_IF(N) BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE)()

#define BOOST_FUSION_MAKE_DEFAULT_INIT_LIST(ATTRIBUTES_SEQ)                     \
            : BOOST_PP_SEQ_FOR_EACH_I(                                          \
              BOOST_FUSION_MAKE_DEFAULT_INIT_LIST_ENTRY,                        \
              ~,                                                                \
              ATTRIBUTES_SEQ)                                                   \

#define BOOST_FUSION_IGNORE_1(ARG1)
#define BOOST_FUSION_IGNORE_2(ARG1, ARG2)

#define BOOST_FUSION_MAKE_COPY_CONSTRUCTOR(NAME, ATTRIBUTES_SEQ)                \
    NAME(BOOST_PP_SEQ_FOR_EACH_I(                                               \
            BOOST_FUSION_MAKE_CONST_REF_PARAM,                                  \
            ~,                                                                  \
            ATTRIBUTES_SEQ))                                                    \
        : BOOST_PP_SEQ_FOR_EACH_I(                                              \
              BOOST_FUSION_MAKE_INIT_LIST_ENTRY,                                \
              ~,                                                                \
              ATTRIBUTES_SEQ)                                                   \
    {                                                                           \
    }                                                                           \

#define BOOST_FUSION_MAKE_CONST_REF_PARAM(R, DATA, N, ATTRIBUTE)                \
    BOOST_PP_COMMA_IF(N)                                                        \
    BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE) const&                                 \
    BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE)

#define BOOST_FUSION_MAKE_INIT_LIST_ENTRY_I(NAME) NAME(NAME)

#define BOOST_FUSION_MAKE_INIT_LIST_ENTRY(R, DATA, N, ATTRIBUTE)                \
    BOOST_PP_COMMA_IF(N)                                                        \
    BOOST_FUSION_MAKE_INIT_LIST_ENTRY_I(BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE))

// Note: all template parameter names need to be uglified, otherwise they might
//       shadow a template parameter of the struct when used with
//       BOOST_FUSION_DEFINE_TPL_STRUCT_INLINE

#define BOOST_FUSION_MAKE_ITERATOR_VALUE_OF_SPECS(Z, N, NAME)                   \
    template <typename boost_fusion_uglified_Sq>                                \
    struct value_of<NAME##_iterator<boost_fusion_uglified_Sq, N> >              \
        : boost::mpl::identity<                                                 \
              typename boost_fusion_uglified_Sq::t##N##_type                    \
          >                                                                     \
    {                                                                           \
    };

#define BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                  \
    SPEC_TYPE, CALL_ARG_TYPE, TYPE_QUAL, ATTRIBUTE, N)                          \
                                                                                \
    template <typename boost_fusion_uglified_Sq>                                \
    struct deref<SPEC_TYPE, N> >                                                \
    {                                                                           \
        typedef typename boost_fusion_uglified_Sq::t##N##_type TYPE_QUAL& type; \
        static type call(CALL_ARG_TYPE, N> const& iter)                         \
        {                                                                       \
            return iter.seq_.BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE);              \
        }                                                                       \
    };

#define BOOST_FUSION_MAKE_ITERATOR_DEREF_SPECS(R, NAME, N, ATTRIBUTE)           \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        BOOST_PP_CAT(NAME, _iterator)<boost_fusion_uglified_Sq,                 \
        BOOST_PP_CAT(NAME, _iterator)<boost_fusion_uglified_Sq,                 \
        ,                                                                       \
        ATTRIBUTE,                                                              \
        N)                                                                      \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        BOOST_PP_CAT(NAME, _iterator)<const boost_fusion_uglified_Sq,           \
        BOOST_PP_CAT(NAME, _iterator)<const boost_fusion_uglified_Sq,           \
        const,                                                                  \
        ATTRIBUTE,                                                              \
        N)                                                                      \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        const BOOST_PP_CAT(NAME, _iterator)<boost_fusion_uglified_Sq,           \
        BOOST_PP_CAT(NAME, _iterator)<boost_fusion_uglified_Sq,                 \
        ,                                                                       \
        ATTRIBUTE,                                                              \
        N)                                                                      \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        const BOOST_PP_CAT(NAME, _iterator)<const boost_fusion_uglified_Sq,     \
        BOOST_PP_CAT(NAME, _iterator)<const boost_fusion_uglified_Sq,           \
        const,                                                                  \
        ATTRIBUTE,                                                              \
        N)                                                                      \

#define BOOST_FUSION_MAKE_VALUE_AT_SPECS(Z, N, DATA)                            \
    template <typename boost_fusion_uglified_Sq>                                \
    struct value_at<boost_fusion_uglified_Sq, boost::mpl::int_<N> >             \
    {                                                                           \
        typedef typename boost_fusion_uglified_Sq::t##N##_type type;            \
    };

#define BOOST_FUSION_MAKE_AT_SPECS(R, DATA, N, ATTRIBUTE)                       \
    template <typename boost_fusion_uglified_Sq>                                \
    struct at<boost_fusion_uglified_Sq, boost::mpl::int_<N> >                   \
    {                                                                           \
        typedef typename boost::mpl::if_<                                       \
            boost::is_const<boost_fusion_uglified_Sq>,                          \
            typename boost_fusion_uglified_Sq::t##N##_type const&,              \
            typename boost_fusion_uglified_Sq::t##N##_type&                     \
        >::type type;                                                           \
                                                                                \
        static type call(boost_fusion_uglified_Sq& sq)                          \
        {                                                                       \
            return sq. BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE);                    \
        }                                                                       \
    };

#define BOOST_FUSION_MAKE_TYPEDEF(R, DATA, N, ATTRIBUTE)                        \
    typedef BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE) t##N##_type;

#define BOOST_FUSION_MAKE_DATA_MEMBER(R, DATA, N, ATTRIBUTE)                    \
    BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE) BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE);

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_IMPL(NAME, ATTRIBUTES)                \
    struct NAME : boost::fusion::sequence_facade<                               \
                      NAME,                                                     \
                      boost::fusion::random_access_traversal_tag                \
                  >                                                             \
    {                                                                           \
        BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS(NAME, ATTRIBUTES)             \
    };

#define BOOST_FUSION_DEFINE_TPL_STRUCT_INLINE_IMPL(                             \
    TEMPLATE_PARAMS_SEQ, NAME, ATTRIBUTES)                                      \
                                                                                \
    template <                                                                  \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS_IMPL(                  \
            (0)TEMPLATE_PARAMS_SEQ)                                             \
    >                                                                           \
    struct NAME : boost::fusion::sequence_facade<                               \
                      NAME<                                                     \
                          BOOST_PP_SEQ_ENUM(TEMPLATE_PARAMS_SEQ)                \
                      >,                                                        \
                      boost::fusion::random_access_traversal_tag                \
                  >                                                             \
    {                                                                           \
        BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS(NAME, ATTRIBUTES)             \
    };

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS(NAME, ATTRIBUTES)             \
    BOOST_FUSION_DEFINE_STRUCT_MEMBERS_IMPL(                                    \
        NAME,                                                                   \
        BOOST_PP_CAT(BOOST_FUSION_ADAPT_STRUCT_FILLER_0 ATTRIBUTES,_END))

#define BOOST_FUSION_DEFINE_STRUCT_MEMBERS_IMPL(NAME, ATTRIBUTES_SEQ)           \
    BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS_IMPL_IMPL(                        \
        NAME,                                                                   \
        ATTRIBUTES_SEQ,                                                         \
        BOOST_PP_SEQ_SIZE(ATTRIBUTES_SEQ))

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS_IMPL_IMPL(                    \
    NAME, ATTRIBUTES_SEQ, ATTRIBUTES_SEQ_SIZE)                                  \
                                                                                \
    NAME()                                                                      \
        BOOST_PP_IF(                                                            \
            BOOST_PP_SEQ_SIZE(ATTRIBUTES_SEQ),                                  \
            BOOST_FUSION_MAKE_DEFAULT_INIT_LIST,                                \
            BOOST_FUSION_IGNORE_1)                                              \
                (ATTRIBUTES_SEQ)                                                \
    {                                                                           \
    }                                                                           \
                                                                                \
    BOOST_PP_IF(                                                                \
        BOOST_PP_SEQ_SIZE(ATTRIBUTES_SEQ),                                      \
        BOOST_FUSION_MAKE_COPY_CONSTRUCTOR,                                     \
        BOOST_FUSION_IGNORE_2)                                                  \
            (NAME, ATTRIBUTES_SEQ)                                              \
                                                                                \
    template <typename boost_fusion_uglified_Seq>                               \
    NAME(const boost_fusion_uglified_Seq& rhs)                                  \
    {                                                                           \
        boost::fusion::copy(rhs, *this);                                        \
    }                                                                           \
                                                                                \
    template <typename boost_fusion_uglified_Seq>                               \
    NAME& operator=(const boost_fusion_uglified_Seq& rhs)                       \
    {                                                                           \
        boost::fusion::copy(rhs, *this);                                        \
        return *this;                                                           \
    }                                                                           \
                                                                                \
    template <typename boost_fusion_uglified_Seq, int N>                        \
    struct NAME##_iterator                                                      \
        : boost::fusion::iterator_facade<                                       \
              NAME##_iterator<boost_fusion_uglified_Seq, N>,                    \
              boost::fusion::random_access_traversal_tag                        \
          >                                                                     \
    {                                                                           \
        typedef boost::mpl::int_<N> index;                                      \
        typedef boost_fusion_uglified_Seq sequence_type;                        \
                                                                                \
        NAME##_iterator(boost_fusion_uglified_Seq& seq) : seq_(seq) {}          \
                                                                                \
        boost_fusion_uglified_Seq& seq_;                                        \
                                                                                \
        template <typename boost_fusion_uglified_T> struct value_of;            \
        BOOST_PP_REPEAT(                                                        \
            ATTRIBUTES_SEQ_SIZE,                                                \
            BOOST_FUSION_MAKE_ITERATOR_VALUE_OF_SPECS,                          \
            NAME)                                                               \
                                                                                \
        template <typename boost_fusion_uglified_T> struct deref;               \
        BOOST_PP_SEQ_FOR_EACH_I(                                                \
            BOOST_FUSION_MAKE_ITERATOR_DEREF_SPECS,                             \
            NAME,                                                               \
            ATTRIBUTES_SEQ)                                                     \
                                                                                \
        template <typename boost_fusion_uglified_It>                            \
        struct next                                                             \
        {                                                                       \
            typedef NAME##_iterator<                                            \
                typename boost_fusion_uglified_It::sequence_type,               \
                boost_fusion_uglified_It::index::value + 1                      \
            > type;                                                             \
                                                                                \
            static type call(boost_fusion_uglified_It const& it)                \
            {                                                                   \
                return type(it.seq_);                                           \
            }                                                                   \
        };                                                                      \
                                                                                \
        template <typename boost_fusion_uglified_It>                            \
        struct prior                                                            \
        {                                                                       \
            typedef NAME##_iterator<                                            \
                typename boost_fusion_uglified_It::sequence_type,               \
                boost_fusion_uglified_It::index::value - 1                      \
            > type;                                                             \
                                                                                \
            static type call(boost_fusion_uglified_It const& it)                \
            {                                                                   \
                return type(it.seq_);                                           \
            }                                                                   \
        };                                                                      \
                                                                                \
        template <                                                              \
            typename boost_fusion_uglified_It1,                                 \
            typename boost_fusion_uglified_It2                                  \
        >                                                                       \
        struct distance                                                         \
        {                                                                       \
            typedef typename boost::mpl::minus<                                 \
                typename boost_fusion_uglified_It2::index,                      \
                typename boost_fusion_uglified_It1::index                       \
            >::type type;                                                       \
                                                                                \
             static type call(boost_fusion_uglified_It1 const& it1,             \
                              boost_fusion_uglified_It2 const& it2)             \
            {                                                                   \
                return type();                                                  \
            }                                                                   \
        };                                                                      \
                                                                                \
        template <                                                              \
            typename boost_fusion_uglified_It,                                  \
            typename boost_fusion_uglified_M                                    \
        >                                                                       \
        struct advance                                                          \
        {                                                                       \
            typedef NAME##_iterator<                                            \
                typename boost_fusion_uglified_It::sequence_type,               \
                boost_fusion_uglified_It::index::value                          \
                    + boost_fusion_uglified_M::value                            \
            > type;                                                             \
                                                                                \
            static type call(boost_fusion_uglified_It const& it)                \
            {                                                                   \
                return type(it.seq_);                                           \
            }                                                                   \
        };                                                                      \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_uglified_Sq>                                \
    struct begin                                                                \
    {                                                                           \
        typedef NAME##_iterator<boost_fusion_uglified_Sq, 0> type;              \
                                                                                \
        static type call(boost_fusion_uglified_Sq& sq)                          \
        {                                                                       \
            return type(sq);                                                    \
        }                                                                       \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_uglified_Sq>                                \
    struct end                                                                  \
    {                                                                           \
        typedef NAME##_iterator<                                                \
            boost_fusion_uglified_Sq,                                           \
            ATTRIBUTES_SEQ_SIZE                                                 \
        > type;                                                                 \
                                                                                \
        static type call(boost_fusion_uglified_Sq& sq)                          \
        {                                                                       \
            return type(sq);                                                    \
        }                                                                       \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_uglified_Sq>                                \
    struct size : boost::mpl::int_<ATTRIBUTES_SEQ_SIZE>                         \
    {                                                                           \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_uglified_Sq>                                \
    struct empty : boost::mpl::bool_<ATTRIBUTES_SEQ_SIZE == 0>                  \
    {                                                                           \
    };                                                                          \
                                                                                \
    template <                                                                  \
        typename boost_fusion_uglified_Sq,                                      \
        typename boost_fusion_uglified_N                                        \
    >                                                                           \
    struct value_at : value_at<                                                 \
                          boost_fusion_uglified_Sq,                             \
                          boost::mpl::int_<boost_fusion_uglified_N::value>      \
                      >                                                         \
    {                                                                           \
    };                                                                          \
                                                                                \
    BOOST_PP_REPEAT(                                                            \
        ATTRIBUTES_SEQ_SIZE,                                                    \
        BOOST_FUSION_MAKE_VALUE_AT_SPECS,                                       \
        ~)                                                                      \
                                                                                \
    template <                                                                  \
        typename boost_fusion_uglified_Sq,                                      \
        typename boost_fusion_uglified_N                                        \
    >                                                                           \
    struct at : at<                                                             \
                    boost_fusion_uglified_Sq,                                   \
                    boost::mpl::int_<boost_fusion_uglified_N::value>            \
                >                                                               \
    {                                                                           \
    };                                                                          \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_FUSION_MAKE_AT_SPECS, ~, ATTRIBUTES_SEQ)      \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_FUSION_MAKE_TYPEDEF, ~, ATTRIBUTES_SEQ)       \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(                                                    \
        BOOST_FUSION_MAKE_DATA_MEMBER,                                          \
        ~,                                                                      \
        ATTRIBUTES_SEQ)

#endif
