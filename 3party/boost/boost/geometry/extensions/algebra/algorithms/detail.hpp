// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_DETAIL_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_DETAIL_HPP

// TODO - for multiplication of coordinates
// if coordinate_type is_integral - use double as the result type

#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace algebra {

// Cross 3D of 3 components of Vectors/Quaternion starting from IS1 and IS2 and placing the result starting from D
template <std::size_t IS1, std::size_t IS2, std::size_t ID, typename S1, typename S2, typename D>
inline void cross(S1 const& s1, S2 const& s2, D & d)
{
    set<ID+0>(d, get<IS1+1>(s1)*get<IS2+2>(s2) - get<IS1+2>(s1)*get<IS2+1>(s2));
    set<ID+1>(d, get<IS1+2>(s1)*get<IS2+0>(s2) - get<IS1+0>(s1)*get<IS2+2>(s2));
    set<ID+2>(d, get<IS1+0>(s1)*get<IS2+1>(s2) - get<IS1+1>(s1)*get<IS2+0>(s2));
}

// Dot of N components of Vectors/Quaternion starting from IS1 and IS2

template <typename S1, typename S2, std::size_t IS1, std::size_t IS2, std::size_t N>
struct dot_impl
{
    BOOST_STATIC_ASSERT(0 < N);

    typedef typename geometry::select_most_precise<
        typename traits::coordinate_type<S1>::type,
        typename traits::coordinate_type<S2>::type
    >::type coordinate_type;

    static inline coordinate_type apply(S1 const& s1, S2 const& s2)
    {
        return get<IS1>(s1)*get<IS2>(s2) + dot_impl<S1, S2, IS1+1, IS2+1, N-1>::apply(s1, s2);
    }
};

template <typename S1, typename S2, std::size_t IS1, std::size_t IS2>
struct dot_impl<S1, S2, IS1, IS2, 1>
{
    typedef typename geometry::select_most_precise<
        typename traits::coordinate_type<S1>::type,
        typename traits::coordinate_type<S2>::type
    >::type coordinate_type;

    static inline coordinate_type apply(S1 const& s1, S2 const& s2)
    {
        return get<IS1>(s1)*get<IS2>(s2);
    }
};

template <std::size_t IS1, std::size_t IS2, std::size_t N, typename S1, typename S2>
inline static
typename geometry::select_most_precise<
    typename traits::coordinate_type<S1>::type,
    typename traits::coordinate_type<S2>::type
>::type
dot(S1 const& s1, S2 const& s2)
{
    return dot_impl<S1, S2, IS1, IS2, N>::apply(s1, s2);
}

// Multiplication of N components starting from Ith by Value v

template <typename S, typename T, std::size_t IS, std::size_t I, std::size_t N>
struct mul_impl
{
    BOOST_STATIC_ASSERT(0 < N);

    static inline void apply(S & s, T const& v)
    {
        set<IS>(s, get<IS>(s) * v);
        mul_impl<S, T, IS+1, I+1, N>::apply(s, v);
    }
};

template <typename S, typename T, std::size_t IS, std::size_t N>
struct mul_impl<S, T, IS, N, N>
{
    static inline void apply(S &, T const&) {}
};

template <std::size_t IS, std::size_t N, typename S, typename T>
inline static void mul(S & s, T const& v)
{
    return mul_impl<S, T, IS, 0, N>::apply(s, v);
}

// Negation

template <typename V, std::size_t I, std::size_t N>
struct neg_impl
{
    BOOST_STATIC_ASSERT(0 < N);

    static inline void apply(V & v)
    {
        set<I>(v, -get<I>(v));
        neg_impl<V, I+1, N>::apply(v);
    }
};

template <typename V, std::size_t N>
struct neg_impl<V, N, N>
{
    static inline void apply(V &) {}
};

template <std::size_t I, std::size_t N, typename V>
inline static void neg(V & v)
{
    return neg_impl<V, I, N>::apply(v);
}

// Normalization of N components starting from Ith

template <std::size_t I, std::size_t N, typename S>
inline static void normalize(S & s)
{
    typedef typename traits::coordinate_type<S>::type T;

    T lsqr = dot<I, I, N>(s, s);
    if ( std::numeric_limits<T>::epsilon() < lsqr )
        mul<I, N>(s, 1.0f / math::sqrt(lsqr));
}

// Square matrix * Vector of the same dimension

template <typename M, typename V, typename VD, std::size_t I, std::size_t N>
struct matrix_mul_row_impl
{
    BOOST_STATIC_ASSERT(0 < N);

    static const std::size_t dimension = traits::dimension<M>::value;

    static inline
    typename traits::coordinate_type<VD>::type
    apply(M const& m, V const& v)
    {
        return matrix_mul_row_impl<M, V, VD, I, N-1>::apply(m, v) + get<I, N-1>(m) * get<N-1>(v);
    }
};

template <typename M, typename V, typename VD, std::size_t I>
struct matrix_mul_row_impl<M, V, VD, I, 1>
{
    static const std::size_t dimension = traits::dimension<M>::value;

    static inline
    typename traits::coordinate_type<VD>::type
    apply(M const& m, V const& v)
    {
        return get<I, 0>(m) * get<0>(v);
    }
};

template <typename M, typename V, typename VD, std::size_t I, std::size_t N>
struct matrix_mul_impl
{
    static inline void apply(M const& m, V const& v, VD & vd)
    {
        set<I>(vd, matrix_mul_row_impl<M, V, VD, I, N>::apply(m, v));
        matrix_mul_impl<M, V, VD, I+1, N>::apply(m, v, vd);
    }
};

template <typename M, typename V, typename VD, std::size_t N>
struct matrix_mul_impl<M, V, VD, N, N>
{
    static inline void apply(M const&, V const&, VD &) {}
};

// Matrix rotation - M*V

template <typename M, typename V, typename VD>
inline static void matrix_rotate(M const& m, V const& v, VD & vd)
{
    static const std::size_t dimension = traits::dimension<M>::value;

    matrix_mul_impl<M, V, VD, 0, dimension>::apply(m, v, vd);
}

// Quaternion rotation - Q*V*Q' - * is Hamilton product

template <typename V, typename Q>
inline static void quaternion_rotate(V & v, Q const& r)
{
    // TODO - choose more precise type?

    typedef typename geometry::select_most_precise<
        typename traits::coordinate_type<V>::type,
        typename traits::coordinate_type<Q>::type
    >::type T;

    // Hamilton product T=Q*V
    T a = /*get<0>(r) * 0 */- get<1>(r) * get<0>(v) - get<2>(r) * get<1>(v) - get<3>(r) * get<2>(v);
    T b = get<0>(r) * get<0>(v)/* + get<1>(r) * 0*/ + get<2>(r) * get<2>(v) - get<3>(r) * get<1>(v);
    T c = get<0>(r) * get<1>(v) - get<1>(r) * get<2>(v)/* + get<2>(r) * 0*/ + get<3>(r) * get<0>(v);
    T d = get<0>(r) * get<2>(v) + get<1>(r) * get<1>(v) - get<2>(r) * get<0>(v)/* + get<3>(r) * 0*/;
    // Hamilton product V=T*inv(Q)
    set<0>(v, - a * get<1>(r) + b * get<0>(r) - c * get<3>(r) + d * get<2>(r));
    set<1>(v, - a * get<2>(r) + b * get<3>(r) + c * get<0>(r) - d * get<1>(r));
    set<2>(v, - a * get<3>(r) - b * get<2>(r) + c * get<1>(r) + d * get<0>(r));
}

// Assign value

template <typename G, typename V, std::size_t B, std::size_t E>
struct assign_value
{
    static inline void apply(G & g, V const& v)
    {
        set<B>(g, v);
        assign_value<G, V, B+1, E>::apply(g, v);
    }
};

template <typename G, typename V, std::size_t E>
struct assign_value<G, V, E, E>
{
    static inline void apply(G &, V const&) {}
};

template <typename G, typename V, std::size_t BI, std::size_t BD, std::size_t EI, std::size_t ED>
struct indexed_assign_value_per_index
{
    static inline void apply(G & g, V const& v)
    {
        set<BI, BD>(g, v);
        indexed_assign_value_per_index<G, V, BI, BD+1, EI, ED>::apply(g, v);
    }
};

// Assign value using indexed access

template <typename G, typename V, std::size_t BI, std::size_t EI, std::size_t ED>
struct indexed_assign_value_per_index<G, V, BI, ED, EI, ED>
{
    static inline void apply(G &, V const&) {}
};

template <typename G, typename V, std::size_t BI, std::size_t BD, std::size_t EI, std::size_t ED>
struct indexed_assign_value
{
    static inline void apply(G & g, V const& v)
    {
        indexed_assign_value_per_index<G, V, BI, BD, EI, ED>::apply(g, v);
        indexed_assign_value<G, V, BI+1, BD, EI, ED>::apply(g, v);
    }
};

template <typename G, typename V, std::size_t BD, std::size_t EI, std::size_t ED>
struct indexed_assign_value<G, V, EI, BD, EI, ED>
{
    static inline void apply(G &, V const&) {}
};

template <typename G, std::size_t BI, std::size_t BD, std::size_t EI, std::size_t ED>
struct identity_matrix_per_index
{
    static inline void apply(G & g)
    {
        set<BI, BD>(g, 0);
        identity_matrix_per_index<G, BI, BD+1, EI, ED>::apply(g);
    }
};

// Identity matrix

template <typename G, std::size_t BI, std::size_t EI, std::size_t ED>
struct identity_matrix_per_index<G, BI, BI, EI, ED>
{
    static inline void apply(G & g)
    {
        set<BI, BI>(g, 1);
        identity_matrix_per_index<G, BI, BI+1, EI, ED>::apply(g);
    }
};

template <typename G, std::size_t BI, std::size_t EI, std::size_t ED>
struct identity_matrix_per_index<G, BI, ED, EI, ED>
{
    static inline void apply(G &) {}
};

template <typename G, std::size_t BI, std::size_t BD, std::size_t EI, std::size_t ED>
struct identity_matrix
{
    static inline void apply(G & g)
    {
        identity_matrix_per_index<G, BI, BD, EI, ED>::apply(g);
        identity_matrix<G, BI+1, BD, EI, ED>::apply(g);
    }
};

template <typename G, std::size_t BD, std::size_t EI, std::size_t ED>
struct identity_matrix<G, EI, BD, EI, ED>
{
    static inline void apply(G &) {}
};

// Matrix transpose

template <typename G, std::size_t I, std::size_t D, std::size_t N>
struct matrix_transpose_per_index
{
    static inline void apply(G & g)
    {
        // swap coordinates
        typename coordinate_type<G>::type tmp = get<I, D>(g);
        set<I, D>(g, get<D, I>(g));
        set<D, I>(g, tmp);
        matrix_transpose_per_index<G, I, D+1, N>::apply(g);
    }
};

template <typename G, std::size_t I, std::size_t N>
struct matrix_transpose_per_index<G, I, N, N>
{
    static inline void apply(G &) {}
};

template <typename G, std::size_t I, std::size_t D, std::size_t N>
struct matrix_transpose
{
    static inline void apply(G & g)
    {
        matrix_transpose_per_index<G, I, I+1, N>::apply(g);
        matrix_transpose<G, I+1, D, N>::apply(g);
    }
};

template <typename G, std::size_t D, std::size_t N>
struct matrix_transpose<G, N, D, N>
{
    static inline void apply(G &) {}
};

}} // namespace detail::algebra
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_DETAIL_HPP
