
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_HPP
#define BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_HPP

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
class push_coroutine {
private:
    template< typename X >
    friend class pull_coroutine;

    struct control_block;

    control_block   *   cb_;

    explicit push_coroutine( control_block *);

public:
    template< typename Fn >
    explicit push_coroutine( Fn &&, bool = false);

    template< typename StackAllocator, typename Fn >
    explicit push_coroutine( StackAllocator, Fn &&, bool = false);

    ~push_coroutine();

    push_coroutine( push_coroutine const&) = delete;
    push_coroutine & operator=( push_coroutine const&) = delete;

    push_coroutine( push_coroutine &&);

    push_coroutine & operator=( push_coroutine && other) {
        if ( this != & other) {
            cb_ = other.cb_;
            other.cb_ = nullptr;
        }
        return * this;
    }

    push_coroutine & operator()( T const&);

    push_coroutine & operator()( T &&);

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;

    class iterator : public std::iterator< std::output_iterator_tag, void, void, void, void > {
    private:
        push_coroutine< T > *   c_;

    public:
        iterator() :
            c_( nullptr) {
        }

        explicit iterator( push_coroutine< T > * c) :
            c_( c) {
        }

        iterator & operator=( T t) {
            BOOST_ASSERT( c_);
            if ( ! ( * c_)( t) ) c_ = 0;
            return * this;
        }

        bool operator==( iterator const& other) const {
            return other.c_ == c_;
        }

        bool operator!=( iterator const& other) const {
            return other.c_ != c_;
        }

        iterator & operator*() {
            return * this;
        }

        iterator & operator++() {
            return * this;
        }
    };
};

template< typename T >
class push_coroutine< T & > {
private:
    template< typename X >
    friend class pull_coroutine;

    struct control_block;

    control_block   *   cb_;

    explicit push_coroutine( control_block *);

public:
    template< typename Fn >
    explicit push_coroutine( Fn &&, bool = false);

    template< typename StackAllocator, typename Fn >
    explicit push_coroutine( StackAllocator, Fn &&, bool = false);

    ~push_coroutine();

    push_coroutine( push_coroutine const&) = delete;
    push_coroutine & operator=( push_coroutine const&) = delete;

    push_coroutine( push_coroutine &&);

    push_coroutine & operator=( push_coroutine && other) {
        if ( this != & other) {
            cb_ = other.cb_;
            other.cb_ = nullptr;
        }
        return * this;
    }

    push_coroutine & operator()( T &);

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;

    class iterator : public std::iterator< std::output_iterator_tag, void, void, void, void > {
    private:
        push_coroutine< T & >   *   c_;

    public:
        iterator() :
            c_( nullptr) {
        }

        explicit iterator( push_coroutine< T & > * c) :
            c_( c) {
        }

        iterator & operator=( T & t) {
            BOOST_ASSERT( c_);
            if ( ! ( * c_)( t) ) c_ = 0;
            return * this;
        }

        bool operator==( iterator const& other) const {
            return other.c_ == c_;
        }

        bool operator!=( iterator const& other) const {
            return other.c_ != c_;
        }

        iterator & operator*() {
            return * this;
        }

        iterator & operator++() {
            return * this;
        }
    };
};

template<>
class push_coroutine< void > {
private:
    template< typename X >
    friend class pull_coroutine;

    struct control_block;

    control_block   *   cb_;

    explicit push_coroutine( control_block *);

public:
    template< typename Fn >
    explicit push_coroutine( Fn &&, bool = false);

    template< typename StackAllocator, typename Fn >
    explicit push_coroutine( StackAllocator, Fn &&, bool = false);

    ~push_coroutine();

    push_coroutine( push_coroutine const&) = delete;
    push_coroutine & operator=( push_coroutine const&) = delete;

    push_coroutine( push_coroutine &&);

    push_coroutine & operator=( push_coroutine && other) {
        if ( this != & other) {
            cb_ = other.cb_;
            other.cb_ = nullptr;
        }
        return * this;
    }

    push_coroutine & operator()();

    explicit operator bool() const noexcept;

    bool operator!() const noexcept;
};

template< typename T >
typename push_coroutine< T >::iterator
begin( push_coroutine< T > & c) {
    return typename push_coroutine< T >::iterator( & c);
}

template< typename T >
typename push_coroutine< T >::iterator
end( push_coroutine< T > &) {
    return typename push_coroutine< T >::iterator();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_HPP
