//  (C) Copyright Gennadiy Rozental 2011-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
/// Defines monomorphic dataset n+m dimentional *. Samples in this
/// dataset is grid of elements in DS1 and DS2. There will be total
/// |DS1| * |DS2| samples
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_GRID_HPP_101512GER
#define BOOST_TEST_DATA_MONOMORPHIC_GRID_HPP_101512GER

// Boost.Test
#include <boost/test/data/config.hpp>


#if !defined(BOOST_TEST_NO_GRID_COMPOSITION_AVAILABLE) || defined(BOOST_TEST_DOXYGEN_DOC__)

#include <boost/test/data/monomorphic/dataset.hpp>
#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

namespace ds_detail {

// !! ?? variadic template implementation; use forward_as_tuple?
template<typename T1, typename T2>
struct grid_traits {
    typedef std::tuple<T1,T2> type;
    typedef typename monomorphic::traits<type>::ref_type ref_type;

    static ref_type
    tuple_merge(T1 const& a1, T2 const& a2)
    {
        return ref_type(a1,a2);
    }
};

//____________________________________________________________________________//

template<typename T1, typename T2,typename T3>
struct grid_traits<T1,std::tuple<T2,T3>> {
    typedef std::tuple<T1,T2,T3> type;
    typedef typename monomorphic::traits<type>::ref_type ref_type;

    static ref_type
    tuple_merge(T1 const& a1, std::tuple<T2 const&,T3 const&> const& a2)
    {
        return ref_type(a1,get<0>(a2),get<1>(a2));
    }
};

//____________________________________________________________________________//

template<typename T1, typename T2,typename T3>
struct grid_traits<std::tuple<T1,T2>,T3> {
    typedef std::tuple<T1,T2,T3> type;
    typedef typename monomorphic::traits<type>::ref_type ref_type;

    static ref_type
    tuple_merge(std::tuple<T1 const&,T2 const&> const& a1, T3 const& a2)
    {
        return ref_type(get<0>(a1),get<1>(a1),a2);
    }
};

//____________________________________________________________________________//

} // namespace ds_detail

// ************************************************************************** //
// **************                       grid                    ************** //
// ************************************************************************** //


//! Implements the dataset resulting from a cartesian product/grid operation on datasets.
//!
//! The arity of the resulting dataset is the sum of the arity of its operands.
template<typename DS1, typename DS2>
class grid : public monomorphic::dataset<typename ds_detail::grid_traits<typename boost::decay<DS1>::type::data_type,
                                                                         typename boost::decay<DS2>::type::data_type>::type> {
    typedef typename boost::decay<DS1>::type::data_type T1;
    typedef typename boost::decay<DS2>::type::data_type T2;

    typedef typename monomorphic::dataset<T1>::iter_ptr ds1_iter_ptr;
    typedef typename monomorphic::dataset<T2>::iter_ptr ds2_iter_ptr;

    typedef typename ds_detail::grid_traits<T1,T2>::type T;
    typedef monomorphic::dataset<T> base;
    typedef typename base::iter_ptr iter_ptr;

    struct iterator : public base::iterator {
        typedef typename monomorphic::traits<T>::ref_type ref_type;

        // Constructor
        explicit    iterator( ds1_iter_ptr iter1, DS2 const& ds2 )
        : m_iter1( iter1 )
        , m_iter2( ds2.begin() )
        , m_ds2( ds2 )
        , m_ds2_pos( 0 )
        {}

        // forward iterator interface
        virtual ref_type    operator*()     { return ds_detail::grid_traits<T1,T2>::tuple_merge( **m_iter1, **m_iter2 ); }
        virtual void        operator++()
        {
            ++m_ds2_pos;
            if( m_ds2_pos != m_ds2.size() )
                ++(*m_iter2);
            else {
                m_ds2_pos = 0;
                ++(*m_iter1);
                m_iter2 = m_ds2.begin();
            }
        }

    private:
        // Data members
        ds1_iter_ptr    m_iter1;
        ds2_iter_ptr    m_iter2;
        DS2 const&      m_ds2;
        data::size_t    m_ds2_pos;
    };

public:
    enum { arity = boost::decay<DS1>::type::arity + boost::decay<DS2>::type::arity };

    //! Constructor
    grid( DS1&& ds1, DS2&& ds2 )
    : m_ds1( std::forward<DS1>( ds1 ) )
    , m_ds2( std::forward<DS2>( ds2 ) )
    {}

    //! Move constructor
    grid( grid&& j )
    : m_ds1( std::forward<DS1>( j.m_ds1 ) )
    , m_ds2( std::forward<DS2>( j.m_ds2 ) )
    {}

    // dataset interface
    virtual data::size_t    size() const    { return m_ds1.size() * m_ds2.size(); }
    virtual iter_ptr        begin() const   { return boost::make_shared<iterator>( m_ds1.begin(), m_ds2 ); }

private:
    // Data members
    DS1             m_ds1;
    DS2             m_ds2;
};

//____________________________________________________________________________//

// A grid dataset is a dataset
template<typename DS1, typename DS2>
struct is_dataset<grid<DS1,DS2> > : mpl::true_ {};

//____________________________________________________________________________//

namespace result_of {

/// Result type of the grid operation on dataset.
template<typename DS1Gen, typename DS2Gen>
struct grid {
    typedef monomorphic::grid<typename DS1Gen::type,typename DS2Gen::type> type;
};

} // namespace result_of

//____________________________________________________________________________//



//! Grid operation
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::grid<mpl::identity<DS1>,mpl::identity<DS2>>
>::type
operator*( DS1&& ds1, DS2&& ds2 )
{
    BOOST_TEST_DS_ASSERT( !ds1.size().is_inf() && !ds2.size().is_inf(), "Grid dimension can't have infinite size" );

    return grid<DS1,DS2>( std::forward<DS1>( ds1 ),  std::forward<DS2>( ds2 ) );
}

//! @overload boost::unit_test::data::operator*
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && !is_dataset<DS2>::value,
                                        result_of::grid<mpl::identity<DS1>,data::result_of::make<DS2>>
>::type
operator*( DS1&& ds1, DS2&& ds2 )
{
    return std::forward<DS1>(ds1) * data::make(std::forward<DS2>(ds2));
}

//! @overload boost::unit_test::data::operator*
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<!is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::grid<data::result_of::make<DS1>,mpl::identity<DS2>>
>::type
operator*( DS1&& ds1, DS2&& ds2 )
{
    return data::make(std::forward<DS1>(ds1)) * std::forward<DS2>(ds2);
}

//____________________________________________________________________________//

} // namespace monomorphic

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_NO_GRID_COMPOSITION_AVAILABLE

#endif // BOOST_TEST_DATA_MONOMORPHIC_GRID_HPP_101512GER
