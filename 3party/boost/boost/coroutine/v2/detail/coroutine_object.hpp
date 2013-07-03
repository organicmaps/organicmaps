
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_V1_DETAIL_COROUTINE_OBJECT_H
#define BOOST_COROUTINES_V1_DETAIL_COROUTINE_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/utility.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/exceptions.hpp>
#include <boost/coroutine/detail/flags.hpp>
#include <boost/coroutine/detail/holder.hpp>
#include <boost/coroutine/detail/param.hpp>
#include <boost/coroutine/flags.hpp>
#include <boost/coroutine/stack_context.hpp>
#include <boost/coroutine/v2/detail/arg.hpp>
#include <boost/coroutine/v2/detail/coroutine_base.hpp>

template<
    typename Arg,
    typename Fn, typename StackAllocator, typename Allocator
>
class coroutine_object< Arg, Fn, StackAllocator, Allocator > :
    private stack_tuple< StackAllocator >,
    public coroutine_base
{
public:
    typedef typename Allocator::template rebind<
        coroutine_object<
            Arg, Fn, StackAllocator, Allocator
        >
    >::other                                    allocator_t;

private:
    typedef stack_tuple< StackAllocator >       pbase_type;
    typedef coroutine_base                      base_type;

    Fn                      fn_;
    allocator_t             alloc_;

    static void destroy_( allocator_t & alloc, coroutine_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    coroutine_object( coroutine_object const&);
    coroutine_object & operator=( coroutine_object const&);

    void enter_()
    {
        holder< void > * hldr_from(
            reinterpret_cast< holder< void > * >(
                this->caller_.jump(
                    this->callee_,
                    reinterpret_cast< intptr_t >( this),
                    this->preserve_fpu() ) ) );
        this->callee_ = * hldr_from->ctx;
        this->result_ = hldr_from->data;
        if ( this->except_) rethrow_exception( this->except_);
    }

    void run_( Caller & c)
    {
        coroutine_context callee;
        coroutine_context caller;
        try
        {
            fn_( c);
            this->flags_ |= flag_complete;
            callee = c.impl_->callee_;
            holder< void > hldr_to( & caller);
            caller.jump(
                callee,
                reinterpret_cast< intptr_t >( & hldr_to),
                this->preserve_fpu() );
            BOOST_ASSERT_MSG( false, "coroutine is complete");
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { this->except_ = current_exception(); }

        this->flags_ |= flag_complete;
        callee = c.impl_->callee_;
        holder< void > hldr_to( & caller);
        caller.jump(
            callee,
            reinterpret_cast< intptr_t >( & hldr_to),
            this->preserve_fpu() );
        BOOST_ASSERT_MSG( false, "coroutine is complete");
    }

    void unwind_stack_() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! this->is_complete() );

        this->flags_ |= flag_unwind_stack;
        holder< void > hldr_to( & this->caller_, true);
        this->caller_.jump(
            this->callee_,
            reinterpret_cast< intptr_t >( & hldr_to),
            this->preserve_fpu() );
        this->flags_ &= ~flag_unwind_stack;

        BOOST_ASSERT( this->is_complete() );
    }

public:
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                      StackAllocator const& stack_alloc,
                      allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline1< coroutine_object >,
            & this->stack_ctx,
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( forward< Fn >( fn) ),
        alloc_( alloc)
    { enter_(); }
#else
    coroutine_object( Fn fn, attributes const& attr,
                      StackAllocator const& stack_alloc,
                      allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline1< coroutine_object >,
            & this->stack_ctx,
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }

    coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attr,
                      StackAllocator const& stack_alloc,
                      allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline1< coroutine_object >,
            & this->stack_ctx,
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }
#endif

    ~coroutine_object()
    {
        if ( ! this->is_complete() && this->force_unwind() )
            unwind_stack_();
    }

    void run()
    {
        Caller c( this->caller_, false, this->preserve_fpu(), alloc_);
        run_( c);
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

template<
    typename Arg,
    typename Fn, typename StackAllocator, typename Allocator,
    typename Caller,
    typename Result
>
class coroutine_object< Arg, reference_wrapper< Fn >, StackAllocator, Allocator, Caller, Result, 0 > :
    private stack_tuple< StackAllocator >,
    public coroutine_base< Arg >
{
public:
    typedef typename Allocator::template rebind<
        coroutine_object<
            Arg, Fn, StackAllocator, Allocator, Caller, Result, 0
        >
    >::other                                            allocator_t;

private:
    typedef stack_tuple< StackAllocator >               pbase_type;
    typedef coroutine_base< Arg >                 base_type;

    Fn                      fn_;
    allocator_t             alloc_;

    static void destroy_( allocator_t & alloc, coroutine_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    coroutine_object( coroutine_object const&);
    coroutine_object & operator=( coroutine_object const&);

    void enter_()
    {
        holder< Result > * hldr_from(
            reinterpret_cast< holder< Result > * >(
                this->caller_.jump(
                    this->callee_,
                    reinterpret_cast< intptr_t >( this),
                    this->preserve_fpu() ) ) );
        this->callee_ = * hldr_from->ctx;
        this->result_ = hldr_from->data;
        if ( this->except_) rethrow_exception( this->except_);
    }

    void run_( Caller & c)
    {
        coroutine_context callee;
        coroutine_context caller;
        try
        {
            fn_( c);
            this->flags_ |= flag_complete;
            callee = c.impl_->callee_;
            holder< Result > hldr_to( & caller);
            caller.jump(
                callee,
                reinterpret_cast< intptr_t >( & hldr_to),
                this->preserve_fpu() );
            BOOST_ASSERT_MSG( false, "coroutine is complete");
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { this->except_ = current_exception(); }

        this->flags_ |= flag_complete;
        callee = c.impl_->callee_;
        holder< Result > hldr_to( & caller);
        caller.jump(
            callee,
            reinterpret_cast< intptr_t >( & hldr_to),
            this->preserve_fpu() );
        BOOST_ASSERT_MSG( false, "coroutine is complete");
    }

    void unwind_stack_() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! this->is_complete() );

        this->flags_ |= flag_unwind_stack;
        holder< void > hldr_to( & this->caller_, true);
        this->caller_.jump(
            this->callee_,
            reinterpret_cast< intptr_t >( & hldr_to),
            this->preserve_fpu() );
        this->flags_ &= ~flag_unwind_stack;

        BOOST_ASSERT( this->is_complete() );
    }

public:
    coroutine_object( reference_wrapper< Fn > fn, attributes const& attr,
                      StackAllocator const& stack_alloc,
                      allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline1< coroutine_object >,
            & this->stack_ctx,
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }

    ~coroutine_object()
    {
        if ( ! this->is_complete() && this->force_unwind() )
            unwind_stack_();
    }

    void run()
    {
        Caller c( this->caller_, false, this->preserve_fpu(), alloc_);
        run_( c);
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};

template<
    typename Arg,
    typename Fn, typename StackAllocator, typename Allocator,
    typename Caller,
    typename Result
>
class coroutine_object< Arg, const reference_wrapper< Fn >, StackAllocator, Allocator, Caller, Result, 0 > :
    private stack_tuple< StackAllocator >,
    public coroutine_base< Arg >
{
public:
    typedef typename Allocator::template rebind<
        coroutine_object<
            Arg, Fn, StackAllocator, Allocator, Caller, Result, 0
        >
    >::other                                            allocator_t;

private:
    typedef stack_tuple< StackAllocator >               pbase_type;
    typedef coroutine_base< Arg >                 base_type;

    Fn                      fn_;
    allocator_t             alloc_;

    static void destroy_( allocator_t & alloc, coroutine_object * p)
    {
        alloc.destroy( p);
        alloc.deallocate( p, 1);
    }

    coroutine_object( coroutine_object const&);
    coroutine_object & operator=( coroutine_object const&);

    void enter_()
    {
        holder< Result > * hldr_from(
            reinterpret_cast< holder< Result > * >(
                this->caller_.jump(
                    this->callee_,
                    reinterpret_cast< intptr_t >( this),
                    this->preserve_fpu() ) ) );
        this->callee_ = hldr_from->ctx;
        this->result_ = hldr_from->data;
        if ( this->except_) rethrow_exception( this->except_);
    }

    void run_( Caller & c)
    {
        coroutine_context callee;
        coroutine_context caller;
        try
        {
            fn_( c);
            this->flags_ |= flag_complete;
            callee = c.impl_->callee_;
            holder< Result > hldr_to( & caller);
            caller.jump(
                callee,
                reinterpret_cast< intptr_t >( & hldr_to),
                this->preserve_fpu() );
            BOOST_ASSERT_MSG( false, "coroutine is complete");
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { this->except_ = current_exception(); }

        this->flags_ |= flag_complete;
        callee = c.impl_->callee_;
        holder< Result > hldr_to( & caller);
        caller.jump(
            callee,
            reinterpret_cast< intptr_t >( & hldr_to),
            this->preserve_fpu() );
        BOOST_ASSERT_MSG( false, "coroutine is complete");
    }

    void unwind_stack_() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! this->is_complete() );

        this->flags_ |= flag_unwind_stack;
        holder< void > hldr_to( & this->caller_, true);
        this->caller_.jump(
            this->callee_,
            reinterpret_cast< intptr_t >( & hldr_to),
            this->preserve_fpu() );
        this->flags_ &= ~flag_unwind_stack;

        BOOST_ASSERT( this->is_complete() );
    }

public:
    coroutine_object( const reference_wrapper< Fn > fn, attributes const& attr,
                      StackAllocator const& stack_alloc,
                      allocator_t const& alloc) :
        pbase_type( stack_alloc, attr.size),
        base_type(
            trampoline1< coroutine_object >,
            & this->stack_ctx,
            stack_unwind == attr.do_unwind,
            fpu_preserved == attr.preserve_fpu),
        fn_( fn),
        alloc_( alloc)
    { enter_(); }

    ~coroutine_object()
    {
        if ( ! this->is_complete() && this->force_unwind() )
            unwind_stack_();
    }

    void run()
    {
        Caller c( & this->caller_, false, this->preserve_fpu(), alloc_);
        run_( c);
    }

    void deallocate_object()
    { destroy_( alloc_, this); }
};
