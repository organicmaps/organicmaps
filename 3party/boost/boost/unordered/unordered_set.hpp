
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/unordered for documentation

#ifndef BOOST_UNORDERED_UNORDERED_SET_HPP_INCLUDED
#define BOOST_UNORDERED_UNORDERED_SET_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/unordered/unordered_set_fwd.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered/detail/allocator_helpers.hpp>
#include <boost/unordered/detail/equivalent.hpp>
#include <boost/unordered/detail/unique.hpp>

#if defined(BOOST_NO_RVALUE_REFERENCES)
#include <boost/unordered/detail/move.hpp>
#endif

#if !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)
#if BOOST_MSVC >= 1400
#pragma warning(disable:4396) //the inline specifier cannot be used when a
                              // friend declaration refers to a specialization
                              // of a function template
#endif
#endif

namespace boost
{
    template <class T, class H, class P, class A>
    class unordered_set
    {
    public:

        typedef T key_type;
        typedef T value_type;
        typedef H hasher;
        typedef P key_equal;
        typedef A allocator_type;

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0582)
    private:
#endif

        typedef BOOST_DEDUCED_TYPENAME
            boost::unordered_detail::rebind_wrap<
                allocator_type, value_type>::type
            value_allocator;

        typedef boost::unordered_detail::set<H, P,
            value_allocator> types;
        typedef BOOST_DEDUCED_TYPENAME types::impl table;

        typedef BOOST_DEDUCED_TYPENAME types::iterator_base iterator_base;

    public:

        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::pointer pointer;
        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::const_pointer const_pointer;
        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::reference reference;
        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::const_reference const_reference;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        typedef boost::unordered_detail::hash_const_local_iterator<
            value_allocator, boost::unordered_detail::ungrouped>
                const_local_iterator;
        typedef boost::unordered_detail::hash_const_iterator<
            value_allocator, boost::unordered_detail::ungrouped>
                const_iterator;
        typedef const_local_iterator local_iterator;
        typedef const_iterator iterator;

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0582)
    private:
#endif

        table table_;
        
        BOOST_DEDUCED_TYPENAME types::iterator_base const&
            get(const_iterator const& it)
        {
            return boost::unordered_detail::iterator_access::get(it);
        }

    public:

        // construct/destroy/copy

        explicit unordered_set(
                size_type n = boost::unordered_detail::default_bucket_count,
                const hasher &hf = hasher(),
                const key_equal &eql = key_equal(),
                const allocator_type &a = allocator_type())
          : table_(n, hf, eql, a)
        {
        }

        explicit unordered_set(allocator_type const& a)
          : table_(boost::unordered_detail::default_bucket_count,
                hasher(), key_equal(), a)
        {
        }

        unordered_set(unordered_set const& other, allocator_type const& a)
          : table_(other.table_, a)
        {
        }

        template <class InputIt>
        unordered_set(InputIt f, InputIt l)
          : table_(boost::unordered_detail::initial_size(f, l),
            hasher(), key_equal(), allocator_type())
        {
            table_.insert_range(f, l);
        }

        template <class InputIt>
        unordered_set(InputIt f, InputIt l, size_type n,
                const hasher &hf = hasher(),
                const key_equal &eql = key_equal())
          : table_(boost::unordered_detail::initial_size(f, l, n),
            hf, eql, allocator_type())
        {
            table_.insert_range(f, l);
        }
        
        template <class InputIt>
        unordered_set(InputIt f, InputIt l, size_type n,
                const hasher &hf,
                const key_equal &eql,
                const allocator_type &a)
          : table_(boost::unordered_detail::initial_size(f, l, n), hf, eql, a)
        {
            table_.insert_range(f, l);
        }
        
        ~unordered_set() {}

#if !defined(BOOST_NO_RVALUE_REFERENCES)
        unordered_set(unordered_set&& other)
          : table_(other.table_, boost::unordered_detail::move_tag())
        {
        }

        unordered_set(unordered_set&& other, allocator_type const& a)
          : table_(other.table_, a, boost::unordered_detail::move_tag())
        {
        }

        unordered_set& operator=(unordered_set&& x)
        {
            table_.move(x.table_);
            return *this;
        }
#else
        unordered_set(boost::unordered_detail::move_from<
                unordered_set<T, H, P, A>
            > other)
          : table_(other.source.table_, boost::unordered_detail::move_tag())
        {
        }

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0593)
        unordered_set& operator=(unordered_set x)
        {
            table_.move(x.table_);
            return *this;
        }
#endif
#endif

#if !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
        unordered_set(std::initializer_list<value_type> list,
                size_type n = boost::unordered_detail::default_bucket_count,
                const hasher &hf = hasher(),
                const key_equal &eql = key_equal(),
                const allocator_type &a = allocator_type())
          : table_(boost::unordered_detail::initial_size(
                    list.begin(), list.end(), n),
                hf, eql, a)
        {
            table_.insert_range(list.begin(), list.end());
        }

        unordered_set& operator=(std::initializer_list<value_type> list)
        {
            table_.clear();
            table_.insert_range(list.begin(), list.end());
            return *this;
        }
#endif

        allocator_type get_allocator() const
        {
            return table_.node_alloc();
        }

        // size and capacity

        bool empty() const
        {
            return table_.size_ == 0;
        }

        size_type size() const
        {
            return table_.size_;
        }

        size_type max_size() const
        {
            return table_.max_size();
        }

        // iterators

        iterator begin()
        {
            return iterator(table_.begin());
        }

        const_iterator begin() const
        {
            return const_iterator(table_.begin());
        }

        iterator end()
        {
            return iterator(table_.end());
        }

        const_iterator end() const
        {
            return const_iterator(table_.end());
        }

        const_iterator cbegin() const
        {
            return const_iterator(table_.begin());
        }

        const_iterator cend() const
        {
            return const_iterator(table_.end());
        }

        // modifiers

#if defined(BOOST_UNORDERED_STD_FORWARD)
        template <class... Args>
        std::pair<iterator, bool> emplace(Args&&... args)
        {
            return boost::unordered_detail::pair_cast<iterator, bool>(
                table_.emplace(std::forward<Args>(args)...));
        }

        template <class... Args>
        iterator emplace_hint(const_iterator, Args&&... args)
        {
            return iterator(table_.emplace(std::forward<Args>(args)...).first);
        }
#else

        std::pair<iterator, bool> emplace(value_type const& v = value_type())
        {
            return boost::unordered_detail::pair_cast<iterator, bool>(
                table_.emplace(v));
        }

        iterator emplace_hint(const_iterator,
            value_type const& v = value_type())
        {
            return iterator(table_.emplace(v).first);
        }

#define BOOST_UNORDERED_EMPLACE(z, n, _)                                       \
            template <                                                         \
                BOOST_UNORDERED_TEMPLATE_ARGS(z, n)                            \
            >                                                                  \
            std::pair<iterator, bool> emplace(                                 \
                BOOST_UNORDERED_FUNCTION_PARAMS(z, n)                          \
            )                                                                  \
            {                                                                  \
                return boost::unordered_detail::pair_cast<iterator, bool>(     \
                    table_.emplace(                                            \
                        BOOST_UNORDERED_CALL_PARAMS(z, n)                      \
                    ));                                                        \
            }                                                                  \
                                                                               \
            template <                                                         \
                BOOST_UNORDERED_TEMPLATE_ARGS(z, n)                            \
            >                                                                  \
            iterator emplace_hint(const_iterator,                              \
                BOOST_UNORDERED_FUNCTION_PARAMS(z, n)                          \
            )                                                                  \
            {                                                                  \
                return iterator(table_.emplace(                                \
                    BOOST_UNORDERED_CALL_PARAMS(z, n)).first);                 \
            }

        BOOST_PP_REPEAT_FROM_TO(1, BOOST_UNORDERED_EMPLACE_LIMIT,
            BOOST_UNORDERED_EMPLACE, _)

#undef BOOST_UNORDERED_EMPLACE

#endif

        std::pair<iterator, bool> insert(const value_type& obj)
        {
            return boost::unordered_detail::pair_cast<iterator, bool>(
                    table_.emplace(obj));
        }

        iterator insert(const_iterator, const value_type& obj)
        {
            return iterator(table_.emplace(obj).first);
        }

        template <class InputIt>
            void insert(InputIt first, InputIt last)
        {
            table_.insert_range(first, last);
        }

#if !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
        void insert(std::initializer_list<value_type> list)
        {
            table_.insert_range(list.begin(), list.end());
        }
#endif

        iterator erase(const_iterator position)
        {
            return iterator(table_.erase_return_iterator(get(position)));
        }

        size_type erase(const key_type& k)
        {
            return table_.erase_key(k);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            return iterator(table_.erase_range(get(first), get(last)));
        }

        void quick_erase(const_iterator position)
        {
            table_.erase(get(position));
        }

        void erase_return_void(const_iterator position)
        {
            table_.erase(get(position));
        }

        void clear()
        {
            table_.clear();
        }

        void swap(unordered_set& other)
        {
            table_.swap(other.table_);
        }

        // observers

        hasher hash_function() const
        {
            return table_.hash_function();
        }

        key_equal key_eq() const
        {
            return table_.key_eq();
        }

        // lookup

        const_iterator find(const key_type& k) const
        {
            return const_iterator(table_.find(k));
        }

        template <class CompatibleKey, class CompatibleHash,
            class CompatiblePredicate>
        const_iterator find(
            CompatibleKey const& k,
            CompatibleHash const& hash,
            CompatiblePredicate const& eq) const
        {
            return iterator(table_.find(k, hash, eq));
        }
        size_type count(const key_type& k) const
        {
            return table_.count(k);
        }

        std::pair<const_iterator, const_iterator>
            equal_range(const key_type& k) const
        {
            return boost::unordered_detail::pair_cast<
                const_iterator, const_iterator>(
                    table_.equal_range(k));
        }

        // bucket interface

        size_type bucket_count() const
        {
            return table_.bucket_count_;
        }

        size_type max_bucket_count() const
        {
            return table_.max_bucket_count();
        }

        size_type bucket_size(size_type n) const
        {
            return table_.bucket_size(n);
        }

        size_type bucket(const key_type& k) const
        {
            return table_.bucket_index(k);
        }

        local_iterator begin(size_type n)
        {
            return local_iterator(table_.bucket_begin(n));
        }

        const_local_iterator begin(size_type n) const
        {
            return const_local_iterator(table_.bucket_begin(n));
        }

        local_iterator end(size_type)
        {
            return local_iterator();
        }

        const_local_iterator end(size_type) const
        {
            return const_local_iterator();
        }

        const_local_iterator cbegin(size_type n) const
        {
            return const_local_iterator(table_.bucket_begin(n));
        }

        const_local_iterator cend(size_type) const
        {
            return const_local_iterator();
        }

        // hash policy

        float load_factor() const
        {
            return table_.load_factor();
        }

        float max_load_factor() const
        {
            return table_.mlf_;
        }

        void max_load_factor(float m)
        {
            table_.max_load_factor(m);
        }

        void rehash(size_type n)
        {
            table_.rehash(n);
        }

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0582)
        friend bool operator==<T, H, P, A>(
            unordered_set const&, unordered_set const&);
        friend bool operator!=<T, H, P, A>(
            unordered_set const&, unordered_set const&);
#endif
    }; // class template unordered_set

    template <class T, class H, class P, class A>
    inline bool operator==(unordered_set<T, H, P, A> const& m1,
        unordered_set<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x0613))
        struct dummy { unordered_set<T,H,P,A> x; };
#endif
        return m1.table_.equals(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline bool operator!=(unordered_set<T, H, P, A> const& m1,
        unordered_set<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x0613))
        struct dummy { unordered_set<T,H,P,A> x; };
#endif
        return !m1.table_.equals(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline void swap(unordered_set<T, H, P, A> &m1,
            unordered_set<T, H, P, A> &m2)
    {
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x0613))
        struct dummy { unordered_set<T,H,P,A> x; };
#endif
        m1.swap(m2);
    }

    template <class T, class H, class P, class A>
    class unordered_multiset
    {
    public:

        typedef T key_type;
        typedef T value_type;
        typedef H hasher;
        typedef P key_equal;
        typedef A allocator_type;

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0582)
    private:
#endif

        typedef BOOST_DEDUCED_TYPENAME
            boost::unordered_detail::rebind_wrap<
                allocator_type, value_type>::type
            value_allocator;

        typedef boost::unordered_detail::multiset<H, P,
            value_allocator> types;
        typedef BOOST_DEDUCED_TYPENAME types::impl table;

        typedef BOOST_DEDUCED_TYPENAME types::iterator_base iterator_base;

    public:

        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::pointer pointer;
        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::const_pointer const_pointer;
        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::reference reference;
        typedef BOOST_DEDUCED_TYPENAME
            value_allocator::const_reference const_reference;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        typedef boost::unordered_detail::hash_const_local_iterator<
            value_allocator, boost::unordered_detail::grouped>
                const_local_iterator;
        typedef boost::unordered_detail::hash_const_iterator<
            value_allocator, boost::unordered_detail::grouped>
                const_iterator;
        typedef const_local_iterator local_iterator;
        typedef const_iterator iterator;

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0582)
    private:
#endif

        table table_;
        
        BOOST_DEDUCED_TYPENAME types::iterator_base const&
            get(const_iterator const& it)
        {
            return boost::unordered_detail::iterator_access::get(it);
        }

    public:

        // construct/destroy/copy

        explicit unordered_multiset(
                size_type n = boost::unordered_detail::default_bucket_count,
                const hasher &hf = hasher(),
                const key_equal &eql = key_equal(),
                const allocator_type &a = allocator_type())
          : table_(n, hf, eql, a)
        {
        }

        explicit unordered_multiset(allocator_type const& a)
          : table_(boost::unordered_detail::default_bucket_count,
                hasher(), key_equal(), a)
        {
        }

        unordered_multiset(unordered_multiset const& other,
            allocator_type const& a)
          : table_(other.table_, a)
        {
        }

        template <class InputIt>
        unordered_multiset(InputIt f, InputIt l)
          : table_(boost::unordered_detail::initial_size(f, l),
                hasher(), key_equal(), allocator_type())
        {
            table_.insert_range(f, l);
        }

        template <class InputIt>
        unordered_multiset(InputIt f, InputIt l, size_type n,
                const hasher &hf = hasher(),
                const key_equal &eql = key_equal())
          : table_(boost::unordered_detail::initial_size(f, l, n),
                hf, eql, allocator_type())
        {
            table_.insert_range(f, l);
        }

        template <class InputIt>
        unordered_multiset(InputIt f, InputIt l, size_type n,
                const hasher &hf,
                const key_equal &eql,
                const allocator_type &a)
          : table_(boost::unordered_detail::initial_size(f, l, n), hf, eql, a)
        {
            table_.insert_range(f, l);
        }

        ~unordered_multiset() {}

#if !defined(BOOST_NO_RVALUE_REFERENCES)
        unordered_multiset(unordered_multiset&& other)
          : table_(other.table_, boost::unordered_detail::move_tag())
        {
        }

        unordered_multiset(unordered_multiset&& other, allocator_type const& a)
          : table_(other.table_, a, boost::unordered_detail::move_tag())
        {
        }

        unordered_multiset& operator=(unordered_multiset&& x)
        {
            table_.move(x.table_);
            return *this;
        }
#else
        unordered_multiset(boost::unordered_detail::move_from<
                unordered_multiset<T, H, P, A>
            > other)
          : table_(other.source.table_, boost::unordered_detail::move_tag())
        {
        }

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0593)
        unordered_multiset& operator=(unordered_multiset x)
        {
            table_.move(x.table_);
            return *this;
        }
#endif
#endif

#if !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
        unordered_multiset(std::initializer_list<value_type> list,
                size_type n = boost::unordered_detail::default_bucket_count,
                const hasher &hf = hasher(),
                const key_equal &eql = key_equal(),
                const allocator_type &a = allocator_type())
          : table_(boost::unordered_detail::initial_size(
                    list.begin(), list.end(), n),
                hf, eql, a)
        {
            table_.insert_range(list.begin(), list.end());
        }

        unordered_multiset& operator=(std::initializer_list<value_type> list)
        {
            table_.clear();
            table_.insert_range(list.begin(), list.end());
            return *this;
        }
#endif

        allocator_type get_allocator() const
        {
            return table_.node_alloc();
        }

        // size and capacity

        bool empty() const
        {
            return table_.size_ == 0;
        }

        size_type size() const
        {
            return table_.size_;
        }

        size_type max_size() const
        {
            return table_.max_size();
        }

        // iterators

        iterator begin()
        {
            return iterator(table_.begin());
        }

        const_iterator begin() const
        {
            return const_iterator(table_.begin());
        }

        iterator end()
        {
            return iterator(table_.end());
        }

        const_iterator end() const
        {
            return const_iterator(table_.end());
        }

        const_iterator cbegin() const
        {
            return const_iterator(table_.begin());
        }

        const_iterator cend() const
        {
            return const_iterator(table_.end());
        }

        // modifiers

#if defined(BOOST_UNORDERED_STD_FORWARD)
        template <class... Args>
        iterator emplace(Args&&... args)
        {
            return iterator(table_.emplace(std::forward<Args>(args)...));
        }

        template <class... Args>
        iterator emplace_hint(const_iterator, Args&&... args)
        {
            return iterator(table_.emplace(std::forward<Args>(args)...));
        }
#else

        iterator emplace(value_type const& v = value_type())
        {
            return iterator(table_.emplace(v));
        }

        iterator emplace_hint(const_iterator,
            value_type const& v = value_type())
        {
            return iterator(table_.emplace(v));
        }

#define BOOST_UNORDERED_EMPLACE(z, n, _)                                       \
            template <                                                         \
                BOOST_UNORDERED_TEMPLATE_ARGS(z, n)                            \
            >                                                                  \
            iterator emplace(                                                  \
                BOOST_UNORDERED_FUNCTION_PARAMS(z, n)                          \
            )                                                                  \
            {                                                                  \
                return iterator(                                               \
                    table_.emplace(BOOST_UNORDERED_CALL_PARAMS(z, n)));        \
            }                                                                  \
                                                                               \
            template <                                                         \
                BOOST_UNORDERED_TEMPLATE_ARGS(z, n)                            \
            >                                                                  \
            iterator emplace_hint(const_iterator,                              \
                BOOST_UNORDERED_FUNCTION_PARAMS(z, n)                          \
            )                                                                  \
            {                                                                  \
                return iterator(table_.emplace(                                \
                        BOOST_UNORDERED_CALL_PARAMS(z, n)                      \
                ));                                                            \
            }

        BOOST_PP_REPEAT_FROM_TO(1, BOOST_UNORDERED_EMPLACE_LIMIT,
            BOOST_UNORDERED_EMPLACE, _)

#undef BOOST_UNORDERED_EMPLACE

#endif

        iterator insert(const value_type& obj)
        {
            return iterator(table_.emplace(obj));
        }

        iterator insert(const_iterator, const value_type& obj)
        {
            return iterator(table_.emplace(obj));
        }

        template <class InputIt>
            void insert(InputIt first, InputIt last)
        {
            table_.insert_range(first, last);
        }

#if !defined(BOOST_NO_0X_HDR_INITIALIZER_LIST)
        void insert(std::initializer_list<value_type> list)
        {
            table_.insert_range(list.begin(), list.end());
        }
#endif

        iterator erase(const_iterator position)
        {
            return iterator(table_.erase_return_iterator(get(position)));
        }

        size_type erase(const key_type& k)
        {
            return table_.erase_key(k);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            return iterator(table_.erase_range(get(first), get(last)));
        }

        void quick_erase(const_iterator position)
        {
            table_.erase(get(position));
        }

        void erase_return_void(const_iterator position)
        {
            table_.erase(get(position));
        }

        void clear()
        {
            table_.clear();
        }

        void swap(unordered_multiset& other)
        {
            table_.swap(other.table_);
        }

        // observers

        hasher hash_function() const
        {
            return table_.hash_function();
        }

        key_equal key_eq() const
        {
            return table_.key_eq();
        }

        // lookup

        const_iterator find(const key_type& k) const
        {
            return const_iterator(table_.find(k));
        }

        template <class CompatibleKey, class CompatibleHash,
            class CompatiblePredicate>
        const_iterator find(
            CompatibleKey const& k,
            CompatibleHash const& hash,
            CompatiblePredicate const& eq) const
        {
            return iterator(table_.find(k, hash, eq));
        }

        size_type count(const key_type& k) const
        {
            return table_.count(k);
        }

        std::pair<const_iterator, const_iterator>
            equal_range(const key_type& k) const
        {
            return boost::unordered_detail::pair_cast<
                const_iterator, const_iterator>(
                    table_.equal_range(k));
        }

        // bucket interface

        size_type bucket_count() const
        {
            return table_.bucket_count_;
        }

        size_type max_bucket_count() const
        {
            return table_.max_bucket_count();
        }

        size_type bucket_size(size_type n) const
        {
            return table_.bucket_size(n);
        }

        size_type bucket(const key_type& k) const
        {
            return table_.bucket_index(k);
        }

        local_iterator begin(size_type n)
        {
            return local_iterator(table_.bucket_begin(n));
        }

        const_local_iterator begin(size_type n) const
        {
            return const_local_iterator(table_.bucket_begin(n));
        }

        local_iterator end(size_type)
        {
            return local_iterator();
        }

        const_local_iterator end(size_type) const
        {
            return const_local_iterator();
        }

        const_local_iterator cbegin(size_type n) const
        {
            return const_local_iterator(table_.bucket_begin(n));
        }

        const_local_iterator cend(size_type) const
        {
            return const_local_iterator();
        }

        // hash policy

        float load_factor() const
        {
            return table_.load_factor();
        }

        float max_load_factor() const
        {
            return table_.mlf_;
        }

        void max_load_factor(float m)
        {
            table_.max_load_factor(m);
        }

        void rehash(size_type n)
        {
            table_.rehash(n);
        }

#if !BOOST_WORKAROUND(__BORLANDC__, < 0x0582)
        friend bool operator==<T, H, P, A>(
            unordered_multiset const&, unordered_multiset const&);
        friend bool operator!=<T, H, P, A>(
            unordered_multiset const&, unordered_multiset const&);
#endif
    }; // class template unordered_multiset

    template <class T, class H, class P, class A>
    inline bool operator==(unordered_multiset<T, H, P, A> const& m1,
        unordered_multiset<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x0613))
        struct dummy { unordered_multiset<T,H,P,A> x; };
#endif
        return m1.table_.equals(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline bool operator!=(unordered_multiset<T, H, P, A> const& m1,
        unordered_multiset<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x0613))
        struct dummy { unordered_multiset<T,H,P,A> x; };
#endif
        return !m1.table_.equals(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline void swap(unordered_multiset<T, H, P, A> &m1,
            unordered_multiset<T, H, P, A> &m2)
    {
#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x0613))
        struct dummy { unordered_multiset<T,H,P,A> x; };
#endif
        m1.swap(m2);
    }

} // namespace boost

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_UNORDERED_UNORDERED_SET_HPP_INCLUDED
