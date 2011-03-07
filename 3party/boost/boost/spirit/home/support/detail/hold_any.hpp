/*=============================================================================
    Copyright (c) 2007-2011 Hartmut Kaiser
    Copyright (c) Christopher Diggins 2005
    Copyright (c) Pablo Aguilar 2005
    Copyright (c) Kevlin Henney 2001

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    The class boost::spirit::hold_any is built based on the any class
    published here: http://www.codeproject.com/cpp/dynamic_typing.asp. It adds
    support for std streaming operator<<() and operator>>().
==============================================================================*/
#if !defined(BOOST_SPIRIT_HOLD_ANY_MAY_02_2007_0857AM)
#define BOOST_SPIRIT_HOLD_ANY_MAY_02_2007_0857AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/assert.hpp>
#include <boost/detail/sp_typeinfo.hpp>

#include <stdexcept>
#include <typeinfo>
#include <algorithm>
#include <iosfwd>

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(push)
# pragma warning(disable: 4100)   // 'x': unreferenced formal parameter
# pragma warning(disable: 4127)   // conditional expression is constant
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    struct bad_any_cast
      : std::bad_cast
    {
        bad_any_cast(boost::detail::sp_typeinfo const& src, boost::detail::sp_typeinfo const& dest)
          : from(src.name()), to(dest.name())
        {}

        virtual const char* what() const throw() { return "bad any cast"; }

        const char* from;
        const char* to;
    };

    namespace detail
    {
        // function pointer table
        struct fxn_ptr_table
        {
            boost::detail::sp_typeinfo const& (*get_type)();
            void (*static_delete)(void**);
            void (*destruct)(void**);
            void (*clone)(void* const*, void**);
            void (*move)(void* const*, void**);
            std::istream& (*stream_in)(std::istream&, void**);
            std::ostream& (*stream_out)(std::ostream&, void* const*);
        };

        // static functions for small value-types
        template<typename Small>
        struct fxns;

        template<>
        struct fxns<mpl::true_>
        {
            template<typename T>
            struct type
            {
                static boost::detail::sp_typeinfo const& get_type()
                {
                    return BOOST_SP_TYPEID(T);
                }
                static void static_delete(void** x)
                {
                    reinterpret_cast<T*>(x)->~T();
                }
                static void destruct(void** x)
                {
                    reinterpret_cast<T*>(x)->~T();
                }
                static void clone(void* const* src, void** dest)
                {
                    new (dest) T(*reinterpret_cast<T const*>(src));
                }
                static void move(void* const* src, void** dest)
                {
                    reinterpret_cast<T*>(dest)->~T();
                    *reinterpret_cast<T*>(dest) =
                        *reinterpret_cast<T const*>(src);
                }
                static std::istream& stream_in (std::istream& i, void** obj)
                {
                    i >> *reinterpret_cast<T*>(obj);
                    return i;
                }
                static std::ostream& stream_out(std::ostream& o, void* const* obj)
                {
                    o << *reinterpret_cast<T const*>(obj);
                    return o;
                }
            };
        };

        // static functions for big value-types (bigger than a void*)
        template<>
        struct fxns<mpl::false_>
        {
            template<typename T>
            struct type
            {
                static boost::detail::sp_typeinfo const& get_type()
                {
                    return BOOST_SP_TYPEID(T);
                }
                static void static_delete(void** x)
                {
                    // destruct and free memory
                    delete (*reinterpret_cast<T**>(x));
                }
                static void destruct(void** x)
                {
                    // destruct only, we'll reuse memory
                    (*reinterpret_cast<T**>(x))->~T();
                }
                static void clone(void* const* src, void** dest)
                {
                    *dest = new T(**reinterpret_cast<T* const*>(src));
                }
                static void move(void* const* src, void** dest)
                {
                    (*reinterpret_cast<T**>(dest))->~T();
                    **reinterpret_cast<T**>(dest) =
                        **reinterpret_cast<T* const*>(src);
                }
                static std::istream& stream_in(std::istream& i, void** obj)
                {
                    i >> **reinterpret_cast<T**>(obj);
                    return i;
                }
                static std::ostream& stream_out(std::ostream& o, void* const* obj)
                {
                    o << **reinterpret_cast<T* const*>(obj);
                    return o;
                }
            };
        };

        template<typename T>
        struct get_table
        {
            typedef mpl::bool_<(sizeof(T) <= sizeof(void*))> is_small;

            static fxn_ptr_table* get()
            {
                static fxn_ptr_table static_table =
                {
                    fxns<is_small>::template type<T>::get_type,
                    fxns<is_small>::template type<T>::static_delete,
                    fxns<is_small>::template type<T>::destruct,
                    fxns<is_small>::template type<T>::clone,
                    fxns<is_small>::template type<T>::move,
                    fxns<is_small>::template type<T>::stream_in,
                    fxns<is_small>::template type<T>::stream_out
                };
                return &static_table;
            }
        };

        ///////////////////////////////////////////////////////////////////////
        struct empty {};

        inline std::istream&
        operator>> (std::istream& i, empty&)
        {
            // If this assertion fires you tried to insert from a std istream
            // into an empty hold_any instance. This simply can't work, because
            // there is no way to figure out what type to extract from the
            // stream.
            // The only way to make this work is to assign an arbitrary
            // value of the required type to the hold_any instance you want to
            // stream to. This assignment has to be executed before the actual
            // call to the operator>>().
            BOOST_ASSERT(false && 
                "Tried to insert from a std istream into an empty "
                "hold_any instance");
            return i;
        }

        inline std::ostream&
        operator<< (std::ostream& o, empty const&)
        {
            return o;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    class hold_any
    {
    public:
        // constructors
        template <typename T>
        explicit hold_any(T const& x)
          : table(spirit::detail::get_table<T>::get()), object(0)
        {
            if (spirit::detail::get_table<T>::is_small::value)
                new (&object) T(x);
            else
                object = new T(x);
        }

        hold_any()
          : table(spirit::detail::get_table<spirit::detail::empty>::get()),
            object(0)
        {
        }

        hold_any(hold_any const& x)
          : table(spirit::detail::get_table<spirit::detail::empty>::get()),
            object(0)
        {
            assign(x);
        }

        ~hold_any()
        {
            table->static_delete(&object);
        }

        // assignment
        hold_any& assign(hold_any const& x)
        {
            if (&x != this) {
                // are we copying between the same type?
                if (table == x.table) {
                    // if so, we can avoid reallocation
                    table->move(&x.object, &object);
                }
                else {
                    reset();
                    x.table->clone(&x.object, &object);
                    table = x.table;
                }
            }
            return *this;
        }

        template <typename T>
        hold_any& assign(T const& x)
        {
            // are we copying between the same type?
            spirit::detail::fxn_ptr_table* x_table =
                spirit::detail::get_table<T>::get();
            if (table == x_table) {
            // if so, we can avoid deallocating and re-use memory
                table->destruct(&object);    // first destruct the old content
                if (spirit::detail::get_table<T>::is_small::value) {
                    // create copy on-top of object pointer itself
                    new (&object) T(x);
                }
                else {
                    // create copy on-top of old version
                    new (object) T(x);
                }
            }
            else {
                if (spirit::detail::get_table<T>::is_small::value) {
                    // create copy on-top of object pointer itself
                    table->destruct(&object); // first destruct the old content
                    new (&object) T(x);
                }
                else {
                    reset();                  // first delete the old content
                    object = new T(x);
                }
                table = x_table;      // update table pointer
            }
            return *this;
        }

        // assignment operator
        template <typename T>
        hold_any& operator=(T const& x)
        {
            return assign(x);
        }

        // utility functions
        hold_any& swap(hold_any& x)
        {
            std::swap(table, x.table);
            std::swap(object, x.object);
            return *this;
        }

        boost::detail::sp_typeinfo const& type() const
        {
            return table->get_type();
        }

        template <typename T>
        T const& cast() const
        {
            if (type() != BOOST_SP_TYPEID(T))
              throw bad_any_cast(type(), BOOST_SP_TYPEID(T));

            return spirit::detail::get_table<T>::is_small::value ?
                *reinterpret_cast<T const*>(&object) :
                *reinterpret_cast<T const*>(object);
        }

// implicit casting is disabled by default for compatibility with boost::any
#ifdef BOOST_SPIRIT_ANY_IMPLICIT_CASTING
        // automatic casting operator
        template <typename T>
        operator T const& () const { return cast<T>(); }
#endif // implicit casting

        bool empty() const
        {
            return table == spirit::detail::get_table<spirit::detail::empty>::get();
        }

        void reset()
        {
            if (!empty())
            {
                table->static_delete(&object);
                table = spirit::detail::get_table<spirit::detail::empty>::get();
                object = 0;
            }
        }

    // these functions have been added in the assumption that the embedded
    // type has a corresponding operator defined, which is completely safe
    // because spirit::hold_any is used only in contexts where these operators
    // do exist
        friend std::istream& operator>> (std::istream& i, hold_any& obj)
        {
            return obj.table->stream_in(i, &obj.object);
        }

        friend std::ostream& operator<< (std::ostream& o, hold_any const& obj)
        {
            return obj.table->stream_out(o, &obj.object);
        }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    private: // types
        template<typename T>
        friend T* any_cast(hold_any *);
#else
    public: // types (public so any_cast can be non-friend)
#endif
        // fields
        spirit::detail::fxn_ptr_table* table;
        void* object;
    };

    // boost::any-like casting
    template <typename T>
    inline T* any_cast (hold_any* operand)
    {
        if (operand && operand->type() == BOOST_SP_TYPEID(T)) {
            return spirit::detail::get_table<T>::is_small::value ?
                reinterpret_cast<T*>(&operand->object) :
                reinterpret_cast<T*>(operand->object);
        }
        return 0;
    }

    template <typename T>
    inline T const* any_cast(hold_any const* operand)
    {
        return any_cast<T>(const_cast<hold_any*>(operand));
    }

    template <typename T>
    T any_cast(hold_any& operand)
    {
        typedef BOOST_DEDUCED_TYPENAME remove_reference<T>::type nonref;

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        // If 'nonref' is still reference type, it means the user has not
        // specialized 'remove_reference'.

        // Please use BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION macro
        // to generate specialization of remove_reference for your class
        // See type traits library documentation for details
        BOOST_STATIC_ASSERT(!is_reference<nonref>::value);
#endif

        nonref* result = any_cast<nonref>(&operand);
        if(!result)
            boost::throw_exception(bad_any_cast(operand.type(), BOOST_SP_TYPEID(T)));
        return *result;
    }

    template <typename T>
    T const& any_cast(hold_any const& operand)
    {
        typedef BOOST_DEDUCED_TYPENAME remove_reference<T>::type nonref;

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        // The comment in the above version of 'any_cast' explains when this
        // assert is fired and what to do.
        BOOST_STATIC_ASSERT(!is_reference<nonref>::value);
#endif

        return any_cast<nonref const&>(const_cast<hold_any &>(operand));
    }

///////////////////////////////////////////////////////////////////////////////
}}    // namespace boost::spirit

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
# pragma warning(pop)
#endif

#endif
