/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEQUE_DETAIL_KEYED_ELEMENT_26112006_1330)
#define BOOST_FUSION_DEQUE_DETAIL_KEYED_ELEMENT_26112006_1330

#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>

#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/next.hpp>

namespace boost { namespace fusion {

    struct fusion_sequence_tag;

namespace detail {

    struct nil_keyed_element
    {
        typedef fusion_sequence_tag tag;
        void get();

        template<typename It>
        static nil_keyed_element 
        from_iterator(It const&)
        {
            return nil_keyed_element();
        }
    };

    template<typename Key, typename Value, typename Rest>
    struct keyed_element
        : Rest
    {
        typedef Rest base;
        typedef fusion_sequence_tag tag;
        using Rest::get;

        template<typename It>
        static keyed_element
        from_iterator(It const& it)
        {
            return keyed_element(
                *it, base::from_iterator(fusion::next(it)));
        }

        template<typename U, typename Rst>
        keyed_element(keyed_element<Key, U, Rst> const& rhs)
            : Rest(rhs.get_base()), value_(rhs.value_)
        {}

        Rest const get_base() const
        {
            return *this;
        }

        typename add_reference<typename add_const<Value>::type>::type get(Key) const
        {
            return value_;
        }

        typename add_reference<Value>::type get(Key)
        {
            return value_;
        }

        keyed_element(typename add_reference<typename add_const<Value>::type>::type value, Rest const& rest)
            : Rest(rest), value_(value)
        {}

        keyed_element()
            : Rest(), value_()
        {}

        template<typename U, typename Rst>
        keyed_element& operator=(keyed_element<Key, U, Rst> const& rhs)
        {
            base::operator=(static_cast<Rst const&>(rhs)); // cast for msvc-7.1
            value_ = rhs.value_;
            return *this;
        }

        keyed_element& operator=(keyed_element const& rhs)
        {
            base::operator=(rhs);
            value_ = rhs.value_;
            return *this;
        }

        Value value_;
    };

    template<typename Elem, typename Key>
    struct keyed_element_value_at
        : keyed_element_value_at<typename Elem::base, Key>
    {};

    template<typename Key, typename Value, typename Rest>
    struct keyed_element_value_at<keyed_element<Key, Value, Rest>, Key>
    {
        typedef Value type;
    };

}}}

#endif
