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
    template<typename N, typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_ , typename T10 = void_ , typename T11 = void_ , typename T12 = void_ , typename T13 = void_ , typename T14 = void_ , typename T15 = void_ , typename T16 = void_ , typename T17 = void_ , typename T18 = void_ , typename T19 = void_ , typename T20 = void_ , typename T21 = void_ , typename T22 = void_ , typename T23 = void_ , typename T24 = void_ , typename T25 = void_ , typename T26 = void_ , typename T27 = void_ , typename T28 = void_ , typename T29 = void_ , typename T30 = void_ , typename T31 = void_ , typename T32 = void_ , typename T33 = void_ , typename T34 = void_ , typename T35 = void_ , typename T36 = void_ , typename T37 = void_ , typename T38 = void_ , typename T39 = void_>
    struct deque_keyed_values_impl;
    template<typename N>
    struct deque_keyed_values_impl<N, void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
    template<typename N, typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28 , typename T29 , typename T30 , typename T31 , typename T32 , typename T33 , typename T34 , typename T35 , typename T36 , typename T37 , typename T38 , typename T39>
    struct deque_keyed_values_impl
    {
        typedef mpl::int_<mpl::plus<N, mpl::int_<1> >::value> next_index;
        typedef typename deque_keyed_values_impl<
            next_index,
            T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37 , T38 , T39>::type tail;
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
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33 , typename add_reference<typename add_const<T34 >::type>::type t34)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33 , t34));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33 , typename T_34>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33 , T_34 && t34)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33 , T_34
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33) , std::forward<T_34>(t34)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33 , typename add_reference<typename add_const<T34 >::type>::type t34 , typename add_reference<typename add_const<T35 >::type>::type t35)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33 , t34 , t35));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33 , typename T_34 , typename T_35>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33 , T_34 && t34 , T_35 && t35)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33 , T_34 , T_35
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33) , std::forward<T_34>(t34) , std::forward<T_35>(t35)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33 , typename add_reference<typename add_const<T34 >::type>::type t34 , typename add_reference<typename add_const<T35 >::type>::type t35 , typename add_reference<typename add_const<T36 >::type>::type t36)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33 , t34 , t35 , t36));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33 , typename T_34 , typename T_35 , typename T_36>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33 , T_34 && t34 , T_35 && t35 , T_36 && t36)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33 , T_34 , T_35 , T_36
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33) , std::forward<T_34>(t34) , std::forward<T_35>(t35) , std::forward<T_36>(t36)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33 , typename add_reference<typename add_const<T34 >::type>::type t34 , typename add_reference<typename add_const<T35 >::type>::type t35 , typename add_reference<typename add_const<T36 >::type>::type t36 , typename add_reference<typename add_const<T37 >::type>::type t37)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33 , t34 , t35 , t36 , t37));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33 , typename T_34 , typename T_35 , typename T_36 , typename T_37>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33 , T_34 && t34 , T_35 && t35 , T_36 && t36 , T_37 && t37)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33 , T_34 , T_35 , T_36 , T_37
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33) , std::forward<T_34>(t34) , std::forward<T_35>(t35) , std::forward<T_36>(t36) , std::forward<T_37>(t37)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33 , typename add_reference<typename add_const<T34 >::type>::type t34 , typename add_reference<typename add_const<T35 >::type>::type t35 , typename add_reference<typename add_const<T36 >::type>::type t36 , typename add_reference<typename add_const<T37 >::type>::type t37 , typename add_reference<typename add_const<T38 >::type>::type t38)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37 , T38
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33 , t34 , t35 , t36 , t37 , t38));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33 , typename T_34 , typename T_35 , typename T_36 , typename T_37 , typename T_38>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33 , T_34 && t34 , T_35 && t35 , T_36 && t36 , T_37 && t37 , T_38 && t38)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33 , T_34 , T_35 , T_36 , T_37 , T_38
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33) , std::forward<T_34>(t34) , std::forward<T_35>(t35) , std::forward<T_36>(t36) , std::forward<T_37>(t37) , std::forward<T_38>(t38)));
        }
# endif
        BOOST_FUSION_GPU_ENABLED
        static type construct(typename add_reference<typename add_const<T0 >::type>::type t0 , typename add_reference<typename add_const<T1 >::type>::type t1 , typename add_reference<typename add_const<T2 >::type>::type t2 , typename add_reference<typename add_const<T3 >::type>::type t3 , typename add_reference<typename add_const<T4 >::type>::type t4 , typename add_reference<typename add_const<T5 >::type>::type t5 , typename add_reference<typename add_const<T6 >::type>::type t6 , typename add_reference<typename add_const<T7 >::type>::type t7 , typename add_reference<typename add_const<T8 >::type>::type t8 , typename add_reference<typename add_const<T9 >::type>::type t9 , typename add_reference<typename add_const<T10 >::type>::type t10 , typename add_reference<typename add_const<T11 >::type>::type t11 , typename add_reference<typename add_const<T12 >::type>::type t12 , typename add_reference<typename add_const<T13 >::type>::type t13 , typename add_reference<typename add_const<T14 >::type>::type t14 , typename add_reference<typename add_const<T15 >::type>::type t15 , typename add_reference<typename add_const<T16 >::type>::type t16 , typename add_reference<typename add_const<T17 >::type>::type t17 , typename add_reference<typename add_const<T18 >::type>::type t18 , typename add_reference<typename add_const<T19 >::type>::type t19 , typename add_reference<typename add_const<T20 >::type>::type t20 , typename add_reference<typename add_const<T21 >::type>::type t21 , typename add_reference<typename add_const<T22 >::type>::type t22 , typename add_reference<typename add_const<T23 >::type>::type t23 , typename add_reference<typename add_const<T24 >::type>::type t24 , typename add_reference<typename add_const<T25 >::type>::type t25 , typename add_reference<typename add_const<T26 >::type>::type t26 , typename add_reference<typename add_const<T27 >::type>::type t27 , typename add_reference<typename add_const<T28 >::type>::type t28 , typename add_reference<typename add_const<T29 >::type>::type t29 , typename add_reference<typename add_const<T30 >::type>::type t30 , typename add_reference<typename add_const<T31 >::type>::type t31 , typename add_reference<typename add_const<T32 >::type>::type t32 , typename add_reference<typename add_const<T33 >::type>::type t33 , typename add_reference<typename add_const<T34 >::type>::type t34 , typename add_reference<typename add_const<T35 >::type>::type t35 , typename add_reference<typename add_const<T36 >::type>::type t36 , typename add_reference<typename add_const<T37 >::type>::type t37 , typename add_reference<typename add_const<T38 >::type>::type t38 , typename add_reference<typename add_const<T39 >::type>::type t39)
        {
            return type(t0,
                        deque_keyed_values_impl<
                        next_index
                        , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37 , T38 , T39
                        >::construct(t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9 , t10 , t11 , t12 , t13 , t14 , t15 , t16 , t17 , t18 , t19 , t20 , t21 , t22 , t23 , t24 , t25 , t26 , t27 , t28 , t29 , t30 , t31 , t32 , t33 , t34 , t35 , t36 , t37 , t38 , t39));
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename T_0 , typename T_1 , typename T_2 , typename T_3 , typename T_4 , typename T_5 , typename T_6 , typename T_7 , typename T_8 , typename T_9 , typename T_10 , typename T_11 , typename T_12 , typename T_13 , typename T_14 , typename T_15 , typename T_16 , typename T_17 , typename T_18 , typename T_19 , typename T_20 , typename T_21 , typename T_22 , typename T_23 , typename T_24 , typename T_25 , typename T_26 , typename T_27 , typename T_28 , typename T_29 , typename T_30 , typename T_31 , typename T_32 , typename T_33 , typename T_34 , typename T_35 , typename T_36 , typename T_37 , typename T_38 , typename T_39>
        BOOST_FUSION_GPU_ENABLED
        static type forward_(T_0 && t0 , T_1 && t1 , T_2 && t2 , T_3 && t3 , T_4 && t4 , T_5 && t5 , T_6 && t6 , T_7 && t7 , T_8 && t8 , T_9 && t9 , T_10 && t10 , T_11 && t11 , T_12 && t12 , T_13 && t13 , T_14 && t14 , T_15 && t15 , T_16 && t16 , T_17 && t17 , T_18 && t18 , T_19 && t19 , T_20 && t20 , T_21 && t21 , T_22 && t22 , T_23 && t23 , T_24 && t24 , T_25 && t25 , T_26 && t26 , T_27 && t27 , T_28 && t28 , T_29 && t29 , T_30 && t30 , T_31 && t31 , T_32 && t32 , T_33 && t33 , T_34 && t34 , T_35 && t35 , T_36 && t36 , T_37 && t37 , T_38 && t38 , T_39 && t39)
        {
            return type(std::forward<T_0>(t0),
                        deque_keyed_values_impl<
                        next_index
                        , T_1 , T_2 , T_3 , T_4 , T_5 , T_6 , T_7 , T_8 , T_9 , T_10 , T_11 , T_12 , T_13 , T_14 , T_15 , T_16 , T_17 , T_18 , T_19 , T_20 , T_21 , T_22 , T_23 , T_24 , T_25 , T_26 , T_27 , T_28 , T_29 , T_30 , T_31 , T_32 , T_33 , T_34 , T_35 , T_36 , T_37 , T_38 , T_39
                        >::forward_(std::forward<T_1>(t1) , std::forward<T_2>(t2) , std::forward<T_3>(t3) , std::forward<T_4>(t4) , std::forward<T_5>(t5) , std::forward<T_6>(t6) , std::forward<T_7>(t7) , std::forward<T_8>(t8) , std::forward<T_9>(t9) , std::forward<T_10>(t10) , std::forward<T_11>(t11) , std::forward<T_12>(t12) , std::forward<T_13>(t13) , std::forward<T_14>(t14) , std::forward<T_15>(t15) , std::forward<T_16>(t16) , std::forward<T_17>(t17) , std::forward<T_18>(t18) , std::forward<T_19>(t19) , std::forward<T_20>(t20) , std::forward<T_21>(t21) , std::forward<T_22>(t22) , std::forward<T_23>(t23) , std::forward<T_24>(t24) , std::forward<T_25>(t25) , std::forward<T_26>(t26) , std::forward<T_27>(t27) , std::forward<T_28>(t28) , std::forward<T_29>(t29) , std::forward<T_30>(t30) , std::forward<T_31>(t31) , std::forward<T_32>(t32) , std::forward<T_33>(t33) , std::forward<T_34>(t34) , std::forward<T_35>(t35) , std::forward<T_36>(t36) , std::forward<T_37>(t37) , std::forward<T_38>(t38) , std::forward<T_39>(t39)));
        }
# endif
    };
    template<typename T0 = void_ , typename T1 = void_ , typename T2 = void_ , typename T3 = void_ , typename T4 = void_ , typename T5 = void_ , typename T6 = void_ , typename T7 = void_ , typename T8 = void_ , typename T9 = void_ , typename T10 = void_ , typename T11 = void_ , typename T12 = void_ , typename T13 = void_ , typename T14 = void_ , typename T15 = void_ , typename T16 = void_ , typename T17 = void_ , typename T18 = void_ , typename T19 = void_ , typename T20 = void_ , typename T21 = void_ , typename T22 = void_ , typename T23 = void_ , typename T24 = void_ , typename T25 = void_ , typename T26 = void_ , typename T27 = void_ , typename T28 = void_ , typename T29 = void_ , typename T30 = void_ , typename T31 = void_ , typename T32 = void_ , typename T33 = void_ , typename T34 = void_ , typename T35 = void_ , typename T36 = void_ , typename T37 = void_ , typename T38 = void_ , typename T39 = void_>
    struct deque_keyed_values
        : deque_keyed_values_impl<mpl::int_<0>, T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37 , T38 , T39>
    {};
}}}
