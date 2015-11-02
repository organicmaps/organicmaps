//  (C) Copyright Gennadiy Rozental 2004-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description :
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_IFSTREAM_LINE_ITERATOR_HPP
#define BOOST_TEST_UTILS_IFSTREAM_LINE_ITERATOR_HPP

// Boost
#include <boost/test/utils/iterator/istream_line_iterator.hpp>

// STL
#include <fstream>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {

namespace unit_test {

namespace ut_detail {

// ************************************************************************** //
// **************                ifstream_holder               ************** //
// ************************************************************************** //

template<typename CharT>
class ifstream_holder {
public:
    // Constructor
    explicit    ifstream_holder( basic_cstring<CharT const> file_name )
    {
        if( file_name.is_empty() )
            return;

        m_stream.open( file_name.begin(), std::ios::in );
    }

    bool is_valid()
    {
        return m_stream.is_open();
    }

protected:
#ifdef BOOST_CLASSIC_IOSTREAMS
    typedef std::ifstream                                       stream_t;
#else
    typedef std::basic_ifstream<CharT,std::char_traits<CharT> > stream_t;
#endif

    // Data members
    stream_t    m_stream;
};

} // namespace ut_detail

// ************************************************************************** //
// **************         basic_ifstream_line_iterator         ************** //
// ************************************************************************** //

#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif

template<typename CharT>
class basic_ifstream_line_iterator : ut_detail::ifstream_holder<CharT>, public basic_istream_line_iterator<CharT>
{
public:
    basic_ifstream_line_iterator( basic_cstring<CharT const> file_name, CharT delimeter )
    : ut_detail::ifstream_holder<CharT>( file_name ), basic_istream_line_iterator<CharT>( this->m_stream, delimeter ) {}

    explicit basic_ifstream_line_iterator( basic_cstring<CharT const> file_name = basic_cstring<CharT const>() )
    : ut_detail::ifstream_holder<CharT>( file_name ), basic_istream_line_iterator<CharT>( this->m_stream ) {}
};

#ifdef BOOST_MSVC
# pragma warning(default: 4355)
# pragma warning(pop)
#endif

typedef basic_ifstream_line_iterator<char>      ifstream_line_iterator;
typedef basic_ifstream_line_iterator<wchar_t>   wifstream_line_iterator;

} // namespace unit_test

} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_IFSTREAM_LINE_ITERATOR_HPP

