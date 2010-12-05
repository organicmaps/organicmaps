/**
 * -*- c++ -*-
 *
 * \file size.hpp
 *
 * \brief The \c size operation.
 *
 * Copyright (c) 2009, Marco Guazzone
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * \author Marco Guazzone, marco.guazzone@gmail.com
 */

#ifndef BOOST_NUMERIC_UBLAS_OPERATION_SIZE_HPP
#define BOOST_NUMERIC_UBLAS_OPERATION_SIZE_HPP


#include <boost/numeric/ublas/detail/config.hpp>
#include <boost/numeric/ublas/expression_types.hpp>
#include <boost/numeric/ublas/fwd.hpp>
#include <boost/numeric/ublas/tags.hpp>
#include <cstddef>


namespace boost { namespace numeric { namespace ublas {

    namespace detail {

        /**
         * \brief Auxiliary class for computing the size of the given dimension for
         *  a container of the given category..
         * \tparam Dim The dimension number (starting from 1).
         * \tparam CategoryT The category type (e.g., vector_tag).
         */
        template <size_t Dim, typename CategoryT>
        struct size_by_dim_impl;


        /// \brief Specialization of \c size_by_dim_impl for computing the size of a
        ///  vector
        template <>
        struct size_by_dim_impl<1, vector_tag>
        {
            /**
             * \brief Compute the size of the given vector.
             * \tparam ExprT A vector expression type.
             * \pre ExprT must be a model of VectorExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size();
            }
        };


        /// \brief Specialization of \c size_by_dim_impl for computing the number of
        ///  rows of a matrix
        template <>
        struct size_by_dim_impl<1, matrix_tag>
        {
            /**
             * \brief Compute the number of rows of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size1();
            }
        };


        /// \brief Specialization of \c size_by_dim_impl for computing the number of
        ///  columns of a matrix
        template <>
        struct size_by_dim_impl<2, matrix_tag>
        {
            /**
             * \brief Compute the number of columns of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size2();
            }
        };


        /**
         * \brief Auxiliary class for computing the size of the given dimension for
         *  a container of the given category and with the given orientation..
         * \tparam Dim The dimension number (starting from 1).
         * \tparam CategoryT The category type (e.g., vector_tag).
         * \tparam OrientationT The orientation category type (e.g., row_major_tag).
         */
        template <typename TagT, typename CategoryT, typename OrientationT>
        struct size_by_tag_impl;


        /// \brief Specialization of \c size_by_tag_impl for computing the size of the
        ///  major dimension of a row-major oriented matrix.
        template <>
        struct size_by_tag_impl<tag::major, matrix_tag, row_major_tag>
        {
            /**
             * \brief Compute the number of rows of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size1();
            }
        };


        /// \brief Specialization of \c size_by_tag_impl for computing the size of the
        ///  minor dimension of a row-major oriented matrix.
        template <>
        struct size_by_tag_impl<tag::minor, matrix_tag, row_major_tag>
        {
            /**
             * \brief Compute the number of columns of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size2();
            }
        };


        /// \brief Specialization of \c size_by_tag_impl for computing the size of the
        ///  leading dimension of a row-major oriented matrix.
        template <>
        struct size_by_tag_impl<tag::leading, matrix_tag, row_major_tag>
        {
            /**
             * \brief Compute the number of columns of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size2();
            }
        };


        /// \brief Specialization of \c size_by_tag_impl for computing the size of the
        ///  major dimension of a column-major oriented matrix.
        template <>
        struct size_by_tag_impl<tag::major, matrix_tag, column_major_tag>
        {
            /**
             * \brief Compute the number of columns of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size2();
            }
        };


        /// \brief Specialization of \c size_by_tag_impl for computing the size of the
        ///  minor dimension of a column-major oriented matrix.
        template <>
        struct size_by_tag_impl<tag::minor, matrix_tag, column_major_tag>
        {
            /**
             * \brief Compute the number of rows of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size1();
            }
        };


        /// \brief Specialization of \c size_by_tag_impl for computing the size of the
        ///  leading dimension of a column-major oriented matrix.
        template <>
        struct size_by_tag_impl<tag::leading, matrix_tag, column_major_tag>
        {
            /**
             * \brief Compute the number of rows of the given matrix.
             * \tparam ExprT A matrix expression type.
             * \pre ExprT must be a model of MatrixExpression.
             */
            template <typename ExprT>
            BOOST_UBLAS_INLINE
            static typename ExprT::size_type apply(ExprT const& e)
            {
                return e.size1();
            }
        };

    } // Namespace detail


    /**
     * \brief Return the number of columns.
     * \tparam MatrixExprT A type which models the matrix expression concept.
     * \param m A matrix expression.
     * \return The number of columns.
     */
    template <typename VectorExprT>
    BOOST_UBLAS_INLINE
    typename VectorExprT::size_type size(VectorExprT const& v)
    {
        return v.size();
    }


    /**
     * \brief Return the size of the given dimension for the given expression.
     * \tparam Dim The dimension number (starting from 1).
     * \tparam ExprT An expression type.
     * \param e An expression.
     * \return The number of columns.
     * \return The size associated to the dimension \a Dim.
     */
    template <std::size_t Dim, typename ExprT>
    BOOST_UBLAS_INLINE
    typename ExprT::size_type size(ExprT const& e)
    {
        return detail::size_by_dim_impl<Dim, typename ExprT::type_category>::apply(e);
    }


    /**
     * \brief Return the size of the given dimension tag for the given expression.
     * \tparam TagT The dimension tag type (e.g., tag::major).
     * \tparam ExprT An expression type.
     * \param e An expression.
     * \return The size associated to the dimension tag \a TagT.
     */
    template <typename TagT, typename ExprT>
    BOOST_UBLAS_INLINE
    typename ExprT::size_type size(ExprT const& e)
    {
        return detail::size_by_tag_impl<TagT, typename ExprT::type_category, typename ExprT::orientation_category>::apply(e);
    }

}}} // Namespace boost::numeric::ublas


#endif // BOOST_NUMERIC_UBLAS_OPERATION_SIZE_HPP
