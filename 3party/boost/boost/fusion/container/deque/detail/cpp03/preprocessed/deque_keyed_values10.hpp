/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion { namespace detail
{
    template<typename Key, typename Value, typename Rest>
    struct keyed_element;
    struct nil_keyed_element;
    template<typename N, typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_>
    struct deque_keyed_values_impl;
    template<typename N>
    struct deque_keyed_values_impl<N, void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
    {
        typedef nil_keyed_element type;
        BOOST_FUSION_GPU_ENABLED
        static type construct()
        {
            return type();
        }
        BOOST_FUSION_GPU_ENABLED
        static type forward_()
        {
            return type();
        }
    };
    template<typename N, typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct deque_keyed_values_impl
    {
        typedef mpl::int_<mpl::plus<N, mpl::int_<1> >::value> next_index;
        typedef typename deque_keyed_values_impl<
            next_index,
            T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>::type tail;
        typedef keyed_element<N, T0, tail> type;
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        >::construct());
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        >::forward_());
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1
                        >::construct(t1));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1
                        >::forward_(std::forward<T_1>(t1)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2
                        >::construct(t1 , t2));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3
                        >::construct(t1 , t2 , t3));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4
                        >::construct(t1 , t2 , t3 , t4));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5
                        >::construct(t1 , t2 , t3 , t4 , t5));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9)));
        }
# endif
    };
    template<typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_>
    struct deque_keyed_values
        : deque_keyed_values_impl<mpl::int_<0>, T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9>
    {};
}}}
