//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_618474C2DE1511DEB74A388C56D89593
#define UUID_618474C2DE1511DEB74A388C56D89593
#if defined(__GNUC__) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma GCC system_header
#endif
#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(push,1)
#endif

#include <boost/config.hpp>
#ifdef BOOST_NO_EXCEPTIONS
#error This header requires exception handling to be enabled.
#endif
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/detail/type_info.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <new>
#include <ios>

namespace
boost
    {
    typedef shared_ptr<exception_detail::clone_base const> exception_ptr;

    exception_ptr current_exception();

    template <class T>
    inline
    exception_ptr
    copy_exception( T const & e )
        {
        try
            {
            throw enable_current_exception(e);
            }
        catch(
        ... )
            {
            return current_exception();
            }
        }

#ifndef BOOST_NO_RTTI
    typedef error_info<struct tag_original_exception_type,std::type_info const *> original_exception_type;

    inline
    std::string
    to_string( original_exception_type const & x )
        {
        return x.value()->name();
        }
#endif

    namespace
    exception_detail
        {
        struct
        bad_alloc_:
            boost::exception,
            std::bad_alloc
                {
                };

        template <int Dummy>
        exception_ptr
        get_bad_alloc()
            {
            bad_alloc_ ba;
            exception_detail::clone_impl<bad_alloc_> c(ba);
            c <<
                throw_function(BOOST_CURRENT_FUNCTION) <<
                throw_file(__FILE__) <<
                throw_line(__LINE__);
            static exception_ptr ep(new exception_detail::clone_impl<bad_alloc_>(c));
            return ep;
            }

        template <int Dummy>
        struct
        exception_ptr_bad_alloc
            {
            static exception_ptr const e;
            };

        template <int Dummy>
        exception_ptr const
        exception_ptr_bad_alloc<Dummy>::
        e = get_bad_alloc<Dummy>();
        }

    class
    unknown_exception:
        public boost::exception,
        public std::exception
        {
        public:

        unknown_exception()
            {
            }

        explicit
        unknown_exception( std::exception const & e )
            {
            add_original_type(e);
            }

        explicit
        unknown_exception( boost::exception const & e ):
            boost::exception(e)
            {
            add_original_type(e);
            }

        ~unknown_exception() throw()
            {
            }

        private:

        template <class E>
        void
        add_original_type( E const & e )
            {
#ifndef BOOST_NO_RTTI
            (*this) << original_exception_type(&typeid(e));
#endif
            }
        };

    namespace
    exception_detail
        {
        template <class T>
        class
        current_exception_std_exception_wrapper:
            public T,
            public boost::exception
            {
            public:

            explicit
            current_exception_std_exception_wrapper( T const & e1 ):
                T(e1)
                {
                add_original_type(e1);
                }

            current_exception_std_exception_wrapper( T const & e1, boost::exception const & e2 ):
                T(e1),
                boost::exception(e2)
                {
                add_original_type(e1);
                }

            ~current_exception_std_exception_wrapper() throw()
                {
                }

            private:

            template <class E>
            void
            add_original_type( E const & e )
                {
#ifndef BOOST_NO_RTTI
                (*this) << original_exception_type(&typeid(e));
#endif
                }
            };

#ifdef BOOST_NO_RTTI
        template <class T>
        boost::exception const *
        get_boost_exception( T const * )
            {
            try
                {
                throw;
                }
            catch(
            boost::exception & x )
                {
                return &x;
                }
            catch(...)
                {
                return 0;
                }
            }
#else
        template <class T>
        boost::exception const *
        get_boost_exception( T const * x )
            {
            return dynamic_cast<boost::exception const *>(x);
            }
#endif

        template <class T>
        inline
        exception_ptr
        current_exception_std_exception( T const & e1 )
            {
            if( boost::exception const * e2 = get_boost_exception(&e1) )
                return boost::copy_exception(current_exception_std_exception_wrapper<T>(e1,*e2));
            else
                return boost::copy_exception(current_exception_std_exception_wrapper<T>(e1));
            }

        inline
        exception_ptr
        current_exception_unknown_exception()
            {
            return boost::copy_exception(unknown_exception());
            }

        inline
        exception_ptr
        current_exception_unknown_boost_exception( boost::exception const & e )
            {
            return boost::copy_exception(unknown_exception(e));
            }

        inline
        exception_ptr
        current_exception_unknown_std_exception( std::exception const & e )
            {
            if( boost::exception const * be = get_boost_exception(&e) )
                return current_exception_unknown_boost_exception(*be);
            else
                return boost::copy_exception(unknown_exception(e));
            }

        inline
        exception_ptr
        current_exception_impl()
            {
            try
                {
                throw;
                }
            catch(
            exception_detail::clone_base & e )
                {
                return exception_ptr(e.clone());
                }
            catch(
            std::domain_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::invalid_argument & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::length_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::out_of_range & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::logic_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::range_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::overflow_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::underflow_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::ios_base::failure & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::runtime_error & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::bad_alloc & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
#ifndef BOOST_NO_TYPEID
            catch(
            std::bad_cast & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::bad_typeid & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
#endif
            catch(
            std::bad_exception & e )
                {
                return exception_detail::current_exception_std_exception(e);
                }
            catch(
            std::exception & e )
                {
                return exception_detail::current_exception_unknown_std_exception(e);
                }
            catch(
            boost::exception & e )
                {
                return exception_detail::current_exception_unknown_boost_exception(e);
                }
            catch(
            ... )
                {
                return exception_detail::current_exception_unknown_exception();
                }
            }
        }

    inline
    exception_ptr
    current_exception()
        {
        exception_ptr ret;
        BOOST_ASSERT(!ret);
        try
            {
            ret=exception_detail::current_exception_impl();
            }
        catch(
        std::bad_alloc & )
            {
            ret=exception_detail::exception_ptr_bad_alloc<42>::e;
            }
        catch(
        ... )
            {
            try
                {
                ret=exception_detail::current_exception_std_exception(std::bad_exception());
                }
            catch(
            std::bad_alloc & )
                {
                ret=exception_detail::exception_ptr_bad_alloc<42>::e;
                }
            catch(
            ... )
                {
                BOOST_ASSERT(0);
                }
            }
        BOOST_ASSERT(ret);
        return ret;
        }

    inline
    void
    rethrow_exception( exception_ptr const & p )
        {
        BOOST_ASSERT(p);
        p->rethrow();
        }

    inline
    std::string
    diagnostic_information( exception_ptr const & p )
        {
        if( p )
            try
                {
                rethrow_exception(p);
                }
            catch(
            ... )
                {
                return current_exception_diagnostic_information();
                }
        return "<empty>";
        }

    inline
    std::string
    to_string( exception_ptr const & p )
        {
        std::string s='\n'+diagnostic_information(p);
        std::string padding("  ");
        std::string r;
        bool f=false;
        for( std::string::const_iterator i=s.begin(),e=s.end(); i!=e; ++i )
            {
            if( f )
                r+=padding;
            char c=*i;
            r+=c;
            f=(c=='\n');
            }
        return r;
        }
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
