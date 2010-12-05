//Copyright (c) 2006-2010 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_0552D49838DD11DD90146B8956D89593
#define UUID_0552D49838DD11DD90146B8956D89593
#if defined(__GNUC__) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma GCC system_header
#endif
#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(push,1)
#endif

#include <boost/config.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/utility/enable_if.hpp>
#ifndef BOOST_NO_RTTI
#include <boost/units/detail/utility.hpp>
#endif
#include <exception>
#include <sstream>
#include <string>

#ifndef BOOST_NO_EXCEPTIONS
#include <boost/exception/current_exception_cast.hpp>
namespace
boost
    {
    namespace
    exception_detail
        {
        std::string diagnostic_information_impl( boost::exception const *, std::exception const *, bool );
        }

    inline
    std::string
    current_exception_diagnostic_information()
        {
        boost::exception const * be=current_exception_cast<boost::exception const>();
        std::exception const * se=current_exception_cast<std::exception const>();
        if( be || se )
            return exception_detail::diagnostic_information_impl(be,se,true);
        else
            return "No diagnostic information available.";
        }
    }
#endif

namespace
boost
    {
    namespace
    exception_detail
        {
        inline
        exception const *
        get_boost_exception( exception const * e )
            {
            return e;
            }

        inline
        exception const *
        get_boost_exception( ... )
            {
            return 0;
            }

        inline
        std::exception const *
        get_std_exception( std::exception const * e )
            {
            return e;
            }

        inline
        std::exception const *
        get_std_exception( ... )
            {
            return 0;
            }

        inline
        char const *
        get_diagnostic_information( exception const & x, char const * header )
            {
            if( error_info_container * c=x.data_.get() )
#ifndef BOOST_NO_EXCEPTIONS
                try
                    {
#endif
                    return c->diagnostic_information(header);
#ifndef BOOST_NO_EXCEPTIONS
                    }
                catch(...)
                    {
                    }
#endif
            return 0;
            }

        inline
        std::string
        diagnostic_information_impl( boost::exception const * be, std::exception const * se, bool with_what )
            {
            if( !be && !se )
                return "Unknown exception.";
#ifndef BOOST_NO_RTTI
            if( !be )
                be=dynamic_cast<boost::exception const *>(se);
            if( !se )
                se=dynamic_cast<std::exception const *>(be);
#endif
            char const * wh=0;
            if( with_what && se )
                {
                wh=se->what();
                if( be && exception_detail::get_diagnostic_information(*be,0)==wh )
                    return wh;
                }
            std::ostringstream tmp;
            if( be )
                {
                if( char const * const * f=get_error_info<throw_file>(*be) )
                    {
                    tmp << *f;
                    if( int const * l=get_error_info<throw_line>(*be) )
                        tmp << '(' << *l << "): ";
                    }
                tmp << "Throw in function ";
                if( char const * const * fn=get_error_info<throw_function>(*be) )
                    tmp << *fn;
                else
                    tmp << "(unknown)";
                tmp << '\n';
                }
#ifndef BOOST_NO_RTTI
            tmp << std::string("Dynamic exception type: ") <<
                units::detail::demangle((be?BOOST_EXCEPTION_DYNAMIC_TYPEID(*be):BOOST_EXCEPTION_DYNAMIC_TYPEID(*se)).type_.name()) << '\n';
#endif
            if( with_what && se )
                tmp << "std::exception::what: " << wh << '\n';
            if( be )
                if( char const * s=exception_detail::get_diagnostic_information(*be,tmp.str().c_str()) )
                    if( *s )
                        return s;
            return tmp.str();
            }
        }

    template <class T>
    std::string
    diagnostic_information( T const & e )
        {
        return exception_detail::diagnostic_information_impl(exception_detail::get_boost_exception(&e),exception_detail::get_std_exception(&e),true);
        }

    inline
    char const *
    diagnostic_information_what( exception const & e ) throw()
        {
        char const * w=0;
#ifndef BOOST_NO_EXCEPTIONS
        try
            {
#endif
            (void) exception_detail::diagnostic_information_impl(&e,0,false);
            return exception_detail::get_diagnostic_information(e,0);
#ifndef BOOST_NO_EXCEPTIONS
            }
        catch(
        ... )
            {
            }
#endif
        return w;
        }
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
