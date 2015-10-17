//  (C) Copyright Gennadiy Rozental 2011-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! Defines dataset join operation
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_JOIN_HPP_112711GER
#define BOOST_TEST_DATA_MONOMORPHIC_JOIN_HPP_112711GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/monomorphic/dataset.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************                      join                    ************** //
// ************************************************************************** //

//! Defines a new dataset from the concatenation of two datasets
//!
//! The size of the resulting dataset is the sum of the two underlying datasets. The arity of the datasets
//! should match.
template<typename DS1, typename DS2>
class join : public monomorphic::dataset<typename boost::decay<DS1>::type::data_type> {
    typedef typename boost::decay<DS1>::type::data_type T;
    typedef monomorphic::dataset<T> base;
    typedef typename base::iter_ptr iter_ptr;

    struct iterator : public base::iterator {
        // Constructor
        explicit    iterator( iter_ptr it1, iter_ptr it2, data::size_t first_size )
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        : m_it1( std::move(it1) )
        , m_it2( std::move(it2) )
#else
        : m_it1( it1 )
        , m_it2( it2 )
#endif
        , m_first_size( first_size )
        {}

        // forward iterator interface
        virtual T const&    operator*()     { return m_first_size > 0 ? **m_it1 : **m_it2; }
        virtual void        operator++()    { m_first_size > 0 ? (--m_first_size,++(*m_it1)) : ++(*m_it2); }

    private:
        // Data members
        iter_ptr        m_it1;
        iter_ptr        m_it2;
        data::size_t    m_first_size;
    };

public:
    enum { arity = boost::decay<DS1>::type::arity };

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    // Constructor
    join( DS1&& ds1, DS2&& ds2 )
    : m_ds1( std::forward<DS1>( ds1 ) )
    , m_ds2( std::forward<DS2>( ds2 ) )
    {}

    // Move constructor
    join( join&& j )
    : m_ds1( std::forward<DS1>( j.m_ds1 ) )
    , m_ds2( std::forward<DS2>( j.m_ds2 ) )
    {}
#else
    // Constructor
    join( DS1 const& ds1, DS2 const& ds2 )
    : m_ds1( ds1 )
    , m_ds2( ds2 )
    {}
#endif

    // dataset interface
    virtual data::size_t    size() const            { return m_ds1.size() + m_ds2.size(); }
    virtual iter_ptr        begin() const           { return boost::make_shared<iterator>( m_ds1.begin(),
                                                                                           m_ds2.begin(),
                                                                                           m_ds1.size() ); }

private:
    // Data members
    DS1 m_ds1;
    DS2 m_ds2;
};

//____________________________________________________________________________//

// A joined dataset  is a dataset.
template<typename DS1, typename DS2>
struct is_dataset<join<DS1,DS2> > : mpl::true_ {};

//____________________________________________________________________________//

namespace result_of {

//! Result type of the join operation on datasets.
template<typename DS1Gen, typename DS2Gen>
struct join {
    typedef monomorphic::join<typename DS1Gen::type,typename DS2Gen::type> type;
};

} // namespace result_of

//____________________________________________________________________________//

template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::join<mpl::identity<DS1>,mpl::identity<DS2> >
>::type
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
operator+( DS1&& ds1, DS2&& ds2 )
{
    return join<DS1,DS2>( std::forward<DS1>( ds1 ),  std::forward<DS2>( ds2 ) );
}
#else
operator+( DS1 const& ds1, DS2 const& ds2 )
{
    return join<DS1,DS2>( ds1,  ds2 );
}
#endif

//____________________________________________________________________________//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && !is_dataset<DS2>::value,
                                        result_of::join<mpl::identity<DS1>,data::result_of::make<DS2> >
>::type
operator+( DS1&& ds1, DS2&& ds2 )
{
    return std::forward<DS1>(ds1) + data::make(std::forward<DS2>(ds2));
}
#else
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<is_dataset<DS1>::value && !is_dataset<DS2>::value,
                                        result_of::join<mpl::identity<DS1>,data::result_of::make<DS2> >
>::type
operator+( DS1 const& ds1, DS2 const& ds2 )
{
    return ds1 + data::make(ds2);
}
#endif

//____________________________________________________________________________//


#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<!is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::join<data::result_of::make<DS1>,mpl::identity<DS2> >
>::type
operator+( DS1&& ds1, DS2&& ds2 )
{
    return data::make(std::forward<DS1>(ds1)) + std::forward<DS2>(ds2);
}
#else
template<typename DS1, typename DS2>
inline typename boost::lazy_enable_if_c<!is_dataset<DS1>::value && is_dataset<DS2>::value,
                                        result_of::join<data::result_of::make<DS1>,mpl::identity<DS2> >
>::type
operator+( DS1 const& ds1, DS2 const& ds2 )
{
    return data::make(ds1) + ds2;
}

#endif


} // namespace monomorphic

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_JOIN_HPP_112711GER
