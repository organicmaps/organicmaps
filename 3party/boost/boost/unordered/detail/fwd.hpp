
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2009 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This contains the basic data structure, apart from the actual values. There's
// no construction or deconstruction here. So this only depends on the pointer
// type.

#ifndef BOOST_UNORDERED_DETAIL_FWD_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_FWD_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/iterator.hpp>
#include <boost/compressed_pair.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/unordered/detail/allocator_helpers.hpp>
#include <algorithm>

// This header defines most of the classes used to implement the unordered
// containers. It doesn't include the insert methods as they require a lot
// of preprocessor metaprogramming - they are in insert.hpp

// Template parameters:
//
// H = Hash Function
// P = Predicate
// A = Value Allocator
// G = Grouped/Ungrouped
// E = Key Extractor

#if !defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_NO_VARIADIC_TEMPLATES)
#   if defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)
        // STLport doesn't have std::forward.
#   else
#       define BOOST_UNORDERED_STD_FORWARD
#   endif
#endif

#if !defined(BOOST_UNORDERED_EMPLACE_LIMIT)
#define BOOST_UNORDERED_EMPLACE_LIMIT 10
#endif

#if !defined(BOOST_UNORDERED_STD_FORWARD)

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#define BOOST_UNORDERED_TEMPLATE_ARGS(z, num_params) \
    BOOST_PP_ENUM_PARAMS_Z(z, num_params, class Arg)
#define BOOST_UNORDERED_FUNCTION_PARAMS(z, num_params) \
    BOOST_PP_ENUM_BINARY_PARAMS_Z(z, num_params, Arg, const& arg)
#define BOOST_UNORDERED_CALL_PARAMS(z, num_params) \
    BOOST_PP_ENUM_PARAMS_Z(z, num_params, arg)

#endif

namespace boost { namespace unordered_detail {

    static const float minimum_max_load_factor = 1e-3f;
    static const std::size_t default_bucket_count = 11;
    struct move_tag {};

    template <class T> class hash_unique_table;
    template <class T> class hash_equivalent_table;
    template <class Alloc, class Grouped>
    class hash_node_constructor;
    template <class ValueType>
    struct set_extractor;
    template <class Key, class ValueType>
    struct map_extractor;
    struct no_key;

    // Explicitly call a destructor

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter
#endif

    template <class T>
    inline void destroy(T* x) {
        x->~T();
    }

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

    // hash_bucket
    
    template <class A>
    class hash_bucket
    {
        hash_bucket& operator=(hash_bucket const&);
    public:
        typedef hash_bucket<A> bucket;
        typedef BOOST_DEDUCED_TYPENAME
            boost::unordered_detail::rebind_wrap<A, bucket>::type
            bucket_allocator;
        typedef BOOST_DEDUCED_TYPENAME bucket_allocator::pointer bucket_ptr;
        typedef bucket_ptr node_ptr;
    
        node_ptr next_;

        hash_bucket() : next_() {}
    };

    template <class A>
    struct ungrouped_node_base : hash_bucket<A> {
        typedef hash_bucket<A> bucket;
        typedef BOOST_DEDUCED_TYPENAME bucket::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME bucket::node_ptr node_ptr;

        ungrouped_node_base() : bucket() {}
        static inline node_ptr& next_group(node_ptr ptr);
        static inline std::size_t group_count(node_ptr ptr);
        static inline void add_to_bucket(node_ptr n, bucket& b);
        static inline void add_after_node(node_ptr n, node_ptr position);
        static void unlink_node(bucket& b, node_ptr n);
        static void unlink_nodes(bucket& b, node_ptr begin, node_ptr end);
        static void unlink_nodes(bucket& b, node_ptr end);
    };

    template <class A>
    struct grouped_node_base : hash_bucket<A>
    {
        typedef hash_bucket<A> bucket;
        typedef BOOST_DEDUCED_TYPENAME bucket::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME bucket::node_ptr node_ptr;

        node_ptr group_prev_;

        grouped_node_base() : bucket(), group_prev_() {}
        static inline node_ptr& next_group(node_ptr ptr);
        static inline node_ptr first_in_group(node_ptr n);
        static inline std::size_t group_count(node_ptr ptr);
        static inline void add_to_bucket(node_ptr n, bucket& b);
        static inline void add_after_node(node_ptr n, node_ptr position);
        static void unlink_node(bucket& b, node_ptr n);
        static void unlink_nodes(bucket& b, node_ptr begin, node_ptr end);
        static void unlink_nodes(bucket& b, node_ptr end);

    private:
        static inline node_ptr split_group(node_ptr split);
        static inline grouped_node_base& get(node_ptr ptr) {
            return static_cast<grouped_node_base&>(*ptr);
        }
    };

    struct ungrouped
    {
        template <class A>
        struct base {
            typedef ungrouped_node_base<A> type;
        };
    };

    struct grouped
    {
        template <class A>
        struct base {
            typedef grouped_node_base<A> type;
        };
    };

    template <class ValueType>
    struct value_base
    {
        typedef ValueType value_type;
        BOOST_DEDUCED_TYPENAME boost::aligned_storage<
            sizeof(value_type),
            ::boost::alignment_of<value_type>::value>::type data_;

        void* address() {
            return this;
        }
        value_type& value() {
            return *(ValueType*) this;
        }
    private:
        value_base& operator=(value_base const&);
    };

    // Node
    
    template <class A, class G>
    class hash_node :
        public G::BOOST_NESTED_TEMPLATE base<A>::type,
        public value_base<BOOST_DEDUCED_TYPENAME A::value_type>
    {
    public:
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;
        typedef BOOST_DEDUCED_TYPENAME hash_bucket<A>::node_ptr node_ptr;

        static value_type& get_value(node_ptr p) {
            return static_cast<hash_node&>(*p).value();
        }
    private:
        hash_node& operator=(hash_node const&);
    };

    // Iterator Base

    template <class A, class G>
    class hash_iterator_base
    {
    public:
        typedef A value_allocator;
        typedef hash_bucket<A> bucket;
        typedef hash_node<A, G> node;
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;
        typedef BOOST_DEDUCED_TYPENAME bucket::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME bucket::node_ptr node_ptr;

        bucket_ptr bucket_;
        node_ptr node_;

        hash_iterator_base() : bucket_(), node_() {}
        explicit hash_iterator_base(bucket_ptr b)
          : bucket_(b),
            node_(b ? b->next_ : node_ptr()) {}
        hash_iterator_base(bucket_ptr b, node_ptr n)
          : bucket_(b),
            node_(n) {}
        
        bool operator==(hash_iterator_base const& x) const {
            return node_ == x.node_; }
        bool operator!=(hash_iterator_base const& x) const {
            return node_ != x.node_; }
        value_type& operator*() const {
            return node::get_value(node_);
        }
    
        void increment_bucket(node_ptr n) {
            while(!n) {
                ++bucket_;
                n = bucket_->next_;
            }
            node_ = bucket_ == n ? node_ptr() : n;
        }

        void increment() {
            increment_bucket(node_->next_);
        }
    };

    // hash_buckets
    //
    // This is responsible for allocating and deallocating buckets and nodes.
    //
    // Notes:
    // 1. For the sake exception safety the allocators themselves don't allocate
    //    anything.
    // 2. It's the callers responsibility to allocate the buckets before calling
    //    any of the methods (other than getters and setters).

    template <class A, class G>
    class hash_buckets
    {
        hash_buckets(hash_buckets const&);
        hash_buckets& operator=(hash_buckets const&);
    public:
        // Types

        typedef A value_allocator;
        typedef hash_bucket<A> bucket;
        typedef hash_iterator_base<A, G> iterator_base;
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;
        typedef BOOST_DEDUCED_TYPENAME iterator_base::node node;

        typedef BOOST_DEDUCED_TYPENAME bucket::bucket_allocator
            bucket_allocator;
        typedef BOOST_DEDUCED_TYPENAME bucket::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME bucket::node_ptr node_ptr;

        typedef BOOST_DEDUCED_TYPENAME rebind_wrap<value_allocator, node>::type
            node_allocator;
        typedef BOOST_DEDUCED_TYPENAME node_allocator::pointer real_node_ptr;

        // Members

        bucket_ptr buckets_;
        std::size_t bucket_count_;
        boost::compressed_pair<bucket_allocator, node_allocator> allocators_;
        
        // Data access

        bucket_allocator const& bucket_alloc() const {
            return allocators_.first(); }
        node_allocator const& node_alloc() const {
            return allocators_.second(); }
        bucket_allocator& bucket_alloc() {
            return allocators_.first(); }
        node_allocator& node_alloc() {
            return allocators_.second(); }
        std::size_t max_bucket_count() const;

        // Constructors

        hash_buckets(node_allocator const& a, std::size_t n);
        void create_buckets();
        ~hash_buckets();
        
        // no throw
        void swap(hash_buckets& other);
        void move(hash_buckets& other);

        // For the remaining functions, buckets_ must not be null.
        
        bucket_ptr get_bucket(std::size_t n) const;
        bucket_ptr bucket_ptr_from_hash(std::size_t hashed) const;
        std::size_t bucket_size(std::size_t index) const;
        node_ptr bucket_begin(std::size_t n) const;

        // Alloc/Dealloc
        
        void delete_node(node_ptr);

        // 
        void delete_buckets();
        void clear_bucket(bucket_ptr);
        std::size_t delete_nodes(node_ptr begin, node_ptr end);
        std::size_t delete_to_bucket_end(node_ptr begin);
    };

    template <class H, class P> class set_hash_functions;

    template <class H, class P>
    class hash_buffered_functions
    {
        friend class set_hash_functions<H, P>;
        hash_buffered_functions& operator=(hash_buffered_functions const&);

        typedef boost::compressed_pair<H, P> function_pair;
        typedef BOOST_DEDUCED_TYPENAME boost::aligned_storage<
            sizeof(function_pair),
            ::boost::alignment_of<function_pair>::value>::type aligned_function;

        bool current_; // The currently active functions.
        aligned_function funcs_[2];

        function_pair const& current() const {
            return *static_cast<function_pair const*>(
                static_cast<void const*>(&funcs_[current_]));
        }

        void construct(bool which, H const& hf, P const& eq)
        {
            new((void*) &funcs_[which]) function_pair(hf, eq);
        }

        void construct(bool which, function_pair const& f)
        {
            new((void*) &funcs_[which]) function_pair(f);
        }
        
        void destroy(bool which)
        {
            boost::unordered_detail::destroy((function_pair*)(&funcs_[which]));
        }
        
    public:

        hash_buffered_functions(H const& hf, P const& eq)
            : current_(false)
        {
            construct(current_, hf, eq);
        }

        hash_buffered_functions(hash_buffered_functions const& bf)
            : current_(false)
        {
            construct(current_, bf.current());
        }

        ~hash_buffered_functions() {
            destroy(current_);
        }

        H const& hash_function() const {
            return current().first();
        }

        P const& key_eq() const {
            return current().second();
        }
    };
    
    template <class H, class P>
    class set_hash_functions
    {
        set_hash_functions(set_hash_functions const&);
        set_hash_functions& operator=(set_hash_functions const&);
    
        typedef hash_buffered_functions<H, P> buffered_functions;
        buffered_functions& buffered_functions_;
        bool tmp_functions_;

    public:

        set_hash_functions(buffered_functions& f, H const& h, P const& p)
          : buffered_functions_(f),
            tmp_functions_(!f.current_)
        {
            f.construct(tmp_functions_, h, p);
        }

        set_hash_functions(buffered_functions& f,
            buffered_functions const& other)
          : buffered_functions_(f),
            tmp_functions_(!f.current_)
        {
            f.construct(tmp_functions_, other.current());
        }

        ~set_hash_functions()
        {
            buffered_functions_.destroy(tmp_functions_);
        }

        void commit()
        {
            buffered_functions_.current_ = tmp_functions_;
            tmp_functions_ = !tmp_functions_;
        }
    };

    template <class T>
    class hash_table : public T::buckets, public T::buffered_functions
    {
        hash_table(hash_table const&);
    public:
        typedef BOOST_DEDUCED_TYPENAME T::hasher hasher;
        typedef BOOST_DEDUCED_TYPENAME T::key_equal key_equal;
        typedef BOOST_DEDUCED_TYPENAME T::value_allocator value_allocator;
        typedef BOOST_DEDUCED_TYPENAME T::key_type key_type;
        typedef BOOST_DEDUCED_TYPENAME T::value_type value_type;
        typedef BOOST_DEDUCED_TYPENAME T::buffered_functions base;
        typedef BOOST_DEDUCED_TYPENAME T::buckets buckets;
        typedef BOOST_DEDUCED_TYPENAME T::extractor extractor;
        typedef BOOST_DEDUCED_TYPENAME T::node_constructor node_constructor;

        typedef BOOST_DEDUCED_TYPENAME T::node node;
        typedef BOOST_DEDUCED_TYPENAME T::bucket bucket;
        typedef BOOST_DEDUCED_TYPENAME T::node_ptr node_ptr;
        typedef BOOST_DEDUCED_TYPENAME T::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME T::iterator_base iterator_base;
        typedef BOOST_DEDUCED_TYPENAME T::node_allocator node_allocator;
        typedef BOOST_DEDUCED_TYPENAME T::iterator_pair iterator_pair;

        // Members
        
        std::size_t size_;
        float mlf_;
        // Cached data - invalid if !this->buckets_
        bucket_ptr cached_begin_bucket_;
        std::size_t max_load_;

        // Helper methods

        key_type const& get_key(value_type const& v) const {
            return extractor::extract(v);
        }
        key_type const& get_key_from_ptr(node_ptr n) const {
            return extractor::extract(node::get_value(n));
        }
        bool equal(key_type const& k, value_type const& v) const;
        template <class Key, class Pred>
        node_ptr find_iterator(bucket_ptr bucket, Key const& k,
            Pred const&) const;
        node_ptr find_iterator(bucket_ptr bucket, key_type const& k) const;
        node_ptr find_iterator(key_type const& k) const;
        node_ptr* find_for_erase(bucket_ptr bucket, key_type const& k) const;
        
        // Load methods

        std::size_t max_size() const;
        std::size_t bucket_index(key_type const& k) const;
        void max_load_factor(float z);
        std::size_t min_buckets_for_size(std::size_t n) const;
        std::size_t calculate_max_load();

        // Constructors

        hash_table(std::size_t n, hasher const& hf, key_equal const& eq,
            node_allocator const& a);
        hash_table(hash_table const& x, node_allocator const& a);
        hash_table(hash_table& x, move_tag m);
        hash_table(hash_table& x, node_allocator const& a, move_tag m);
        ~hash_table() {}
        hash_table& operator=(hash_table const&);

        // Iterators

        iterator_base begin() const {
            return this->size_ ?
                iterator_base(this->cached_begin_bucket_) :
                iterator_base();
        }
        iterator_base end() const {
            return iterator_base();
        }

        // Swap & Move

        void swap(hash_table& x);
        void fast_swap(hash_table& other);
        void slow_swap(hash_table& other);
        void partial_swap(hash_table& other);
        void move(hash_table& x);

        // Reserve and rehash

        void create_for_insert(std::size_t n);
        bool reserve_for_insert(std::size_t n);
        void rehash(std::size_t n);
        void rehash_impl(std::size_t n);

        // Move/copy buckets

        void move_buckets_to(buckets& dst);
        void copy_buckets_to(buckets& dst) const;

        // Misc. key methods

        std::size_t count(key_type const& k) const;
        iterator_base find(key_type const& k) const;
        template <class Key, class Hash, class Pred>
        iterator_base find(Key const& k, Hash const& h, Pred const& eq) const;
        value_type& at(key_type const& k) const;
        iterator_pair equal_range(key_type const& k) const;

        // Erase
        //
        // no throw

        void clear();
        std::size_t erase_key(key_type const& k);
        iterator_base erase_return_iterator(iterator_base r);
        void erase(iterator_base r);
        std::size_t erase_group(node_ptr* it, bucket_ptr bucket);
        iterator_base erase_range(iterator_base r1, iterator_base r2);

        // recompute_begin_bucket

        void init_buckets();

        // After an erase cached_begin_bucket_ might be left pointing to
        // an empty bucket, so this is called to update it
        //
        // no throw

        void recompute_begin_bucket(bucket_ptr b);

        // This is called when a range has been erased
        //
        // no throw

        void recompute_begin_bucket(bucket_ptr b1, bucket_ptr b2);
        
        // no throw
        float load_factor() const;
        
        iterator_base emplace_empty_impl_with_node(
            node_constructor&, std::size_t);
    };

    // Iterator Access

#if !defined(__clang__)
    class iterator_access
    {
    public:
        template <class Iterator>
        static BOOST_DEDUCED_TYPENAME Iterator::base const&
            get(Iterator const& it)
        {
            return it.base_;
        }
    };
#else
    class iterator_access
    {
    public:
        // Note: we access Iterator::base here, rather than in the function
        // signature to work around a bug in the friend support of an
        // early version of clang.

        template <class Iterator>
        struct base
        {
            typedef BOOST_DEDUCED_TYPENAME Iterator::base type;
        };
    
        template <class Iterator>
        static BOOST_DEDUCED_TYPENAME base<Iterator>::type const&
            get(Iterator const& it)
        {
            return it.base_;
        }
    };
#endif

    // Iterators

    template <class A, class G> class hash_iterator;
    template <class A, class G> class hash_const_iterator;
    template <class A, class G> class hash_local_iterator;
    template <class A, class G> class hash_const_local_iterator;

    // Local Iterators
    //
    // all no throw

    template <class A, class G>
    class hash_local_iterator
        : public boost::iterator <
            std::forward_iterator_tag,
            BOOST_DEDUCED_TYPENAME A::value_type,
            std::ptrdiff_t,
            BOOST_DEDUCED_TYPENAME A::pointer,
            BOOST_DEDUCED_TYPENAME A::reference>
    {
    public:
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;

    private:
        typedef hash_buckets<A, G> buckets;
        typedef BOOST_DEDUCED_TYPENAME buckets::node_ptr node_ptr;
        typedef BOOST_DEDUCED_TYPENAME buckets::node node;
        typedef hash_const_local_iterator<A, G> const_local_iterator;

        friend class hash_const_local_iterator<A, G>;
        node_ptr ptr_;

    public:
        hash_local_iterator() : ptr_() {}
        explicit hash_local_iterator(node_ptr x) : ptr_(x) {}
        BOOST_DEDUCED_TYPENAME A::reference operator*() const {
            return node::get_value(ptr_);
        }
        value_type* operator->() const {
            return &node::get_value(ptr_);
        }
        hash_local_iterator& operator++() {
            ptr_ = ptr_->next_; return *this;
        }
        hash_local_iterator operator++(int) {
            hash_local_iterator tmp(ptr_); ptr_ = ptr_->next_; return tmp; }
        bool operator==(hash_local_iterator x) const {
            return ptr_ == x.ptr_;
        }
        bool operator==(const_local_iterator x) const {
            return ptr_ == x.ptr_;
        }
        bool operator!=(hash_local_iterator x) const {
            return ptr_ != x.ptr_;
        }
        bool operator!=(const_local_iterator x) const {
            return ptr_ != x.ptr_;
        }
    };

    template <class A, class G>
    class hash_const_local_iterator
        : public boost::iterator <
            std::forward_iterator_tag,
            BOOST_DEDUCED_TYPENAME A::value_type,
            std::ptrdiff_t,
            BOOST_DEDUCED_TYPENAME A::const_pointer,
            BOOST_DEDUCED_TYPENAME A::const_reference >
    {
    public:
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;

    private:
        typedef hash_buckets<A, G> buckets;
        typedef BOOST_DEDUCED_TYPENAME buckets::node_ptr ptr;
        typedef BOOST_DEDUCED_TYPENAME buckets::node node;
        typedef hash_local_iterator<A, G> local_iterator;
        friend class hash_local_iterator<A, G>;
        ptr ptr_;

    public:
        hash_const_local_iterator() : ptr_() {}
        explicit hash_const_local_iterator(ptr x) : ptr_(x) {}
        hash_const_local_iterator(local_iterator x) : ptr_(x.ptr_) {}
        BOOST_DEDUCED_TYPENAME A::const_reference
            operator*() const {
            return node::get_value(ptr_);
        }
        value_type const* operator->() const {
            return &node::get_value(ptr_);
        }
        hash_const_local_iterator& operator++() {
            ptr_ = ptr_->next_; return *this;
        }
        hash_const_local_iterator operator++(int) {
            hash_const_local_iterator tmp(ptr_); ptr_ = ptr_->next_; return tmp;
        }
        bool operator==(local_iterator x) const {
            return ptr_ == x.ptr_;
        }
        bool operator==(hash_const_local_iterator x) const {
            return ptr_ == x.ptr_;
        }
        bool operator!=(local_iterator x) const {
            return ptr_ != x.ptr_;
        }
        bool operator!=(hash_const_local_iterator x) const {
            return ptr_ != x.ptr_;
        }
    };

    // iterators
    //
    // all no throw


    template <class A, class G>
    class hash_iterator
        : public boost::iterator <
            std::forward_iterator_tag,
            BOOST_DEDUCED_TYPENAME A::value_type,
            std::ptrdiff_t,
            BOOST_DEDUCED_TYPENAME A::pointer,
            BOOST_DEDUCED_TYPENAME A::reference >
    {
    public:
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;

    private:
        typedef hash_buckets<A, G> buckets;
        typedef BOOST_DEDUCED_TYPENAME buckets::node node;
        typedef BOOST_DEDUCED_TYPENAME buckets::iterator_base base;
        typedef hash_const_iterator<A, G> const_iterator;
        friend class hash_const_iterator<A, G>;
        base base_;

    public:

        hash_iterator() : base_() {}
        explicit hash_iterator(base const& x) : base_(x) {}
        BOOST_DEDUCED_TYPENAME A::reference operator*() const {
            return *base_;
        }
        value_type* operator->() const {
            return &*base_;
        }
        hash_iterator& operator++() {
            base_.increment(); return *this;
        }
        hash_iterator operator++(int) {
            hash_iterator tmp(base_); base_.increment(); return tmp;
        }
        bool operator==(hash_iterator const& x) const {
            return base_ == x.base_;
        }
        bool operator==(const_iterator const& x) const {
            return base_ == x.base_;
        }
        bool operator!=(hash_iterator const& x) const {
            return base_ != x.base_;
        }
        bool operator!=(const_iterator const& x) const {
            return base_ != x.base_;
        }
    };

    template <class A, class G>
    class hash_const_iterator
        : public boost::iterator <
            std::forward_iterator_tag,
            BOOST_DEDUCED_TYPENAME A::value_type,
            std::ptrdiff_t,
            BOOST_DEDUCED_TYPENAME A::const_pointer,
            BOOST_DEDUCED_TYPENAME A::const_reference >
    {
    public:
        typedef BOOST_DEDUCED_TYPENAME A::value_type value_type;

    private:
        typedef hash_buckets<A, G> buckets;
        typedef BOOST_DEDUCED_TYPENAME buckets::node node;
        typedef BOOST_DEDUCED_TYPENAME buckets::iterator_base base;
        typedef hash_iterator<A, G> iterator;
        friend class hash_iterator<A, G>;
        friend class iterator_access;
        base base_;

    public:

        hash_const_iterator() : base_() {}
        explicit hash_const_iterator(base const& x) : base_(x) {}
        hash_const_iterator(iterator const& x) : base_(x.base_) {}
        BOOST_DEDUCED_TYPENAME A::const_reference operator*() const {
            return *base_;
        }
        value_type const* operator->() const {
            return &*base_;
        }
        hash_const_iterator& operator++() {
            base_.increment(); return *this;
        }
        hash_const_iterator operator++(int) {
            hash_const_iterator tmp(base_); base_.increment(); return tmp;
        }
        bool operator==(iterator const& x) const {
            return base_ == x.base_;
        }
        bool operator==(hash_const_iterator const& x) const {
            return base_ == x.base_;
        }
        bool operator!=(iterator const& x) const {
            return base_ != x.base_;
        }
        bool operator!=(hash_const_iterator const& x) const {
            return base_ != x.base_;
        }
    };

    // types

    template <class K, class V, class H, class P, class A, class E, class G>
    struct types
    {
    public:
        typedef K key_type;
        typedef V value_type;
        typedef H hasher;
        typedef P key_equal;
        typedef A value_allocator;
        typedef E extractor;
        typedef G group_type;
        
        typedef hash_node_constructor<value_allocator, group_type>
            node_constructor;
        typedef hash_buckets<value_allocator, group_type> buckets;
        typedef hash_buffered_functions<hasher, key_equal> buffered_functions;

        typedef BOOST_DEDUCED_TYPENAME buckets::node node;
        typedef BOOST_DEDUCED_TYPENAME buckets::bucket bucket;
        typedef BOOST_DEDUCED_TYPENAME buckets::node_ptr node_ptr;
        typedef BOOST_DEDUCED_TYPENAME buckets::bucket_ptr bucket_ptr;
        typedef BOOST_DEDUCED_TYPENAME buckets::iterator_base iterator_base;
        typedef BOOST_DEDUCED_TYPENAME buckets::node_allocator node_allocator;

        typedef std::pair<iterator_base, iterator_base> iterator_pair;
    };
}}

#endif
