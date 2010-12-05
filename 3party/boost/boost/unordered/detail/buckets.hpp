
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2009 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_DETAIL_MANAGER_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_MANAGER_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/unordered/detail/node.hpp>
#include <boost/unordered/detail/util.hpp>

namespace boost { namespace unordered_detail {
    
    ////////////////////////////////////////////////////////////////////////////
    // Buckets
    
    template <class A, class G>
    inline std::size_t hash_buckets<A, G>::max_bucket_count() const {
        // -1 to account for the sentinel.
        return prev_prime(this->bucket_alloc().max_size() - 1);
    }

    template <class A, class G>
    inline BOOST_DEDUCED_TYPENAME hash_buckets<A, G>::bucket_ptr
        hash_buckets<A, G>::get_bucket(std::size_t num) const
    {
        return buckets_ + static_cast<std::ptrdiff_t>(num);
    }

    template <class A, class G>
    inline BOOST_DEDUCED_TYPENAME hash_buckets<A, G>::bucket_ptr
        hash_buckets<A, G>::bucket_ptr_from_hash(std::size_t hashed) const
    {
        return get_bucket(hashed % bucket_count_);
    }
    
    template <class A, class G>
    std::size_t hash_buckets<A, G>::bucket_size(std::size_t index) const
    {
        if(!buckets_) return 0;
        bucket_ptr ptr = get_bucket(index)->next_;
        std::size_t count = 0;
        while(ptr) {
            ++count;
            ptr = ptr->next_;
        }
        return count;
    }

    template <class A, class G>
    inline BOOST_DEDUCED_TYPENAME hash_buckets<A, G>::node_ptr
        hash_buckets<A, G>::bucket_begin(std::size_t num) const
    {
        return buckets_ ? get_bucket(num)->next_ : node_ptr();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Delete

    template <class A, class G>
    inline void hash_buckets<A, G>::delete_node(node_ptr b)
    {
        node* raw_ptr = static_cast<node*>(&*b);
        boost::unordered_detail::destroy(&raw_ptr->value());
        real_node_ptr n(node_alloc().address(*raw_ptr));
        node_alloc().destroy(n);
        node_alloc().deallocate(n, 1);
    }

    template <class A, class G>
    inline void hash_buckets<A, G>::clear_bucket(bucket_ptr b)
    {
        node_ptr node_it = b->next_;
        b->next_ = node_ptr();

        while(node_it) {
            node_ptr node_to_delete = node_it;
            node_it = node_it->next_;
            delete_node(node_to_delete);
        }
    }

    template <class A, class G>
    inline void hash_buckets<A, G>::delete_buckets()
    {      
        bucket_ptr end = this->get_bucket(this->bucket_count_);

        for(bucket_ptr begin = this->buckets_; begin != end; ++begin) {
            clear_bucket(begin);
        }

        // Destroy the buckets (including the sentinel bucket).
        ++end;
        for(bucket_ptr begin = this->buckets_; begin != end; ++begin) {
            bucket_alloc().destroy(begin);
        }

        bucket_alloc().deallocate(this->buckets_, this->bucket_count_ + 1);

        this->buckets_ = bucket_ptr();
    }

    template <class A, class G>
    inline std::size_t hash_buckets<A, G>::delete_nodes(
        node_ptr begin, node_ptr end)
    {
        std::size_t count = 0;
        while(begin != end) {
            node_ptr n = begin;
            begin = begin->next_;
            delete_node(n);
            ++count;
        }
        return count;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Constructors and Destructors

    template <class A, class G>
    inline hash_buckets<A, G>::hash_buckets(
        node_allocator const& a, std::size_t bucket_count)
      : buckets_(),
        bucket_count_(bucket_count),
        allocators_(a,a)
    {
    }

    template <class A, class G>
    inline hash_buckets<A, G>::~hash_buckets()
    {
        if(this->buckets_) { this->delete_buckets(); }
    }
    
    template <class A, class G>
    inline void hash_buckets<A, G>::create_buckets()
    {
        // The array constructor will clean up in the event of an
        // exception.
        allocator_array_constructor<bucket_allocator>
            constructor(bucket_alloc());

        // Creates an extra bucket to act as a sentinel.
        constructor.construct(bucket(), this->bucket_count_ + 1);

        // Set up the sentinel (node_ptr cast)
        bucket_ptr sentinel = constructor.get() +
            static_cast<std::ptrdiff_t>(this->bucket_count_);
        sentinel->next_ = sentinel;

        // Only release the buckets once everything is successfully
        // done.
        this->buckets_ = constructor.release();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Constructors and Destructors

    // no throw
    template <class A, class G>
    inline void hash_buckets<A, G>::move(hash_buckets& other)
    {
        BOOST_ASSERT(node_alloc() == other.node_alloc());
        if(this->buckets_) { this->delete_buckets(); }
        this->buckets_ = other.buckets_;
        this->bucket_count_ = other.bucket_count_;
        other.buckets_ = bucket_ptr();
        other.bucket_count_ = 0;
    }

    template <class A, class G>
    inline void hash_buckets<A, G>::swap(hash_buckets<A, G>& other)
    {
        BOOST_ASSERT(node_alloc() == other.node_alloc());
        std::swap(buckets_, other.buckets_);
        std::swap(bucket_count_, other.bucket_count_);
    }
}}

#endif
