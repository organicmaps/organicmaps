/*=============================================================================
    Copyright (c) 2005 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_LIST_07172005_1153)
#define FUSION_LIST_07172005_1153

#include <boost/fusion/container/list/list_fwd.hpp>
#include <boost/fusion/container/list/detail/list_to_cons.hpp>

namespace boost { namespace fusion
{
    struct nil;
    struct void_;

    template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename T)>
    struct list 
        : detail::list_to_cons<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, T)>::type
    {
    private:
        typedef 
            detail::list_to_cons<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, T)>
        list_to_cons;

    public:
        typedef typename list_to_cons::type inherited_type;

        list()
            : inherited_type() {}

        template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename U)>
        list(list<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, U)> const& rhs)
            : inherited_type(rhs) {}

        template <typename Sequence>
        list(Sequence const& rhs)
            : inherited_type(rhs) {}

        //  Expand a couple of forwarding constructors for arguments
        //  of type (T0), (T0, T1), (T0, T1, T2) etc. Exanple:
        //
        //  list(
        //      typename detail::call_param<T0>::type _0
        //    , typename detail::call_param<T1>::type _1)
        //    : inherited_type(list_to_cons::call(_0, _1)) {}
        #include <boost/fusion/container/list/detail/list_forward_ctor.hpp>

        template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename U)>
        list&
        operator=(list<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, U)> const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }

        template <typename T>
        list&
        operator=(T const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }
    };
}}

#endif
