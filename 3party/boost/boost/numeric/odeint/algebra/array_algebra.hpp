/*
 [auto_generated]
 boost/numeric/odeint/algebra/array_algebra.hpp

 [begin_description]
 Algebra for boost::array. Highly specialized for odeint. Const arguments are introduce to work with odeint.
 [end_description]

 Copyright 2011-2013 Mario Mulansky
 Copyright 2011-2012 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_ALGEBRA_ARRAY_ALGEBRA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_ALGEBRA_ARRAY_ALGEBRA_HPP_INCLUDED

#include <algorithm>
#include <boost/array.hpp>

#include <boost/numeric/odeint/algebra/norm_result_type.hpp>

namespace boost {
namespace numeric {
namespace odeint {

struct array_algebra
{
    template< typename T , size_t dim , class Op >
    static void for_each1( boost::array< T , dim > &s1 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] );
    }


    template< typename T1 , typename T2 , size_t dim , class Op >
    static void for_each2( boost::array< T1 , dim > &s1 ,
            const boost::array< T2 , dim > &s2 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each3( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] );
    }

    /* different const signature - required for the scale_sum_swap2 operation */
    template< typename T , size_t dim , class Op >
    static void for_each3( boost::array< T , dim > &s1 ,
            boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each4( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each5( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each6( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each7( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each8( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each9( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each10( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 ,
            const boost::array< T , dim > &s10 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] , s10[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each11( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 ,
            const boost::array< T , dim > &s10 ,
            const boost::array< T , dim > &s11 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] , s10[i] , s11[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each12( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 ,
            const boost::array< T , dim > &s10 ,
            const boost::array< T , dim > &s11 ,
            const boost::array< T , dim > &s12 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] , s10[i] , s11[i] , s12[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each13( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 ,
            const boost::array< T , dim > &s10 ,
            const boost::array< T , dim > &s11 ,
            const boost::array< T , dim > &s12 ,
            const boost::array< T , dim > &s13 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] , s10[i] , s11[i] , s12[i] , s13[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each14( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 ,
            const boost::array< T , dim > &s10 ,
            const boost::array< T , dim > &s11 ,
            const boost::array< T , dim > &s12 ,
            const boost::array< T , dim > &s13 ,
            const boost::array< T , dim > &s14 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] , s10[i] , s11[i] , s12[i] , s13[i] , s14[i] );
    }

    template< typename T , size_t dim , class Op >
    static void for_each15( boost::array< T , dim > &s1 ,
            const boost::array< T , dim > &s2 ,
            const boost::array< T , dim > &s3 ,
            const boost::array< T , dim > &s4 ,
            const boost::array< T , dim > &s5 ,
            const boost::array< T , dim > &s6 ,
            const boost::array< T , dim > &s7 ,
            const boost::array< T , dim > &s8 ,
            const boost::array< T , dim > &s9 ,
            const boost::array< T , dim > &s10 ,
            const boost::array< T , dim > &s11 ,
            const boost::array< T , dim > &s12 ,
            const boost::array< T , dim > &s13 ,
            const boost::array< T , dim > &s14 ,
            const boost::array< T , dim > &s15 , Op op )
    {
        for( size_t i=0 ; i<dim ; ++i )
            op( s1[i] , s2[i] , s3[i] , s4[i] , s5[i] , s6[i] , s7[i] , s8[i] , s9[i] , s10[i] , s11[i] , s12[i] , s13[i] , s14[i] , s15[i] );
    }


    template< typename T , size_t dim >
    static typename norm_result_type< boost::array< T , dim > >::type norm_inf( const boost::array< T , dim > &s )
    {
        BOOST_USING_STD_MAX();
        using std::abs;
        typedef typename norm_result_type< boost::array< T , dim > >::type result_type;
        result_type init = static_cast< result_type >( 0 );
        for( size_t i=0 ; i<dim ; ++i )
            init = max BOOST_PREVENT_MACRO_SUBSTITUTION ( init , static_cast< result_type >(abs(s[i])) );
        return init;
    }

};

}
}
}

#endif // BOOST_NUMERIC_ODEINT_ALGEBRA_ARRAY_ALGEBRA_HPP_INCLUDED
