//  (C) Copyright Gennadiy Rozental 2011-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///Defines monomorphic dataset based on forward iterable sequence
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_COLLECTION_HPP_102211GER
#define BOOST_TEST_DATA_MONOMORPHIC_COLLECTION_HPP_102211GER

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
// **************                  collection                  ************** //
// ************************************************************************** //


//!@brief Dataset from a forward iterable container (collection)
//!
//! This dataset is applicable to any container implementing a forward iterator. Note that
//! container with one element will be considered as singletons.
//! This dataset is constructible with the @ref boost::unit_test::data::make function.
//!
//! For compilers supporting r-value references, the collection is moved instead of copied.
template<typename C>
class collection : public monomorphic::dataset<typename boost::decay<C>::type::value_type> {
    typedef typename boost::decay<C>::type col_type;
    typedef typename col_type::value_type T;
    typedef monomorphic::dataset<T> base;
    typedef typename base::iter_ptr iter_ptr;

    struct iterator : public base::iterator {
        // Constructor
        explicit    iterator( collection<C> const& owner )
        : m_iter( owner.col().begin() )
        , m_singleton( owner.col().size() == 1 )
        {}

        // forward iterator interface
        virtual T const&    operator*()     { return *m_iter; }
        virtual void        operator++()    { if( !m_singleton ) ++m_iter; }

    private:
        // Data members
        typename col_type::const_iterator m_iter;
        bool                m_singleton;
    };

public:
    enum { arity = 1 };

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    //! Constructor
    //! The collection is moved
    explicit                collection( C&& col ) : m_col( std::forward<C>(col) ) {}

    //! Move constructor
    collection( collection&& c ) : m_col( std::forward<C>( c.m_col ) ) {}
#else
    //! Constructor
    //! The collection is copied
    explicit                collection( C const& col ) : m_col( col ) {}
#endif

    //! Returns the underlying collection
    C const&                col() const             { return m_col; }

    // dataset interface
    virtual data::size_t    size() const            { return m_col.size(); }
    virtual iter_ptr        begin() const           { return boost::make_shared<iterator>( *this ); }

private:
    // Data members
    C               m_col;
};

//____________________________________________________________________________//

//! A collection from a forward iterable container is a dataset.
template<typename C>
struct is_dataset<collection<C> > : mpl::true_ {};

} // namespace monomorphic

//! @overload boost::unit_test::data::make()
template<typename C>
inline typename BOOST_TEST_ENABLE_IF<is_forward_iterable<C>::value,
                                     monomorphic::collection<C> >::type
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
make( C&& c )
{
    return monomorphic::collection<C>( std::forward<C>(c) );
}
#else
make( C const& c )
{
    return monomorphic::collection<C>( c );
}
#endif

//____________________________________________________________________________//

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_COLLECTION_HPP_102211GER
