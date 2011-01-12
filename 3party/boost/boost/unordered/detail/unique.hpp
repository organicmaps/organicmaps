
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2010 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_DETAIL_UNIQUE_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_UNIQUE_HPP_INCLUDED

#include <boost/unordered/detail/table.hpp>
#include <boost/unordered/detail/extract_key.hpp>

namespace boost { namespace unordered_detail {

    template <class T>
    class hash_unique_table : public T::table
    {
    public:
        typedef BOOST_DEDUCED_TYPENAME T::hasher hasher;
        typedef BOOST_DEDUCED_TYPENAME T::key_equal key_equal;
        typedef BOOST_DEDUCED_TYPENAME T::value_allocator value_allocator;
        typedef BOOST_DEDUCED_TYPENAME T::key_type key_type;
        typedef BOOST_DEDUCED_TYPENAME T::value_type value_type;
        typedef BOOST_DEDUCED_TYPENAME T::table table;
        typedef BOOST_DEDUCED_TYPENAME T::node_constructor node_constructor;

        typedef BOOST_DEDUCED_TYPENAME T::node node;
        typedef BOOST_DEDUCED_TYPENAME T::node_ptr node_ptr;
        typedef BOOST_DEDUCED_TYPENAME T::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME T::iterator_base iterator_base;
        typedef BOOST_DEDUCED_TYPENAME T::extractor extractor;
        
        typedef std::pair<iterator_base, bool> emplace_return;

        // Constructors

        hash_unique_table(std::size_t n, hasher const& hf, key_equal const& eq,
            value_allocator const& a)
          : table(n, hf, eq, a) {}
        hash_unique_table(hash_unique_table const& x)
          : table(x, x.node_alloc()) {}
        hash_unique_table(hash_unique_table const& x, value_allocator const& a)
          : table(x, a) {}
        hash_unique_table(hash_unique_table& x, move_tag m)
          : table(x, m) {}
        hash_unique_table(hash_unique_table& x, value_allocator const& a,
            move_tag m)
          : table(x, a, m) {}
        ~hash_unique_table() {}

        // Insert methods

        emplace_return emplace_impl_with_node(node_constructor& a);
        value_type& operator[](key_type const& k);

        // equals

        bool equals(hash_unique_table const&) const;

        node_ptr add_node(node_constructor& a, bucket_ptr bucket);
        
#if defined(BOOST_UNORDERED_STD_FORWARD)

        template<class... Args>
        emplace_return emplace(Args&&... args);
        template<class... Args>
        emplace_return emplace_impl(key_type const& k, Args&&... args);
        template<class... Args>
        emplace_return emplace_impl(no_key, Args&&... args);
        template<class... Args>
        emplace_return emplace_empty_impl(Args&&... args);
#else

#define BOOST_UNORDERED_INSERT_IMPL(z, n, _)                                   \
        template <BOOST_UNORDERED_TEMPLATE_ARGS(z, n)>                         \
        emplace_return emplace(                                                \
            BOOST_UNORDERED_FUNCTION_PARAMS(z, n));                            \
        template <BOOST_UNORDERED_TEMPLATE_ARGS(z, n)>                         \
        emplace_return emplace_impl(key_type const& k,                         \
           BOOST_UNORDERED_FUNCTION_PARAMS(z, n));                             \
        template <BOOST_UNORDERED_TEMPLATE_ARGS(z, n)>                         \
        emplace_return emplace_impl(no_key,                                    \
           BOOST_UNORDERED_FUNCTION_PARAMS(z, n));                             \
        template <BOOST_UNORDERED_TEMPLATE_ARGS(z, n)>                         \
        emplace_return emplace_empty_impl(                                     \
           BOOST_UNORDERED_FUNCTION_PARAMS(z, n));

        BOOST_PP_REPEAT_FROM_TO(1, BOOST_UNORDERED_EMPLACE_LIMIT,
            BOOST_UNORDERED_INSERT_IMPL, _)

#undef BOOST_UNORDERED_INSERT_IMPL

#endif

        // if hash function throws, or inserting > 1 element, basic exception
        // safety strong otherwise
        template <class InputIt>
        void insert_range(InputIt i, InputIt j);
        template <class InputIt>
        void insert_range_impl(key_type const&, InputIt i, InputIt j);
        template <class InputIt>
        void insert_range_impl2(node_constructor&, key_type const&, InputIt i, InputIt j);
        template <class InputIt>
        void insert_range_impl(no_key, InputIt i, InputIt j);
    };

    template <class H, class P, class A>
    struct set : public types<
        BOOST_DEDUCED_TYPENAME A::value_type,
        BOOST_DEDUCED_TYPENAME A::value_type,
        H, P, A,
        set_extractor<BOOST_DEDUCED_TYPENAME A::value_type>,
        ungrouped>
    {        
        typedef hash_unique_table<set<H, P, A> > impl;
        typedef hash_table<set<H, P, A> > table;
    };

    template <class K, class H, class P, class A>
    struct map : public types<
        K, BOOST_DEDUCED_TYPENAME A::value_type,
        H, P, A,
        map_extractor<K, BOOST_DEDUCED_TYPENAME A::value_type>,
        ungrouped>
    {
        typedef hash_unique_table<map<K, H, P, A> > impl;
        typedef hash_table<map<K, H, P, A> > table;
    };

    ////////////////////////////////////////////////////////////////////////////
    // Equality

    template <class T>
    bool hash_unique_table<T>
        ::equals(hash_unique_table<T> const& other) const
    {
        if(this->size_ != other.size_) return false;
        if(!this->size_) return true;

        bucket_ptr end = this->get_bucket(this->bucket_count_);
        for(bucket_ptr i = this->cached_begin_bucket_; i != end; ++i)
        {
            node_ptr it1 = i->next_;
            while(BOOST_UNORDERED_BORLAND_BOOL(it1))
            {
                node_ptr it2 = other.find_iterator(this->get_key_from_ptr(it1));
                if(!BOOST_UNORDERED_BORLAND_BOOL(it2)) return false;
                if(!extractor::compare_mapped(
                    node::get_value(it1), node::get_value(it2)))
                    return false;
                it1 = it1->next_;
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // A convenience method for adding nodes.

    template <class T>
    inline BOOST_DEDUCED_TYPENAME hash_unique_table<T>::node_ptr
        hash_unique_table<T>::add_node(node_constructor& a,
            bucket_ptr bucket)
    {
        node_ptr n = a.release();
        node::add_to_bucket(n, *bucket);
        ++this->size_;
        if(bucket < this->cached_begin_bucket_)
            this->cached_begin_bucket_ = bucket;
        return n;
    }
        
    ////////////////////////////////////////////////////////////////////////////
    // Insert methods

    // if hash function throws, basic exception safety
    // strong otherwise
    template <class T>
    BOOST_DEDUCED_TYPENAME hash_unique_table<T>::value_type&
        hash_unique_table<T>::operator[](key_type const& k)
    {
        typedef BOOST_DEDUCED_TYPENAME value_type::second_type mapped_type;

        std::size_t hash_value = this->hash_function()(k);
        bucket_ptr bucket = this->bucket_ptr_from_hash(hash_value);
        
        if(!this->buckets_) {
            node_constructor a(*this);
            a.construct_pair(k, (mapped_type*) 0);
            return *this->emplace_empty_impl_with_node(a, 1);
        }

        node_ptr pos = this->find_iterator(bucket, k);

        if (BOOST_UNORDERED_BORLAND_BOOL(pos)) {
            return node::get_value(pos);
        }
        else {
            // Side effects only in this block.

            // Create the node before rehashing in case it throws an
            // exception (need strong safety in such a case).
            node_constructor a(*this);
            a.construct_pair(k, (mapped_type*) 0);

            // reserve has basic exception safety if the hash function
            // throws, strong otherwise.
            if(this->reserve_for_insert(this->size_ + 1))
                bucket = this->bucket_ptr_from_hash(hash_value);

            // Nothing after this point can throw.

            return node::get_value(add_node(a, bucket));
        }
    }

    template <class T>
    inline BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return
    hash_unique_table<T>::emplace_impl_with_node(node_constructor& a)
    {
        // No side effects in this initial code
        key_type const& k = this->get_key(a.value());
        std::size_t hash_value = this->hash_function()(k);
        bucket_ptr bucket = this->bucket_ptr_from_hash(hash_value);
        node_ptr pos = this->find_iterator(bucket, k);
        
        if (BOOST_UNORDERED_BORLAND_BOOL(pos)) {
            // Found an existing key, return it (no throw).
            return emplace_return(iterator_base(bucket, pos), false);
        } else {
            // reserve has basic exception safety if the hash function
            // throws, strong otherwise.
            if(this->reserve_for_insert(this->size_ + 1))
                bucket = this->bucket_ptr_from_hash(hash_value);

            // Nothing after this point can throw.

            return emplace_return(
                iterator_base(bucket, add_node(a, bucket)),
                true);
        }
    }

#if defined(BOOST_UNORDERED_STD_FORWARD)

    template <class T>
    template<class... Args>
    inline BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return
        hash_unique_table<T>::emplace_impl(key_type const& k,
            Args&&... args)
    {
        // No side effects in this initial code
        std::size_t hash_value = this->hash_function()(k);
        bucket_ptr bucket = this->bucket_ptr_from_hash(hash_value);
        node_ptr pos = this->find_iterator(bucket, k);

        if (BOOST_UNORDERED_BORLAND_BOOL(pos)) {
            // Found an existing key, return it (no throw).
            return emplace_return(iterator_base(bucket, pos), false);

        } else {
            // Doesn't already exist, add to bucket.
            // Side effects only in this block.

            // Create the node before rehashing in case it throws an
            // exception (need strong safety in such a case).
            node_constructor a(*this);
            a.construct(std::forward<Args>(args)...);

            // reserve has basic exception safety if the hash function
            // throws, strong otherwise.
            if(this->reserve_for_insert(this->size_ + 1))
                bucket = this->bucket_ptr_from_hash(hash_value);

            // Nothing after this point can throw.

            return emplace_return(
                iterator_base(bucket, add_node(a, bucket)),
                true);
        }
    }

    template <class T>
    template<class... Args>
    inline BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return
        hash_unique_table<T>::emplace_impl(no_key, Args&&... args)
    {
        // Construct the node regardless - in order to get the key.
        // It will be discarded if it isn't used
        node_constructor a(*this);
        a.construct(std::forward<Args>(args)...);
        return emplace_impl_with_node(a);
    }

    template <class T>
    template<class... Args>
    inline BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return
        hash_unique_table<T>::emplace_empty_impl(Args&&... args)
    {
        node_constructor a(*this);
        a.construct(std::forward<Args>(args)...);
        return emplace_return(this->emplace_empty_impl_with_node(a, 1), true);
    }

#else

#define BOOST_UNORDERED_INSERT_IMPL(z, num_params, _)                          \
    template <class T>                                                         \
    template <BOOST_UNORDERED_TEMPLATE_ARGS(z, num_params)>                    \
    inline BOOST_DEDUCED_TYPENAME                                              \
        hash_unique_table<T>::emplace_return                                   \
            hash_unique_table<T>::emplace_impl(                                \
                key_type const& k,                                             \
                BOOST_UNORDERED_FUNCTION_PARAMS(z, num_params))                \
    {                                                                          \
        std::size_t hash_value = this->hash_function()(k);                     \
        bucket_ptr bucket                                                      \
            = this->bucket_ptr_from_hash(hash_value);                          \
        node_ptr pos = this->find_iterator(bucket, k);                         \
                                                                               \
        if (BOOST_UNORDERED_BORLAND_BOOL(pos)) {                               \
            return emplace_return(iterator_base(bucket, pos), false);          \
        } else {                                                               \
            node_constructor a(*this);                                         \
            a.construct(BOOST_UNORDERED_CALL_PARAMS(z, num_params));           \
                                                                               \
            if(this->reserve_for_insert(this->size_ + 1))                      \
                bucket = this->bucket_ptr_from_hash(hash_value);               \
                                                                               \
            return emplace_return(iterator_base(bucket,                        \
                add_node(a, bucket)), true);                                   \
        }                                                                      \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    template <BOOST_UNORDERED_TEMPLATE_ARGS(z, num_params)>                    \
    inline BOOST_DEDUCED_TYPENAME                                              \
        hash_unique_table<T>::emplace_return                                   \
            hash_unique_table<T>::                                             \
                emplace_impl(no_key,                                           \
                    BOOST_UNORDERED_FUNCTION_PARAMS(z, num_params))            \
    {                                                                          \
        node_constructor a(*this);                                             \
        a.construct(BOOST_UNORDERED_CALL_PARAMS(z, num_params));               \
        return emplace_impl_with_node(a);                                      \
    }                                                                          \
                                                                               \
    template <class T>                                                         \
    template <BOOST_UNORDERED_TEMPLATE_ARGS(z, num_params)>                    \
    inline BOOST_DEDUCED_TYPENAME                                              \
        hash_unique_table<T>::emplace_return                                   \
            hash_unique_table<T>::                                             \
                emplace_empty_impl(                                            \
                    BOOST_UNORDERED_FUNCTION_PARAMS(z, num_params))            \
    {                                                                          \
        node_constructor a(*this);                                             \
        a.construct(BOOST_UNORDERED_CALL_PARAMS(z, num_params));               \
        return emplace_return(this->emplace_empty_impl_with_node(a, 1), true); \
    }

    BOOST_PP_REPEAT_FROM_TO(1, BOOST_UNORDERED_EMPLACE_LIMIT,
        BOOST_UNORDERED_INSERT_IMPL, _)

#undef BOOST_UNORDERED_INSERT_IMPL

#endif

#if defined(BOOST_UNORDERED_STD_FORWARD)

    // Emplace (unique keys)
    // (I'm using an overloaded emplace for both 'insert' and 'emplace')

    // if hash function throws, basic exception safety
    // strong otherwise

    template <class T>
    template<class... Args>
    BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return
        hash_unique_table<T>::emplace(Args&&... args)
    {
        return this->size_ ?
            emplace_impl(
                extractor::extract(std::forward<Args>(args)...),
                std::forward<Args>(args)...) :
            emplace_empty_impl(std::forward<Args>(args)...);
    }

#else

    template <class T>
    template <class Arg0>
    BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return
        hash_unique_table<T>::emplace(Arg0 const& arg0)
    {
        return this->size_ ?
            emplace_impl(extractor::extract(arg0), arg0) :
            emplace_empty_impl(arg0);
    }

#define BOOST_UNORDERED_INSERT_IMPL(z, num_params, _)                          \
    template <class T>                                                         \
    template <BOOST_UNORDERED_TEMPLATE_ARGS(z, num_params)>                    \
    BOOST_DEDUCED_TYPENAME hash_unique_table<T>::emplace_return                \
        hash_unique_table<T>::emplace(                                         \
            BOOST_UNORDERED_FUNCTION_PARAMS(z, num_params))                    \
    {                                                                          \
        return this->size_ ?                                                   \
            emplace_impl(extractor::extract(arg0, arg1),                       \
                BOOST_UNORDERED_CALL_PARAMS(z, num_params)) :                  \
            emplace_empty_impl(                                                \
                BOOST_UNORDERED_CALL_PARAMS(z, num_params));                   \
    }

    BOOST_PP_REPEAT_FROM_TO(2, BOOST_UNORDERED_EMPLACE_LIMIT,
        BOOST_UNORDERED_INSERT_IMPL, _)

#undef BOOST_UNORDERED_INSERT_IMPL

#endif
    
    ////////////////////////////////////////////////////////////////////////////
    // Insert range methods

    template <class T>
    template <class InputIt>
    inline void hash_unique_table<T>::insert_range_impl2(
        node_constructor& a, key_type const& k, InputIt i, InputIt j)
    {
        // No side effects in this initial code
        std::size_t hash_value = this->hash_function()(k);
        bucket_ptr bucket = this->bucket_ptr_from_hash(hash_value);
        node_ptr pos = this->find_iterator(bucket, k);

        if (!BOOST_UNORDERED_BORLAND_BOOL(pos)) {
            // Doesn't already exist, add to bucket.
            // Side effects only in this block.

            // Create the node before rehashing in case it throws an
            // exception (need strong safety in such a case).
            a.construct(*i);

            // reserve has basic exception safety if the hash function
            // throws, strong otherwise.
            if(this->size_ + 1 >= this->max_load_) {
                this->reserve_for_insert(this->size_ + insert_size(i, j));
                bucket = this->bucket_ptr_from_hash(hash_value);
            }

            // Nothing after this point can throw.
            add_node(a, bucket);
        }
    }

    template <class T>
    template <class InputIt>
    inline void hash_unique_table<T>::insert_range_impl(
        key_type const&, InputIt i, InputIt j)
    {
        node_constructor a(*this);

        if(!this->size_) {
            a.construct(*i);
            this->emplace_empty_impl_with_node(a, 1);
            ++i;
            if(i == j) return;
        }

        do {
            // Note: can't use get_key as '*i' might not be value_type - it
            // could be a pair with first_types as key_type without const or a
            // different second_type.
            //
            // TODO: Might be worth storing the value_type instead of the key
            // here. Could be more efficient if '*i' is expensive. Could be
            // less efficient if copying the full value_type is expensive.
            insert_range_impl2(a, extractor::extract(*i), i, j);
        } while(++i != j);
    }

    template <class T>
    template <class InputIt>
    inline void hash_unique_table<T>::insert_range_impl(
        no_key, InputIt i, InputIt j)
    {
        node_constructor a(*this);

        if(!this->size_) {
            a.construct(*i);
            this->emplace_empty_impl_with_node(a, 1);
            ++i;
            if(i == j) return;
        }

        do {
            // No side effects in this initial code
            a.construct(*i);
            emplace_impl_with_node(a);
        } while(++i != j);
    }

    // if hash function throws, or inserting > 1 element, basic exception safety
    // strong otherwise
    template <class T>
    template <class InputIt>
    void hash_unique_table<T>::insert_range(InputIt i, InputIt j)
    {
        if(i != j)
            return insert_range_impl(extractor::extract(*i), i, j);
    }
}}

#endif
