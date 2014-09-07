/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Carl Barron

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SYMBOLS_MARCH_11_2007_1055AM)
#define BOOST_SPIRIT_X3_SYMBOLS_MARCH_11_2007_1055AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/string/tst.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/traits/string_traits.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>

#include <boost/fusion/include/at.hpp>
#include <boost/range.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/shared_ptr.hpp>

#include <initializer_list>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list warning
#endif

namespace boost { namespace spirit { namespace x3
{
    template <
        typename Char = char
      , typename T = unused_type
      , typename Lookup = tst<Char, T>
      , typename Filter = tst_pass_through>
    struct symbols : parser<symbols<Char, T, Lookup, Filter>>
    {
        typedef Char char_type; // the character type
        typedef T value_type; // the value associated with each entry
        typedef symbols<Char, T, Lookup, Filter> this_type;
        typedef value_type attribute_type;

        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;
        static bool const handles_container =
            traits::is_container<attribute_type>::value;

        symbols(std::string const& name = "symbols")
          : add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
        }

        symbols(symbols const& syms)
          : add(*this)
          , remove(*this)
          , lookup(syms.lookup)
          , name_(syms.name_)
        {
        }

        template <typename Filter_>
        symbols(symbols<Char, T, Lookup, Filter_> const& syms)
          : add(*this)
          , remove(*this)
          , lookup(syms.lookup)
          , name_(syms.name_)
        {
        }

        template <typename Symbols>
        symbols(Symbols const& syms, std::string const& name = "symbols")
          : add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
            typename range_const_iterator<Symbols>::type si = boost::begin(syms);
            while (si != boost::end(syms))
                add(*si++);
        }

        template <typename Symbols, typename Data>
        symbols(Symbols const& syms, Data const& data
              , std::string const& name = "symbols")
          : add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
            typename range_const_iterator<Symbols>::type si = boost::begin(syms);
            typename range_const_iterator<Data>::type di = boost::begin(data);
            while (si != boost::end(syms))
                add(*si++, *di++);
        }

        symbols(std::initializer_list<std::pair<Char const*, T>> syms
              , std::string const & name="symbols")
          : add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
            typedef std::initializer_list<std::pair<Char const*, T>> symbols_t;
            typename range_const_iterator<symbols_t>::type si = boost::begin(syms);
            for (;si != boost::end(syms); ++si)
                add(si->first, si->second);
        }
        
        symbols(std::initializer_list<Char const*> syms
              , std::string const &name="symbols")
          : add(*this)
          , remove(*this)
          , lookup(new Lookup())
          , name_(name)
        {
            typedef std::initializer_list<Char const*> symbols_t;
            typename range_const_iterator<symbols_t>::type si = boost::begin(syms);
            while (si != boost::end(syms))
                add(*si++);
        }

        symbols&
        operator=(symbols const& rhs)
        {
            name_ = rhs.name_;
            lookup = rhs.lookup;
            return *this;
        }

        template <typename Filter_>
        symbols&
        operator=(symbols<Char, T, Lookup, Filter_> const& rhs)
        {
            name_ = rhs.name_;
            lookup = rhs.lookup;
            return *this;
        }

        void clear()
        {
            lookup->clear();
        }

        struct adder;
        struct remover;

        template <typename Str>
        adder const&
        operator=(Str const& str)
        {
            lookup->clear();
            return add(str);
        }

        template <typename Str>
        friend adder const&
        operator+=(symbols& sym, Str const& str)
        {
            return sym.add(str);
        }

        template <typename Str>
        friend remover const&
        operator-=(symbols& sym, Str const& str)
        {
            return sym.remove(str);
        }

        template <typename F>
        void for_each(F f) const
        {
            lookup->for_each(f);
        }

        template <typename Str>
        value_type& at(Str const& str)
        {
            return *lookup->add(traits::get_string_begin<Char>(str)
                , traits::get_string_end<Char>(str), T());
        }

        template <typename Iterator>
        value_type* prefix_find(Iterator& first, Iterator const& last)
        {
            return lookup->find(first, last, Filter());
        }

        template <typename Iterator>
        value_type const* prefix_find(Iterator& first, Iterator const& last) const
        {
            return lookup->find(first, last, Filter());
        }

        template <typename Str>
        value_type* find(Str const& str)
        {
            return find_impl(traits::get_string_begin<Char>(str)
                , traits::get_string_end<Char>(str));
        }

        template <typename Str>
        value_type const* find(Str const& str) const
        {
            return find_impl(traits::get_string_begin<Char>(str)
                , traits::get_string_end<Char>(str));
        }

    private:
        template <typename Iterator>
        value_type* find_impl(Iterator begin, Iterator end)
        {
            value_type* r = lookup->find(begin, end, Filter());
            return begin == end ? r : 0;
        }

        template <typename Iterator>
        value_type const* find_impl(Iterator begin, Iterator end) const
        {
            value_type const* r = lookup->find(begin, end, Filter());
            return begin == end ? r : 0;
        }

    public:
        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr) const
        {
            x3::skip_over(first, last, context);

            if (value_type* val_ptr
                = lookup->find(first, last, Filter()))
            {
                x3::traits::move_to(*val_ptr, attr);
                return true;
            }
            return false;
        }

        void name(std::string const &str)
        {
            name_ = str;
        }
        std::string const &name() const
        {
            return name_;
        }

        struct adder
        {
            template <typename, typename = unused_type, typename = unused_type>
            struct result { typedef adder const& type; };

            adder(symbols& sym)
              : sym(sym)
            {
            }

            template <typename Iterator>
            adder const&
            operator()(Iterator first, Iterator last, T const& val) const
            {
                sym.lookup->add(first, last, val);
                return *this;
            }

            template <typename Str>
            adder const&
            operator()(Str const& s, T const& val = T()) const
            {
                sym.lookup->add(traits::get_string_begin<Char>(s)
                  , traits::get_string_end<Char>(s), val);
                return *this;
            }

            template <typename Str>
            adder const&
            operator,(Str const& s) const
            {
                sym.lookup->add(traits::get_string_begin<Char>(s)
                  , traits::get_string_end<Char>(s), T());
                return *this;
            }

            symbols& sym;
        };

        struct remover
        {
            template <typename, typename = unused_type, typename = unused_type>
            struct result { typedef remover const& type; };

            remover(symbols& sym)
              : sym(sym)
            {
            }

            template <typename Iterator>
            remover const&
            operator()(Iterator const& first, Iterator const& last) const
            {
                sym.lookup->remove(first, last);
                return *this;
            }

            template <typename Str>
            remover const&
            operator()(Str const& s) const
            {
                sym.lookup->remove(traits::get_string_begin<Char>(s)
                  , traits::get_string_end<Char>(s));
                return *this;
            }

            template <typename Str>
            remover const&
            operator,(Str const& s) const
            {
                sym.lookup->remove(traits::get_string_begin<Char>(s)
                  , traits::get_string_end<Char>(s));
                return *this;
            }

            symbols& sym;
        };

        adder add;
        remover remove;
        shared_ptr<Lookup> lookup;
        std::string name_;
    };

    template <typename Char, typename T, typename Lookup, typename Filter>
    struct get_info<symbols<Char, T, Lookup, Filter>>
    {
      typedef std::string result_type;
      result_type operator()(symbols< Char, T
                                    , Lookup, Filter
                                    > const& symbols) const
      {
         return symbols.name();
      }
    };
}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
