
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2009 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_DETAIL_ALL_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_ALL_HPP_INCLUDED

#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/throw_exception.hpp>

#include <boost/unordered/detail/buckets.hpp>

namespace boost { namespace unordered_detail {

    ////////////////////////////////////////////////////////////////////////////
    // Helper methods

    // strong exception safety, no side effects
    template <class T>
    inline bool hash_table<T>::equal(
        key_type const& k, value_type const& v) const
    {
        return this->key_eq()(k, get_key(v));
    }

    // strong exception safety, no side effects
    template <class T>
    template <class Key, class Pred>
    inline BOOST_DEDUCED_TYPENAME T::node_ptr
        hash_table<T>::find_iterator(bucket_ptr bucket, Key const& k,
            Pred const& eq) const
    {
        node_ptr it = bucket->next_;
        while (BOOST_UNORDERED_BORLAND_BOOL(it) &&
            !eq(k, get_key(node::get_value(it))))
        {
            it = node::next_group(it);
        }

        return it;
    }

    // strong exception safety, no side effects
    template <class T>
    inline BOOST_DEDUCED_TYPENAME T::node_ptr
        hash_table<T>::find_iterator(
            bucket_ptr bucket, key_type const& k) const
    {
        node_ptr it = bucket->next_;
        while (BOOST_UNORDERED_BORLAND_BOOL(it) &&
            !equal(k, node::get_value(it)))
        {
            it = node::next_group(it);
        }

        return it;
    }

    // strong exception safety, no side effects
    // pre: this->buckets_
    template <class T>
    inline BOOST_DEDUCED_TYPENAME T::node_ptr
        hash_table<T>::find_iterator(key_type const& k) const
    {
        return find_iterator(this->get_bucket(this->bucket_index(k)), k);
    }

    // strong exception safety, no side effects
    template <class T>
    inline BOOST_DEDUCED_TYPENAME T::node_ptr*
        hash_table<T>::find_for_erase(
            bucket_ptr bucket, key_type const& k) const
    {
        node_ptr* it = &bucket->next_;
        while(BOOST_UNORDERED_BORLAND_BOOL(*it) &&
            !equal(k, node::get_value(*it)))
        {
            it = &node::next_group(*it);
        }

        return it;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Load methods

    // no throw
    template <class T>
    std::size_t hash_table<T>::max_size() const
    {
        using namespace std;

        // size < mlf_ * count
        return double_to_size_t(ceil(
                (double) this->mlf_ * this->max_bucket_count())) - 1;
    }

    // strong safety
    template <class T>
    inline std::size_t hash_table<T>::bucket_index(
        key_type const& k) const
    {
        // hash_function can throw:
        return this->hash_function()(k) % this->bucket_count_;
    }


    // no throw
    template <class T>
    inline std::size_t hash_table<T>::calculate_max_load()
    {
        using namespace std;

        // From 6.3.1/13:
        // Only resize when size >= mlf_ * count
        return double_to_size_t(ceil((double) mlf_ * this->bucket_count_));
    }

    template <class T>
    void hash_table<T>::max_load_factor(float z)
    {
        BOOST_ASSERT(z > 0);
        mlf_ = (std::max)(z, minimum_max_load_factor);
        this->max_load_ = this->calculate_max_load();
    }

    // no throw
    template <class T>
    inline std::size_t hash_table<T>::min_buckets_for_size(
        std::size_t size) const
    {
        BOOST_ASSERT(this->mlf_ != 0);

        using namespace std;

        // From 6.3.1/13:
        // size < mlf_ * count
        // => count > size / mlf_
        //
        // Or from rehash post-condition:
        // count > size / mlf_
        return next_prime(double_to_size_t(floor(size / (double) mlf_)) + 1);
    }

    ////////////////////////////////////////////////////////////////////////////
    // recompute_begin_bucket

    // init_buckets

    template <class T>
    inline void hash_table<T>::init_buckets()
    {
        if (this->size_) {
            this->cached_begin_bucket_ = this->buckets_;
            while (!this->cached_begin_bucket_->next_)
                ++this->cached_begin_bucket_;
        } else {
            this->cached_begin_bucket_ = this->get_bucket(this->bucket_count_);
        }
        this->max_load_ = calculate_max_load();
    }

    // After an erase cached_begin_bucket_ might be left pointing to
    // an empty bucket, so this is called to update it
    //
    // no throw

    template <class T>
    inline void hash_table<T>::recompute_begin_bucket(bucket_ptr b)
    {
        BOOST_ASSERT(!(b < this->cached_begin_bucket_));

        if(b == this->cached_begin_bucket_)
        {
            if (this->size_ != 0) {
                while (!this->cached_begin_bucket_->next_)
                    ++this->cached_begin_bucket_;
            } else {
                this->cached_begin_bucket_ =
                    this->get_bucket(this->bucket_count_);
            }
        }
    }

    // This is called when a range has been erased
    //
    // no throw

    template <class T>
    inline void hash_table<T>::recompute_begin_bucket(
        bucket_ptr b1, bucket_ptr b2)
    {
        BOOST_ASSERT(!(b1 < this->cached_begin_bucket_) && !(b2 < b1));
        BOOST_ASSERT(BOOST_UNORDERED_BORLAND_BOOL(b2->next_));

        if(b1 == this->cached_begin_bucket_ && !b1->next_)
            this->cached_begin_bucket_ = b2;
    }

    // no throw
    template <class T>
    inline float hash_table<T>::load_factor() const
    {
        BOOST_ASSERT(this->bucket_count_ != 0);
        return static_cast<float>(this->size_)
            / static_cast<float>(this->bucket_count_);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Constructors

    template <class T>
    hash_table<T>::hash_table(std::size_t num_buckets,
        hasher const& hf, key_equal const& eq, node_allocator const& a)
      : buckets(a, next_prime(num_buckets)),
        base(hf, eq),
        size_(),
        mlf_(1.0f),
        cached_begin_bucket_(),
        max_load_(0)
    {
    }

    // Copy Construct with allocator

    template <class T>
    hash_table<T>::hash_table(hash_table const& x,
        node_allocator const& a)
      : buckets(a, x.min_buckets_for_size(x.size_)),
        base(x),
        size_(x.size_),
        mlf_(x.mlf_),
        cached_begin_bucket_(),
        max_load_(0)
    {
        if(x.size_) {
            x.copy_buckets_to(*this);
            this->init_buckets();
        }
    }

    // Move Construct

    template <class T>
    hash_table<T>::hash_table(hash_table& x, move_tag)
      : buckets(x.node_alloc(), x.bucket_count_),
        base(x),
        size_(0),
        mlf_(1.0f),
        cached_begin_bucket_(),
        max_load_(0)
    {
        this->partial_swap(x);
    }

    template <class T>
    hash_table<T>::hash_table(hash_table& x,
        node_allocator const& a, move_tag)
      : buckets(a, x.bucket_count_),
        base(x),
        size_(0),
        mlf_(x.mlf_),
        cached_begin_bucket_(),
        max_load_(0)
    {
        if(a == x.node_alloc()) {
            this->partial_swap(x);
        }
        else if(x.size_) {
            x.copy_buckets_to(*this);
            this->size_ = x.size_;
            this->init_buckets();
        }
    }

    template <class T>
    hash_table<T>& hash_table<T>::operator=(
        hash_table const& x)
    {
        hash_table tmp(x, this->node_alloc());
        this->fast_swap(tmp);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Swap & Move
    
    // Swap
    //
    // Strong exception safety
    //
    // Can throw if hash or predicate object's copy constructor throws
    // or if allocators are unequal.

    template <class T>
    inline void hash_table<T>::partial_swap(hash_table& x)
    {
        this->buckets::swap(x); // No throw
        std::swap(this->size_, x.size_);
        std::swap(this->mlf_, x.mlf_);
        std::swap(this->cached_begin_bucket_, x.cached_begin_bucket_);
        std::swap(this->max_load_, x.max_load_);
    }

    template <class T>
    inline void hash_table<T>::fast_swap(hash_table& x)
    {
        // These can throw, but they only affect the function objects
        // that aren't in use so it is strongly exception safe, via.
        // double buffering.
        {
            set_hash_functions<hasher, key_equal> op1(*this, x);
            set_hash_functions<hasher, key_equal> op2(x, *this);
            op1.commit();
            op2.commit();
        }
        this->buckets::swap(x); // No throw
        std::swap(this->size_, x.size_);
        std::swap(this->mlf_, x.mlf_);
        std::swap(this->cached_begin_bucket_, x.cached_begin_bucket_);
        std::swap(this->max_load_, x.max_load_);
    }

    template <class T>
    inline void hash_table<T>::slow_swap(hash_table& x)
    {
        if(this == &x) return;

        {
            // These can throw, but they only affect the function objects
            // that aren't in use so it is strongly exception safe, via.
            // double buffering.
            set_hash_functions<hasher, key_equal> op1(*this, x);
            set_hash_functions<hasher, key_equal> op2(x, *this);
        
            // Create new buckets in separate hash_buckets objects
            // which will clean up if anything throws an exception.
            // (all can throw, but with no effect as these are new objects).
        
            buckets b1(this->node_alloc(), x.min_buckets_for_size(x.size_));
            if(x.size_) x.copy_buckets_to(b1);
        
            buckets b2(x.node_alloc(), this->min_buckets_for_size(this->size_));
            if(this->size_) copy_buckets_to(b2);
        
            // Modifying the data, so no throw from now on.
        
            b1.swap(*this);
            b2.swap(x);
            op1.commit();
            op2.commit();
        }
        
        std::swap(this->size_, x.size_);

        if(this->buckets_) this->init_buckets();
        if(x.buckets_) x.init_buckets();
    }

    template <class T>
    void hash_table<T>::swap(hash_table& x)
    {
        if(this->node_alloc() == x.node_alloc()) {
            if(this != &x) this->fast_swap(x);
        }
        else {
            this->slow_swap(x);
        }
    }

    
    // Move
    //
    // Strong exception safety (might change unused function objects)
    //
    // Can throw if hash or predicate object's copy constructor throws
    // or if allocators are unequal.

    template <class T>
    void hash_table<T>::move(hash_table& x)
    {
        // This can throw, but it only affects the function objects
        // that aren't in use so it is strongly exception safe, via.
        // double buffering.
        set_hash_functions<hasher, key_equal> new_func_this(*this, x);

        if(this->node_alloc() == x.node_alloc()) {
            this->buckets::move(x); // no throw
            this->size_ = x.size_;
            this->cached_begin_bucket_ = x.cached_begin_bucket_;
            this->max_load_ = x.max_load_;
            x.size_ = 0;
        }
        else {
            // Create new buckets in separate HASH_TABLE_DATA objects
            // which will clean up if anything throws an exception.
            // (all can throw, but with no effect as these are new objects).
            
            buckets b(this->node_alloc(), x.min_buckets_for_size(x.size_));
            if(x.size_) x.copy_buckets_to(b);

            // Start updating the data here, no throw from now on.
            this->size_ = x.size_;
            b.swap(*this);
            this->init_buckets();
        }

        // We've made it, the rest is no throw.
        this->mlf_ = x.mlf_;
        new_func_this.commit();
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Reserve & Rehash

    // basic exception safety
    template <class T>
    inline void hash_table<T>::create_for_insert(std::size_t size)
    {
        this->bucket_count_ = (std::max)(this->bucket_count_,
            this->min_buckets_for_size(size));
        this->create_buckets();
        this->init_buckets();
    }

    // basic exception safety
    template <class T>
    inline bool hash_table<T>::reserve_for_insert(std::size_t size)
    {
        if(size >= max_load_) {
            std::size_t num_buckets
                = this->min_buckets_for_size((std::max)(size,
                    this->size_ + (this->size_ >> 1)));
            if(num_buckets != this->bucket_count_) {
                rehash_impl(num_buckets);
                return true;
            }
        }
        
        return false;
    }

    // if hash function throws, basic exception safety
    // strong otherwise.

    template <class T>
    inline void hash_table<T>::rehash(std::size_t min_buckets)
    {
        using namespace std;

        if(!this->size_) {
            if(this->buckets_) this->delete_buckets();
            this->bucket_count_ = next_prime(min_buckets);
        }
        else {
            // no throw:
            min_buckets = next_prime((std::max)(min_buckets,
                    double_to_size_t(floor(this->size_ / (double) mlf_)) + 1));
            if(min_buckets != this->bucket_count_) rehash_impl(min_buckets);
        }
    }

    // if hash function throws, basic exception safety
    // strong otherwise

    template <class T>
    void hash_table<T>
        ::rehash_impl(std::size_t num_buckets)
    {    
        hasher const& hf = this->hash_function();
        std::size_t size = this->size_;
        bucket_ptr end = this->get_bucket(this->bucket_count_);

        buckets dst(this->node_alloc(), num_buckets);
        dst.create_buckets();

        buckets src(this->node_alloc(), this->bucket_count_);
        src.swap(*this);
        this->size_ = 0;

        for(bucket_ptr bucket = this->cached_begin_bucket_;
            bucket != end; ++bucket)
        {
            node_ptr group = bucket->next_;
            while(group) {
                // Move the first group of equivalent nodes in bucket to dst.

                // This next line throws iff the hash function throws.
                bucket_ptr dst_bucket = dst.bucket_ptr_from_hash(
                    hf(get_key_from_ptr(group)));

                node_ptr& next_group = node::next_group(group);
                bucket->next_ = next_group;
                next_group = dst_bucket->next_;
                dst_bucket->next_ = group;
                group = bucket->next_;
            }
        }

        // Swap the new nodes back into the container and setup the local
        // variables.
        this->size_ = size;
        dst.swap(*this);                        // no throw
        this->init_buckets();
    }

    ////////////////////////////////////////////////////////////////////////////
    // copy_buckets_to

    // copy_buckets_to
    //
    // basic excpetion safety. If an exception is thrown this will
    // leave dst partially filled.

    template <class T>
    void hash_table<T>
        ::copy_buckets_to(buckets& dst) const
    {
        BOOST_ASSERT(this->buckets_ && !dst.buckets_);

        hasher const& hf = this->hash_function();
        bucket_ptr end = this->get_bucket(this->bucket_count_);

        node_constructor a(dst);
        dst.create_buckets();

        // no throw:
        for(bucket_ptr i = this->cached_begin_bucket_; i != end; ++i) {
            // no throw:
            for(node_ptr it = i->next_; it;) {
                // hash function can throw.
                bucket_ptr dst_bucket = dst.bucket_ptr_from_hash(
                    hf(get_key_from_ptr(it)));
                // throws, strong

                node_ptr group_end = node::next_group(it);

                a.construct(node::get_value(it));
                node_ptr n = a.release();
                node::add_to_bucket(n, *dst_bucket);
        
                for(it = it->next_; it != group_end; it = it->next_) {
                    a.construct(node::get_value(it));
                    node::add_after_node(a.release(), n);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Misc. key methods

    // strong exception safety

    // count
    //
    // strong exception safety, no side effects

    template <class T>
    std::size_t hash_table<T>::count(key_type const& k) const
    {
        if(!this->size_) return 0;
        node_ptr it = find_iterator(k); // throws, strong
        return BOOST_UNORDERED_BORLAND_BOOL(it) ? node::group_count(it) : 0;
    }

    // find
    //
    // strong exception safety, no side effects
    template <class T>
    BOOST_DEDUCED_TYPENAME T::iterator_base
        hash_table<T>::find(key_type const& k) const
    {
        if(!this->size_) return this->end();

        bucket_ptr bucket = this->get_bucket(this->bucket_index(k));
        node_ptr it = find_iterator(bucket, k);

        if (BOOST_UNORDERED_BORLAND_BOOL(it))
            return iterator_base(bucket, it);
        else
            return this->end();
    }

    template <class T>
    template <class Key, class Hash, class Pred>
    BOOST_DEDUCED_TYPENAME T::iterator_base hash_table<T>::find(Key const& k,
        Hash const& h, Pred const& eq) const
    {
        if(!this->size_) return this->end();

        bucket_ptr bucket = this->get_bucket(h(k) % this->bucket_count_);
        node_ptr it = find_iterator(bucket, k, eq);

        if (BOOST_UNORDERED_BORLAND_BOOL(it))
            return iterator_base(bucket, it);
        else
            return this->end();
    }

    template <class T>
    BOOST_DEDUCED_TYPENAME T::value_type&
        hash_table<T>::at(key_type const& k) const
    {
        if(!this->size_)
            boost::throw_exception(std::out_of_range("Unable to find key in unordered_map."));

        bucket_ptr bucket = this->get_bucket(this->bucket_index(k));
        node_ptr it = find_iterator(bucket, k);

        if (!it)
            boost::throw_exception(std::out_of_range("Unable to find key in unordered_map."));

        return node::get_value(it);
    }

    // equal_range
    //
    // strong exception safety, no side effects
    template <class T>
    BOOST_DEDUCED_TYPENAME T::iterator_pair
        hash_table<T>::equal_range(key_type const& k) const
    {
        if(!this->size_)
            return iterator_pair(this->end(), this->end());

        bucket_ptr bucket = this->get_bucket(this->bucket_index(k));
        node_ptr it = find_iterator(bucket, k);
        if (BOOST_UNORDERED_BORLAND_BOOL(it)) {
            iterator_base first(iterator_base(bucket, it));
            iterator_base second(first);
            second.increment_bucket(node::next_group(second.node_));
            return iterator_pair(first, second);
        }
        else {
            return iterator_pair(this->end(), this->end());
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Erase methods    
    
    template <class T>
    void hash_table<T>::clear()
    {
        if(!this->size_) return;

        bucket_ptr end = this->get_bucket(this->bucket_count_);
        for(bucket_ptr begin = this->buckets_; begin != end; ++begin) {
            this->clear_bucket(begin);
        }

        this->size_ = 0;
        this->cached_begin_bucket_ = end;
    }

    template <class T>
    inline std::size_t hash_table<T>::erase_group(
        node_ptr* it, bucket_ptr bucket)
    {
        node_ptr pos = *it;
        node_ptr end = node::next_group(pos);
        *it = end;
        std::size_t count = this->delete_nodes(pos, end);
        this->size_ -= count;
        this->recompute_begin_bucket(bucket);
        return count;
    }
    
    template <class T>
    std::size_t hash_table<T>::erase_key(key_type const& k)
    {
        if(!this->size_) return 0;
    
        // No side effects in initial section
        bucket_ptr bucket = this->get_bucket(this->bucket_index(k));
        node_ptr* it = this->find_for_erase(bucket, k);

        // No throw.
        return *it ? this->erase_group(it, bucket) : 0;
    }

    template <class T>
    void hash_table<T>::erase(iterator_base r)
    {
        BOOST_ASSERT(r.node_);
        --this->size_;
        node::unlink_node(*r.bucket_, r.node_);
        this->delete_node(r.node_);
        // r has been invalidated but its bucket is still valid
        this->recompute_begin_bucket(r.bucket_);
    }

    template <class T>
    BOOST_DEDUCED_TYPENAME T::iterator_base
        hash_table<T>::erase_return_iterator(iterator_base r)
    {
        BOOST_ASSERT(r.node_);
        iterator_base next = r;
        next.increment();
        --this->size_;
        node::unlink_node(*r.bucket_, r.node_);
        this->delete_node(r.node_);
        // r has been invalidated but its bucket is still valid
        this->recompute_begin_bucket(r.bucket_, next.bucket_);
        return next;
    }

    template <class T>
    BOOST_DEDUCED_TYPENAME T::iterator_base
        hash_table<T>::erase_range(
            iterator_base r1, iterator_base r2)
    {
        if(r1 != r2)
        {
            BOOST_ASSERT(r1.node_);
            if (r1.bucket_ == r2.bucket_) {
                node::unlink_nodes(*r1.bucket_, r1.node_, r2.node_);
                this->size_ -= this->delete_nodes(r1.node_, r2.node_);

                // No need to call recompute_begin_bucket because
                // the nodes are only deleted from one bucket, which
                // still contains r2 after the erase.
                 BOOST_ASSERT(r1.bucket_->next_);
            }
            else {
                bucket_ptr end_bucket = r2.node_ ?
                    r2.bucket_ : this->get_bucket(this->bucket_count_);
                BOOST_ASSERT(r1.bucket_ < end_bucket);
                node::unlink_nodes(*r1.bucket_, r1.node_, node_ptr());
                this->size_ -= this->delete_nodes(r1.node_, node_ptr());

                bucket_ptr i = r1.bucket_;
                for(++i; i != end_bucket; ++i) {
                    this->size_ -= this->delete_nodes(i->next_, node_ptr());
                    i->next_ = node_ptr();
                }

                if(r2.node_) {
                    node_ptr first = r2.bucket_->next_;
                    node::unlink_nodes(*r2.bucket_, r2.node_);
                    this->size_ -= this->delete_nodes(first, r2.node_);
                }

                // r1 has been invalidated but its bucket is still
                // valid.
                this->recompute_begin_bucket(r1.bucket_, end_bucket);
            }
        }

        return r2;
    }

    template <class T>
    BOOST_DEDUCED_TYPENAME hash_table<T>::iterator_base
        hash_table<T>::emplace_empty_impl_with_node(
            node_constructor& a, std::size_t size)
    {
        key_type const& k = get_key(a.value());
        std::size_t hash_value = this->hash_function()(k);
        if(this->buckets_) this->reserve_for_insert(size);
        else this->create_for_insert(size);
        bucket_ptr bucket = this->bucket_ptr_from_hash(hash_value);
        node_ptr n = a.release();
        node::add_to_bucket(n, *bucket);
        ++this->size_;
        this->cached_begin_bucket_ = bucket;
        return iterator_base(bucket, n);
    }
}}

#endif
