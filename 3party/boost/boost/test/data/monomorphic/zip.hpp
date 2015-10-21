//  (C) Copyright Gennadiy Rozental 2011-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines monomorphic dataset based on zipping of 2 other monomorphic datasets
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_ZIP_HPP_102211GER
#define BOOST_TEST_DATA_MONOMORPHIC_ZIP_HPP_102211GER

#include <boost/test/data/config.hpp>

#if !defined(BOOST_TEST_NO_ZIP_COMPOSITION_AVAILABLE) || defined(BOOST_TEST_DOXYGEN_DOC__)

// Boost.Test
#include <boost/test/data/monomorphic/dataset.hpp>
#include <boost/test/detail/suppress_warnings.hpp>


namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

namespace ds_detail {


//____________________________________________________________________________//

// !! ?? variadic template implementation; use forward_as_tuple?
template<typename T1, typename T2>
struct zip_traits {
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
struct zip_traits<T1,std::tuple<T2,T3>> {
    typedef std::tuple<T1,T2,T3> type;
    typedef typename monomorphic::traits<type>::ref_type ref_type;

    static ref_type
    tuple_merge(T1 const& a1, std::tuple<T2 const&,T3 const&> const& a2)
    {
        return ref_type(a1,std::get<0>(a2),std::get<1>(a2));
    }
};

//____________________________________________________________________________//

template<typename T1, typename T2,typename T3>
struct zip_traits<std::tuple<T1,T2>,T3> {
    typedef std::tuple<T1,T2,T3> type;
    typedef typename monomorphic::traits<type>::ref_type ref_type;

    static ref_type
    tuple_merge(std::tuple<T1 const&,T2 const&> const& a1, T3 const& a2)
    {
        return ref_type(std::get<0>(a1),std::get<1>(a1),a2);
    }
};

//____________________________________________________________________________//

} // namespace ds_detail

// ************************************************************************** //
// **************                       zip                    ************** //
// ************************************************************************** //

//! Zip datasets
//!
//! A zip of two datasets is a dataset whose arity is the sum of the operand datasets arity. The size is given by
//! the function creating the instance (see @c operator^ on datasets).
template<typename DS1, typename DS2>
class zip : public monomorphic::dataset<typename ds_detail::zip_traits<typename boost::decay<DS1>::type::data_type,
                                                                       typename boost::decay<DS2>::type::data_type>::type> {
    typedef typename boost::decay<DS1>::type::data_type T1;
    typedef typename boost::decay<DS2>::type::data_type T2;

    typedef typename monomorphic::dataset<T1>::iter_ptr ds1_iter_ptr;
    typedef typename monomorphic::dataset<T2>::iter_ptr ds2_iter_ptr;

    typedef typename ds_detail::zip_traits<T1,T2>::type T;
    typedef monomorphic::dataset<T> base;
    typedef typename base::iter_ptr iter_ptr;

    struct iterator : public base::iterator {
        typedef typename monomorphic::traits<T>::ref_type ref_type;

        // Constructor
        explicit    iterator( ds1_iter_ptr iter1, ds2_iter_ptr iter2 )
        : m_iter1( iter1 )
        , m_iter2( iter2 )
        {}

        // forward iterator interface
        virtual ref_type    operator*()     { return ds_detail::zip_traits<T1,T2>::tuple_merge( **m_iter1, **m_iter2 ); }
        virtual void        operator++()    { ++(*m_iter1); ++(*m_iter2); }

    private:
        // Data members
        ds1_iter_ptr    m_iter1;
        ds2_iter_ptr    m_iter2;
    };

public:
    enum { arity = boost::decay<DS1>::type::arity + boost::decay<DS2>::type::arity };

    //! Constructor
    //!
    //! The datasets are moved and not copied.
    zip( DS1&& ds1, DS2&& ds2, data::size_t size )
    : m_ds1( std::forward<DS1>( ds1 ) )
    , m_ds2( std::forward<DS2>( ds2 ) )
    , m_size( size )
    {}

    //! Move constructor
    zip( zip&& j )
    : m_ds1( std::forward<DS1>( j.m_ds1 ) )
    , m_ds2( std::forward<DS2>( j.m_ds2 ) )
    , m_size( j.m_size )
    {}

    // dataset interface
    virtual data::size_t    size() const    { return m_size; }
    virtual iter_ptr        begin() const   { return boost::make_shared<iterator>( m_ds1.begin(), m_ds2.begin() ); }

private:
    // Data members
    DS1             m_ds1;
    DS2             m_ds2;
    data::size_t    m_size;
};

//____________________________________________________________________________//

//! Zipped datasets results in a dataset.
template<typename DS1, typename DS2>
struct is_dataset<zip<DS1,DS2> > : mpl::true_ {};

//____________________________________________________________________________//

namespace ds_detail {

//! Handles the sise of the resulting zipped dataset.
template<typename DS1, typename DS2>
inline data::size_t
zip_size( DS1&& ds1, DS2&& ds2 )
{
    data::size_t ds1_size = ds1.size();
    data::size_t ds2_size = ds2.size();

    if( ds1_size == ds2_size )
        return ds1_size;

    if( ds1_size == 1 || ds1_size.is_inf() )
        return ds2_size;

    if( ds2_size == 1  || ds2_size.is_inf() )
        return ds1_size;

    BOOST_TEST_DS_ERROR( "Can't zip datasets of different sizes" );
}

} // namespace ds_detail

//____________________________________________________________________________//

namespace result_of {

//! Result type of the zip operator.
template<typename DS1Gen, typename DS2Gen>
struct zip {
    typedef monomorphic::zip<typename DS1Gen::type,typename DS2Gen::type> type;
};

} // namespace result_of


//____________________________________________________________________________//


//! Overload operator for zip support
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::zip<mpl::identity<DS1>,mpl::identity<DS2>>
>::type
operator^( DS1&& ds1, DS2&& ds2 )
{
    return zip<DS1,DS2>( std::forward<DS1>( ds1 ),  std::forward<DS2>( ds2 ), ds_detail::zip_size( ds1, ds2 ) );
}

//! @overload boost::unit_test::data::monomorphic::operator^()
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && !is_dataset<DS2>::value,
                                        result_of::zip<mpl::identity<DS1>,data::result_of::make<DS2>>
>::type
operator^( DS1&& ds1, DS2&& ds2 )
{
    return std::forward<DS1>(ds1) ^ data::make(std::forward<DS2>(ds2));
}

//! @overload boost::unit_test::data::monomorphic::operator^()
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<!is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::zip<data::result_of::make<DS1>,mpl::identity<DS2>>
>::type
operator^( DS1&& ds1, DS2&& ds2 )
{
    return data::make(std::forward<DS1>(ds1)) ^ std::forward<DS2>(ds2);
}

//____________________________________________________________________________//

} // namespace monomorphic

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_NO_ZIP_COMPOSITION_AVAILABLE

#endif // BOOST_TEST_DATA_MONOMORPHIC_ZIP_HPP_102211GER
