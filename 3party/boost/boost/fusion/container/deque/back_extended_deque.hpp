/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_BACK_EXTENDED_DEQUE_26112006_2209)
#define BOOST_FUSION_BACK_EXTENDED_DEQUE_26112006_2209

#include <boost/fusion/container/deque/detail/keyed_element.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/support/sequence_base.hpp>

#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>

namespace boost { namespace fusion {
    template<typename Deque, typename T>
    struct back_extended_deque
        : detail::keyed_element<typename Deque::next_up, T, Deque>,
          sequence_base<back_extended_deque<Deque, T> >
    {
        typedef detail::keyed_element<typename Deque::next_up, T, Deque> base;
        typedef typename Deque::next_down next_down;
        typedef mpl::int_<mpl::plus<typename Deque::next_up, mpl::int_<1> >::value> next_up;
        typedef mpl::plus<typename result_of::size<Deque>::type, mpl::int_<1> > size;

        back_extended_deque(Deque const& deque, typename add_reference<typename add_const<T>::type>::type t)
            : base(t, deque)
        {}
    };
}}

#endif
