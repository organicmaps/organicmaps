/**
 * -*- c++ -*-
 *
 * \file num_columns.hpp
 *
 * \brief The \c num_columns operation.
 *
 * Copyright (c) 2009, Marco Guazzone
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * \author Marco Guazzone, marco.guazzone@gmail.com
 */


#ifndef BOOST_NUMERIC_UBLAS_OPERATION_NUM_COLUMNS_HPP
#define BOOST_NUMERIC_UBLAS_OPERATION_NUM_COLUMNS_HPP


#include <boost/numeric/ublas/detail/config.hpp>


namespace boost { namespace numeric { namespace ublas {

    /**
     * \brief Return the number of columns.
     * \tparam MatrixExprT A type which models the matrix expression concept.
     * \param m A matrix expression.
     * \return The number of columns.
     */
    template <typename MatrixExprT>
    BOOST_UBLAS_INLINE
    typename MatrixExprT::size_type num_columns(MatrixExprT const& m)
    {
        return m.size2();
    }

}}} // Namespace boost::numeric::ublas


#endif // BOOST_NUMERIC_UBLAS_OPERATION_NUM_COLUMNS_HPP
