/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNED_ALLOCATOR_HPP
#define BOOST_ALIGN_ALIGNED_ALLOCATOR_HPP

/**
 Class template aligned_allocator.

 @file
 @author Glen Fernandes
*/

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/align/aligned_alloc.hpp>
#include <boost/align/aligned_allocator_forward.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/align/detail/addressof.hpp>
#include <boost/align/detail/is_alignment_const.hpp>
#include <boost/align/detail/max_align.hpp>
#include <boost/align/detail/max_count_of.hpp>
#include <new>

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
         Class template aligned_allocator.

         @tparam Alignment Is the minimum alignment to specify
           for allocations, if it is larger than the alignment
           of the value type. It shall be a power of two.

         @remark **Note:** Except for the destructor, member
           functions of the aligned allocator shall not
           introduce data races as a result of concurrent calls
           to those member functions from different threads.
           Calls to these functions that allocate or deallocate
           a particular unit of storage shall occur in a single
           total order, and each such deallocation call shall
           happen before the next allocation (if any) in this
           order.

         @note Specifying minimum alignment is generally only
           suitable for containers such as vector and undesirable
           with other, node-based, containers. For node-based
           containers, such as list, the node object would have
           the minimum alignment specified instead of the value
           type object.
        */
        template<class T, std::size_t Alignment>
        class aligned_allocator {
            /**
             @cond
            */
            BOOST_STATIC_ASSERT(detail::
                is_alignment_const<Alignment>::value);
            /**
             @endcond
            */

        public:
            typedef T value_type;
            typedef T* pointer;
            typedef const T* const_pointer;
            typedef void* void_pointer;
            typedef const void* const_void_pointer;
            typedef std::size_t size_type;
            typedef std::ptrdiff_t difference_type;
            typedef T& reference;
            typedef const T& const_reference;

        private:
            enum {
                TypeAlign = alignment_of<value_type>::value,

                MaxAlign = detail::
                    max_align<Alignment, TypeAlign>::value
            };

        public:
            /**
             Rebind allocator.
            */
            template<class U>
            struct rebind {
                typedef aligned_allocator<U, Alignment> other;
            };

#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
            aligned_allocator()
                BOOST_NOEXCEPT = default;
#else
            aligned_allocator()
                BOOST_NOEXCEPT {
            }
#endif

            template<class U>
            aligned_allocator(const aligned_allocator<U,
                Alignment>&) BOOST_NOEXCEPT {
            }

            /**
             @return The actual address of the object referenced
               by `value`, even in the presence of an overloaded
               operator&.
            */
            pointer address(reference value) const
                BOOST_NOEXCEPT {
                return detail::addressof(value);
            }

            /**
             @return The actual address of the object referenced
               by `value`, even in the presence of an overloaded
               operator&.
            */
            const_pointer address(const_reference value) const
                BOOST_NOEXCEPT {
                return detail::addressof(value);
            }

            /**
             @return A pointer to the initial element of an array
               of storage of size `n * sizeof(T)`, aligned on the
               maximum of the minimum alignment specified and the
               alignment of objects of type `T`.

             @remark **Throw:** Throws `std::bad_alloc` if the
               storage cannot be obtained.

             @remark **Note:** The storage is obtained by calling
               `aligned_alloc(std::size_t, std::size_t)`.
            */
            pointer allocate(size_type size, const_void_pointer = 0) {
                void* p = aligned_alloc(MaxAlign, sizeof(T) * size);
                if (!p && size > 0) {
                    boost::throw_exception(std::bad_alloc());
                }
                return static_cast<T*>(p);
            }

            /**
             Deallocates the storage referenced by `ptr`.

             @param ptr Shall be a pointer value obtained from
               `allocate()`.

             @remark **Note:** Uses
               `alignment::aligned_free(void*)`.
            */
            void deallocate(pointer ptr, size_type) {
                alignment::aligned_free(ptr);
            }

            /**
             @return The largest value `N` for which the call
               `allocate(N)` might succeed.
            */
            BOOST_CONSTEXPR size_type max_size() const
                BOOST_NOEXCEPT {
                return detail::max_count_of<T>::value;
            }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
            /**
             Calls global
             `new((void*)ptr) U(std::forward<Args>(args)...)`.
            */
            template<class U, class... Args>
            void construct(U* ptr, Args&&... args) {
                void* p = ptr;
                ::new(p) U(std::forward<Args>(args)...);
            }
#else
            /**
             Calls global
             `new((void*)ptr) U(std::forward<V>(value))`.
            */
            template<class U, class V>
            void construct(U* ptr, V&& value) {
                void* p = ptr;
                ::new(p) U(std::forward<V>(value));
            }
#endif
#else
            /**
             Calls global `new((void*)ptr) U(value)`.
            */
            template<class U, class V>
            void construct(U* ptr, const V& value) {
                void* p = ptr;
                ::new(p) U(value);
            }
#endif

            /**
             Calls global `new((void*)ptr) U()`.
            */
            template<class U>
            void construct(U* ptr) {
                void* p = ptr;
                ::new(p) U();
            }

            /**
             Calls `ptr->~U()`.
            */
            template<class U>
            void destroy(U* ptr) {
                (void)ptr;
                ptr->~U();
            }
        };

        /**
         Class template aligned_allocator
         specialization.
        */
        template<std::size_t Alignment>
        class aligned_allocator<void, Alignment> {
            /**
             @cond
            */
            BOOST_STATIC_ASSERT(detail::
                is_alignment_const<Alignment>::value);
            /**
             @endcond
            */

        public:
            typedef void value_type;
            typedef void* pointer;
            typedef const void* const_pointer;

            /**
             Rebind allocator.
            */
            template<class U>
            struct rebind {
                typedef aligned_allocator<U, Alignment> other;
            };
        };

        /**
         @return `true`.
        */
        template<class T1, class T2, std::size_t Alignment>
        inline bool operator==(const aligned_allocator<T1,
            Alignment>&, const aligned_allocator<T2,
            Alignment>&) BOOST_NOEXCEPT
        {
            return true;
        }

        /**
         @return `false`.
        */
        template<class T1, class T2, std::size_t Alignment>
        inline bool operator!=(const aligned_allocator<T1,
            Alignment>&, const aligned_allocator<T2,
            Alignment>&) BOOST_NOEXCEPT
        {
            return false;
        }
    }
}

#endif
