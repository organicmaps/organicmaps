
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PULL_COROUTINE_HPP
#define BOOST_COROUTINES2_DETAIL_PULL_COROUTINE_HPP

#include <iterator>
#include <type_traits>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/execution_context.hpp>

#include <boost/coroutine2/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

template< typename T >
class pull_coroutine {
private:
    template< typename X >
    friend class push_coroutine;

    struct control_block;

    control_block   *   cb_;

    explicit pull_coroutine( control_block *);

    bool has_result_() const;

public:
    template< typename Fn >
    explicit pull_coroutine( Fn &&, bool = false);

    template< typename StackAllocator, typename Fn >
    explicit pull_coroutine( StackAllocator, Fn &&, bool = false);

    ~pull_coroutine();

    pull_coroutine( pull_coroutine const&) = delete;
    pull_coroutine & operator=( pull_coroutine const&) = delete;

    pull_coroutine( pull_coroutine &&);

    pull_coroutine & operator=( pull_coroutine && other) {
        if ( this != & other) {
            cb_ = other.cb_;
            other.cb_ = nullptr;
        }
        return * this;
    }

    pull_coroutine & operator()();

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;

    T get() const noexcept;

    class iterator : public std::iterator< std::input_iterator_tag, typename std::remove_reference< T >::type > {
    private:
        pull_coroutine< T > *   c_;

        void fetch_() {
            BOOST_ASSERT( nullptr != c_);
            if ( ! ( * c_) ) {
                c_ = nullptr;
                return;
            }
        }

        void increment_() {
            BOOST_ASSERT( nullptr != c_);
            BOOST_ASSERT( * c_);
            ( * c_)();
            fetch_();
        }

    public:
        typedef typename iterator::pointer pointer_t;
        typedef typename iterator::reference reference_t;

        iterator() :
            c_( nullptr) {
        }

        explicit iterator( pull_coroutine< T > * c) :
            c_( c) {
            fetch_();
        }

        iterator( iterator const& other) :
            c_( other.c_) {
        }

        iterator & operator=( iterator const& other) {
            if ( this == & other) return * this;
            c_ = other.c_;
            return * this;
        }

        bool operator==( iterator const& other) const {
            return other.c_ == c_;
        }

        bool operator!=( iterator const& other) const {
            return other.c_ != c_;
        }

        iterator & operator++() {
            increment_();
            return * this;
        }

        iterator operator++( int) = delete;

        reference_t operator*() const {
            return * c_->cb_->other->t;
        }

        pointer_t operator->() const {
            return c_->cb_->other->t;
        }
    };

    friend class iterator;
};

template< typename T >
class pull_coroutine< T & > {
private:
    template< typename X >
    friend class push_coroutine;

    struct control_block;

    control_block   *   cb_;

    explicit pull_coroutine( control_block *);

    bool has_result_() const;

public:
    template< typename Fn >
    explicit pull_coroutine( Fn &&, bool = false);

    template< typename StackAllocator, typename Fn >
    explicit pull_coroutine( StackAllocator, Fn &&, bool = false);

    ~pull_coroutine();

    pull_coroutine( pull_coroutine const&) = delete;
    pull_coroutine & operator=( pull_coroutine const&) = delete;

    pull_coroutine( pull_coroutine &&);

    pull_coroutine & operator=( pull_coroutine && other) {
        if ( this != & other) {
            cb_ = other.cb_;
            other.cb_ = nullptr;
        }
        return * this;
    }

    pull_coroutine & operator()();

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;

    T & get() const noexcept;

    class iterator : public std::iterator< std::input_iterator_tag, typename std::remove_reference< T >::type > {
    private:
        pull_coroutine< T & > *   c_;

        void fetch_() {
            BOOST_ASSERT( nullptr != c_);
            if ( ! ( * c_) ) {
                c_ = nullptr;
                return;
            }
        }

        void increment_() {
            BOOST_ASSERT( nullptr != c_);
            BOOST_ASSERT( * c_);
            ( * c_)();
            fetch_();
        }

    public:
        typedef typename iterator::pointer pointer_t;
        typedef typename iterator::reference reference_t;

        iterator() :
            c_( nullptr) {
        }

        explicit iterator( pull_coroutine< T & > * c) :
            c_( c) {
            fetch_();
        }

        iterator( iterator const& other) :
            c_( other.c_) {
        }

        iterator & operator=( iterator const& other) {
            if ( this == & other) return * this;
            c_ = other.c_;
            return * this;
        }

        bool operator==( iterator const& other) const {
            return other.c_ == c_;
        }

        bool operator!=( iterator const& other) const {
            return other.c_ != c_;
        }

        iterator & operator++() {
            increment_();
            return * this;
        }

        iterator operator++( int) = delete;

        reference_t operator*() const {
            return * c_->cb_->other->t;
        }

        pointer_t operator->() const {
            return c_->cb_->other->t;
        }
    };

    friend class iterator;
};

template<>
class pull_coroutine< void > {
private:
    template< typename X >
    friend class push_coroutine;

    struct control_block;

    control_block   *   cb_;

    explicit pull_coroutine( control_block *);

public:
    template< typename Fn >
    explicit pull_coroutine( Fn &&, bool = false);

    template< typename StackAllocator, typename Fn >
    explicit pull_coroutine( StackAllocator, Fn &&, bool = false);

    ~pull_coroutine();

    pull_coroutine( pull_coroutine const&) = delete;
    pull_coroutine & operator=( pull_coroutine const&) = delete;

    pull_coroutine( pull_coroutine &&);

    pull_coroutine & operator=( pull_coroutine && other) {
        if ( this != & other) {
            cb_ = other.cb_;
            other.cb_ = nullptr;
        }
        return * this;
    }

    pull_coroutine & operator()();

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;
};

template< typename T >
typename pull_coroutine< T >::iterator
begin( pull_coroutine< T > & c) {
    return typename pull_coroutine< T >::iterator( & c);
}

template< typename T >
typename pull_coroutine< T >::iterator
end( pull_coroutine< T > &) {
    return typename pull_coroutine< T >::iterator();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PULL_COROUTINE_HPP
