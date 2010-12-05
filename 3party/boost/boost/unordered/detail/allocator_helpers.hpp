
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A couple of templates to make using allocators easier.

#ifndef BOOST_UNORDERED_DETAIL_ALLOCATOR_UTILITIES_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_ALLOCATOR_UTILITIES_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/config.hpp>

#if (defined(BOOST_NO_STD_ALLOCATOR) || defined(BOOST_DINKUMWARE_STDLIB)) \
    && !defined(__BORLANDC__)
#  define BOOST_UNORDERED_USE_ALLOCATOR_UTILITIES
#endif

#if defined(BOOST_UNORDERED_USE_ALLOCATOR_UTILITIES)
#  include <boost/detail/allocator_utilities.hpp>
#endif

namespace boost { namespace unordered_detail {

    // rebind_wrap
    //
    // Rebind allocators. For some problematic libraries, use rebind_to
    // from <boost/detail/allocator_utilities.hpp>.

#if defined(BOOST_UNORDERED_USE_ALLOCATOR_UTILITIES)
    template <class Alloc, class T>
    struct rebind_wrap : ::boost::detail::allocator::rebind_to<Alloc, T> {};
#else
    template <class Alloc, class T>
    struct rebind_wrap
    {
        typedef BOOST_DEDUCED_TYPENAME
            Alloc::BOOST_NESTED_TEMPLATE rebind<T>::other
            type;
    };
#endif

    // allocator_array_constructor
    //
    // Allocate and construct an array in an exception safe manner, and
    // clean up if an exception is thrown before the container takes charge
    // of it.

    template <class Allocator>
    struct allocator_array_constructor
    {
        typedef BOOST_DEDUCED_TYPENAME Allocator::pointer pointer;

        Allocator& alloc_;
        pointer ptr_;
        pointer constructed_;
        std::size_t length_;

        allocator_array_constructor(Allocator& a)
            : alloc_(a), ptr_(), constructed_(), length_(0)
        {
            constructed_ = pointer();
            ptr_ = pointer();
        }

        ~allocator_array_constructor() {
            if (ptr_) {
                for(pointer p = ptr_; p != constructed_; ++p)
                    alloc_.destroy(p);

                alloc_.deallocate(ptr_, length_);
            }
        }

        template <class V>
        void construct(V const& v, std::size_t l)
        {
            BOOST_ASSERT(!ptr_);
            length_ = l;
            ptr_ = alloc_.allocate(length_);
            pointer end = ptr_ + static_cast<std::ptrdiff_t>(length_);
            for(constructed_ = ptr_; constructed_ != end; ++constructed_)
                alloc_.construct(constructed_, v);
        }

        pointer get() const
        {
            return ptr_;
        }

        pointer release()
        {
            pointer p(ptr_);
            ptr_ = pointer();
            return p;
        }
    private:
        allocator_array_constructor(allocator_array_constructor const&);
        allocator_array_constructor& operator=(
            allocator_array_constructor const&);
    };
}}

#if defined(BOOST_UNORDERED_USE_ALLOCATOR_UTILITIES)
#  undef BOOST_UNORDERED_USE_ALLOCATOR_UTILITIES
#endif

#endif
