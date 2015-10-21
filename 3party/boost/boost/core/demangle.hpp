#ifndef BOOST_CORE_DEMANGLE_HPP_INCLUDED
#define BOOST_CORE_DEMANGLE_HPP_INCLUDED

// core::demangle
//
// Copyright 2014 Peter Dimov
// Copyright 2014 Andrey Semashev
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>
#include <string>

#if defined(BOOST_HAS_PRAGMA_ONCE)
# pragma once
#endif

#if defined( __clang__ ) && defined( __has_include )
# if __has_include(<cxxabi.h>)
#  define BOOST_CORE_HAS_CXXABI_H
# endif
#elif defined( __GLIBCXX__ ) || defined( __GLIBCPP__ )
# define BOOST_CORE_HAS_CXXABI_H
#endif

#if defined( BOOST_CORE_HAS_CXXABI_H )
# include <cxxabi.h>
# include <cstdlib>
# include <cstddef>
#endif

// Android compilation error workaround: gabi++ does not define __cxa_demangle()
// 3party/boost/boost/core/demangle.hpp:75:17: error: no member named '__cxa_demangle' in namespace '__cxxabiv1'
//    return abi::__cxa_demangle( name, NULL, &size, &status );
//           ~~~~~^
#ifdef __GABIXX_CXXABI_H__
# undef BOOST_CORE_HAS_CXXABI_H
#endif

namespace boost
{

namespace core
{

inline char const * demangle_alloc( char const * name ) BOOST_NOEXCEPT;
inline void demangle_free( char const * name ) BOOST_NOEXCEPT;

class scoped_demangled_name
{
private:
    char const * m_p;

public:
    explicit scoped_demangled_name( char const * name ) BOOST_NOEXCEPT :
        m_p( demangle_alloc( name ) )
    {
    }

    ~scoped_demangled_name() BOOST_NOEXCEPT
    {
        demangle_free( m_p );
    }

    char const * get() const BOOST_NOEXCEPT
    {
        return m_p;
    }

    BOOST_DELETED_FUNCTION(scoped_demangled_name( scoped_demangled_name const& ))
    BOOST_DELETED_FUNCTION(scoped_demangled_name& operator= ( scoped_demangled_name const& ))
};


#if defined( BOOST_CORE_HAS_CXXABI_H )

inline char const * demangle_alloc( char const * name ) BOOST_NOEXCEPT
{
    int status = 0;
    std::size_t size = 0;
    return abi::__cxa_demangle( name, NULL, &size, &status );
}

inline void demangle_free( char const * name ) BOOST_NOEXCEPT
{
    std::free( const_cast< char* >( name ) );
}

inline std::string demangle( char const * name )
{
    scoped_demangled_name demangled_name( name );
    char const * const p = demangled_name.get();
    if( p )
    {
        return p;
    }
    else
    {
        return name;
    }
}

#else

inline char const * demangle_alloc( char const * name ) BOOST_NOEXCEPT
{
    return name;
}

inline void demangle_free( char const * ) BOOST_NOEXCEPT
{
}

inline std::string demangle( char const * name )
{
    return name;
}

#endif

} // namespace core

} // namespace boost

// Second part of work-around for Android, see above.
#ifdef BOOST_CORE_HAS_CXXABI_H
# undef BOOST_CORE_HAS_CXXABI_H
#endif

#endif // #ifndef BOOST_CORE_DEMANGLE_HPP_INCLUDED
