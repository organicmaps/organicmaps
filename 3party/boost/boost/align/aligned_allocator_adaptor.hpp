/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNED_ALLOCATOR_ADAPTOR_HPP
#define BOOST_ALIGN_ALIGNED_ALLOCATOR_ADAPTOR_HPP

/**
 Class template aligned_allocator_adaptor.

 @file
 @author Glen Fernandes
*/

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/align/align.hpp>
#include <boost/align/aligned_allocator_adaptor_forward.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/align/detail/addressof.hpp>
#include <boost/align/detail/is_alignment_const.hpp>
#include <boost/align/detail/max_align.hpp>
#include <new>

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#include <memory>
#endif

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <utility>
#endif

/**
 Boost namespace.
*/
namespace boost {
    /**
     Alignment namespace.
    */
    namespace alignment {
        /**
         Class template aligned_allocator_adaptor.

         @tparam Alignment Is the minimum alignment to specify
           for allocations, if it is larger than the alignment
           of the value type. The value of `Alignment` shall be
           a fundamental alignment value or an extended alignment
           value, and shall be a power of two.

         @note This adaptor can be used with a C++11 allocator
           whose pointer type is a smart pointer but the adaptor
           will expose only raw pointers.
        */
        template<class Allocator, std::size_t Alignment>
        class aligned_allocator_adaptor
            : public Allocator {
            /**
             @cond
            */
            BOOST_STATIC_ASSERT(detail::
                is_alignment_const<Alignment>::value);
            /**
             @endcond
            */

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
            /**
             Exposition only.
            */
            typedef std::allocator_traits<Allocator> Traits;

            typedef typename Traits::
                template rebind_alloc<char> CharAlloc;

            typedef typename Traits::
                template rebind_traits<char> CharTraits;

            typedef typename CharTraits::pointer CharPtr;
#else
            typedef typename Allocator::
                template rebind<char>::other CharAlloc;

            typedef typename CharAlloc::pointer CharPtr;
#endif

        public:
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
            typedef typename Traits::value_type value_type;
            typedef typename Traits::size_type size_type;
#else
            typedef typename Allocator::value_type value_type;
            typedef typename Allocator::size_type size_type;
#endif

            typedef value_type* pointer;
            typedef const value_type* const_pointer;
            typedef void* void_pointer;
            typedef const void* const_void_pointer;
            typedef std::ptrdiff_t difference_type;

        private:
            enum {
                TypeAlign = alignment_of<value_type>::value,

                PtrAlign = alignment_of<CharPtr>::value,

                BlockAlign = detail::
                    max_align<PtrAlign, TypeAlign>::value,

                MaxAlign = detail::
                    max_align<Alignment, BlockAlign>::value
            };

        public:
            /**
             Rebind allocator.
            */
            template<class U>
            struct rebind {
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
                typedef aligned_allocator_adaptor<typename Traits::
                    template rebind_alloc<U>, Alignment> other;
#else
                typedef aligned_allocator_adaptor<typename Allocator::
                    template rebind<U>::other, Alignment> other;
#endif
            };

#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
            /**
             Value-initializes the `Allocator`
             base class.
            */
            aligned_allocator_adaptor() = default;
#else
            /**
             Value-initializes the `Allocator`
             base class.
            */
            aligned_allocator_adaptor()
                : Allocator() {
            }
#endif

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
            /**
             Initializes the `Allocator` base class with
             `std::forward<A>(alloc)`.

             @remark **Require:** `Allocator` shall be
               constructible from `A`.
            */
            template<class A>
            explicit aligned_allocator_adaptor(A&& alloc)
                BOOST_NOEXCEPT
                : Allocator(std::forward<A>(alloc)) {
            }
#else
            /**
             Initializes the `Allocator` base class with
             `alloc`.

             @remark **Require:** `Allocator` shall be
               constructible from `A`.
            */
            template<class A>
            explicit aligned_allocator_adaptor(const A& alloc)
                BOOST_NOEXCEPT
                : Allocator(alloc) {
            }
#endif

            /**
             Initializes the `Allocator` base class with the
             base from other.
            */
            template<class U>
            aligned_allocator_adaptor(const
                aligned_allocator_adaptor<U, Alignment>& other)
                BOOST_NOEXCEPT
                : Allocator(other.base()) {
            }

            /**
             @return `static_cast<Allocator&>(*this)`.
            */
            Allocator& base()
                BOOST_NOEXCEPT {
                return static_cast<Allocator&>(*this);
            }

            /**
             @return `static_cast<const Allocator&>(*this)`.
            */
            const Allocator& base() const
                BOOST_NOEXCEPT {
                return static_cast<const Allocator&>(*this);
            }

            /**
             @param size The size of the value type object to
               allocate.

             @return A pointer to the initial element of an
               array of storage of size `n * sizeof(value_type)`,
               aligned on the maximum of the minimum alignment
               specified and the alignment of objects of type
               `value_type`.

             @remark **Throw:** Throws an exception thrown from
               `A2::allocate` if the storage cannot be obtained.

             @remark **Note:** The storage is obtained by calling
               `A2::allocate` on an object `a2`, where `a2` of
               type `A2` is a rebound copy of `base()` where its
               `value_type` is unspecified.
            */
            pointer allocate(size_type size) {
                std::size_t n1 = size * sizeof(value_type);
                std::size_t n2 = n1 + MaxAlign - 1;
                CharAlloc a(base());
                CharPtr p1 = a.allocate(sizeof p1 + n2);
                void* p2 = detail::addressof(*p1) + sizeof p1;
                (void)align(MaxAlign, n1, p2, n2);
                void* p3 = static_cast<CharPtr*>(p2) - 1;
                ::new(p3) CharPtr(p1);
                return static_cast<pointer>(p2);
            }

            /**
             @param hint is a value obtained by calling
               `allocate()` on any equivalent aligned allocator
               adaptor object, or else `nullptr`.

             @param size The size of the value type object to
               allocate.

             @return A pointer to the initial element of an
               array of storage of size `n * sizeof(value_type)`,
               aligned on the maximum of the minimum alignment
               specified and the alignment of objects of type
               `value_type`.

             @remark **Throw:** Throws an exception thrown from
               `A2::allocate` if the storage cannot be obtained.

             @remark **Note:** The storage is obtained by calling
               `A2::allocate` on an object `a2`, where `a2` of
               type `A2` is a rebound copy of `base()` where its
               `value_type` is unspecified.
            */
            pointer allocate(size_type size, const_void_pointer hint) {
                std::size_t n1 = size * sizeof(value_type);
                std::size_t n2 = n1 + MaxAlign - 1;
                CharPtr h = CharPtr();
                if (hint) {
                    h = *(static_cast<const CharPtr*>(hint) - 1);
                }
                CharAlloc a(base());
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
                CharPtr p1 = CharTraits::allocate(a, sizeof p1 +
                    n2, h);
#else
                CharPtr p1 = a.allocate(sizeof p1 + n2, h);
#endif
                void* p2 = detail::addressof(*p1) + sizeof p1;
                (void)align(MaxAlign, n1, p2, n2);
                void* p3 = static_cast<CharPtr*>(p2) - 1;
                ::new(p3) CharPtr(p1);
                return static_cast<pointer>(p2);
            }

            /**
             Deallocates the storage referenced by `ptr`.

             @param ptr Shall be a pointer value obtained from
               `allocate()`.

             @param size Shall equal the value passed as the
               first argument to the invocation of `allocate`
               which returned `ptr`.

             @remark **Note:** Uses `A2::deallocate` on an object
               `a2`, where `a2` of type `A2` is a rebound copy of
               `base()` where its `value_type` is unspecified.
            */
            void deallocate(pointer ptr, size_type size) {
                CharPtr* p1 = reinterpret_cast<CharPtr*>(ptr) - 1;
                CharPtr p2 = *p1;
                p1->~CharPtr();
                CharAlloc a(base());
                a.deallocate(p2, size * sizeof(value_type) +
                    MaxAlign + sizeof p2);
            }
        };

        /**
         @return `a.base() == b.base()`.
        */
        template<class A1, class A2, std::size_t Alignment>
        inline bool operator==(const aligned_allocator_adaptor<A1,
            Alignment>& a, const aligned_allocator_adaptor<A2,
            Alignment>& b) BOOST_NOEXCEPT
        {
            return a.base() == b.base();
        }

        /**
         @return `!(a == b)`.
        */
        template<class A1, class A2, std::size_t Alignment>
        inline bool operator!=(const aligned_allocator_adaptor<A1,
            Alignment>& a, const aligned_allocator_adaptor<A2,
            Alignment>& b) BOOST_NOEXCEPT
        {
            return !(a == b);
        }
    }
}

#endif
