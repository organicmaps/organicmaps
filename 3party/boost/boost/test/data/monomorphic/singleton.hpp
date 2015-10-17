//  (C) Copyright Gennadiy Rozental 2011-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines single element monomorphic dataset
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_SINGLETON_HPP_102211GER
#define BOOST_TEST_DATA_MONOMORPHIC_SINGLETON_HPP_102211GER

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
// **************                    singleton                  ************** //
// ************************************************************************** //

/// Models a single element data set
template<typename T>
class singleton : public monomorphic::dataset<typename boost::decay<T>::type> {
    typedef monomorphic::dataset<typename boost::decay<T>::type> base;
    typedef typename base::iter_ptr  iter_ptr;

    struct iterator : public base::iterator {
        // Constructor
        explicit            iterator( singleton<T> const& owner )
        : m_owner( owner )
        {}

        // forward iterator interface
        virtual typename base::data_type const&
                            operator*()     { return m_owner.value(); }
        virtual void        operator++()    {}

    private:
        singleton<T> const& m_owner;
    };

public:
    enum { arity = 1 };

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    //! Constructor
    explicit                singleton( T&& value ) : m_value( std::forward<T>( value ) ) {}

    //! Move constructor
    singleton( singleton&& s ) : m_value( std::forward<T>( s.m_value ) ) {}
#else
    // Constructor
    explicit                singleton( T const& value ) : m_value( value ) {}
#endif

    // Access methods
    T const&                value() const           { return m_value; }

    // dataset interface
    virtual data::size_t    size() const            { return 1; }
    virtual iter_ptr        begin() const           { return boost::make_shared<iterator>( *this ); }

private:
    // Data members
    T               m_value;
};

// a singleton is a dataset
template<typename T>
struct is_dataset<singleton<T> > : mpl::true_ {};

} // namespace monomorphic



/// @overload boost::unit_test::data::make()
template<typename T>
inline typename BOOST_TEST_ENABLE_IF<!is_forward_iterable<T>::value &&
                                     !monomorphic::is_dataset<T>::value &&
                                     !boost::is_array< typename boost::remove_reference<T>::type >::value,
                                     monomorphic::singleton<T>
>::type
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
make( T&& v )
{
    return monomorphic::singleton<T>( std::forward<T>( v ) );
}
#else
make( T const& v )
{
    return monomorphic::singleton<T>( v );
}
#endif


/// @overload boost::unit_test::data::make
inline monomorphic::singleton<char*> make( char* str )
{
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    return monomorphic::singleton<char*>( std::move(str) );
#else
    return monomorphic::singleton<char*>( str );
#endif
}


/// @overload boost::unit_test::data::make
inline monomorphic::singleton<char const*> make( char const* str )
{
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    return monomorphic::singleton<char const*>( std::move(str) );
#else
    return monomorphic::singleton<char const*>( str );
#endif
}



} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_SINGLETON_HPP_102211GER
