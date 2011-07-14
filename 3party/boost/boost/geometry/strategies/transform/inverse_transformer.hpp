// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_TRANSFORM_INVERSE_TRANSFORMER_HPP
#define BOOST_GEOMETRY_STRATEGIES_TRANSFORM_INVERSE_TRANSFORMER_HPP

// Remove the ublas checking, otherwise the inverse might fail
// (while nothing seems to be wrong)
#define BOOST_UBLAS_TYPE_CHECK 0

#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>

#include <boost/geometry/strategies/transform/matrix_transformers.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace transform
{

/*!
\brief Transformation strategy to do an inverse ransformation in Cartesian system
\ingroup strategies
\tparam P1 first point type
\tparam P2 second point type
 */
template <typename P1, typename P2>
class inverse_transformer
    : public ublas_transformer<P1, P2, dimension<P1>::type::value, dimension<P2>::type::value>
{
    typedef typename select_coordinate_type<P1, P2>::type T;

public :
    template <typename Transformer>
    inline inverse_transformer(Transformer const& input)
    {
        typedef boost::numeric::ublas::matrix<T> matrix_type;

        // create a working copy of the input
        matrix_type copy(input.matrix());

        // create a permutation matrix for the LU-factorization
        typedef boost::numeric::ublas::permutation_matrix<> permutation_matrix;
        permutation_matrix pm(copy.size1());

        // perform LU-factorization
        int res = boost::numeric::ublas::lu_factorize<matrix_type>(copy, pm);
        if( res == 0 )
        {
            // create identity matrix
            this->m_matrix.assign(boost::numeric::ublas::identity_matrix<T>(copy.size1()));

            // backsubstitute to get the inverse
            boost::numeric::ublas::lu_substitute(copy, pm, this->m_matrix);
        }
    }

};


}} // namespace strategy::transform


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_TRANSFORM_INVERSE_TRANSFORMER_HPP
