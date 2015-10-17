
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_EXECUTION_CONTEXT_H
#define BOOST_CONTEXT_EXECUTION_CONTEXT_H

#include <boost/context/detail/config.hpp>

#if ! defined(BOOST_CONTEXT_NO_EXECUTION_CONTEXT)

# include <algorithm>
# include <cstddef>
# include <cstdint>
# include <cstdlib>
# include <functional>
# include <memory>
# include <tuple>
# include <utility>

# include <boost/assert.hpp>
# include <boost/config.hpp>
# include <boost/context/fcontext.hpp>
# include <boost/intrusive_ptr.hpp>

# include <boost/context/detail/invoke.hpp>
# include <boost/context/stack_context.hpp>
# include <boost/context/segmented_stack.hpp>

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
# endif

# if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C" {

void __splitstack_getcontext( void * [BOOST_CONTEXT_SEGMENTS]);

void __splitstack_setcontext( void * [BOOST_CONTEXT_SEGMENTS]);

}
# endif

namespace boost {
namespace context {

struct preallocated {
    void        *   sp;
    std::size_t     size;
    stack_context   sctx;

    preallocated( void * sp_, std::size_t size_, stack_context sctx_) noexcept :
        sp( sp_), size( size_), sctx( sctx_) {
    }
};

class BOOST_CONTEXT_DECL execution_context {
private:
    struct activation_record {
        typedef boost::intrusive_ptr< activation_record >    ptr_t;

        enum flag_t {
            flag_main_ctx   = 1 << 1,
            flag_preserve_fpu = 1 << 2,
            flag_segmented_stack = 1 << 3
        };

        thread_local static activation_record       toplevel_rec;
        thread_local static ptr_t                   current_rec;

        std::size_t             use_count;
        fcontext_t              fctx;
        stack_context           sctx;
        int                     flags;

        // used for toplevel-context
        // (e.g. main context, thread-entry context)
        activation_record() noexcept :
            use_count( 1),
            fctx( nullptr),
            sctx(),
            flags( flag_main_ctx) {
        }

        activation_record( fcontext_t fctx_, stack_context sctx_, bool use_segmented_stack) noexcept :
            use_count( 0),
            fctx( fctx_),
            sctx( sctx_),
            flags( use_segmented_stack ? flag_segmented_stack : 0) {
        }

        virtual ~activation_record() noexcept = default;

        void resume( bool fpu = false) noexcept {
            // store current activation record in local variable
            activation_record * from = current_rec.get();
            // store `this` in static, thread local pointer
            // `this` will become the active (running) context
            // returned by execution_context::current()
            current_rec = this;
            // set FPU flag
            if (fpu) {
                from->flags |= flag_preserve_fpu;
                this->flags |= flag_preserve_fpu;
            } else {
                from->flags &= ~flag_preserve_fpu;
                this->flags &= ~flag_preserve_fpu;
            }
# if defined(BOOST_USE_SEGMENTED_STACKS)
            if ( 0 != (flags & flag_segmented_stack) ) {
                // adjust segmented stack properties
                __splitstack_getcontext( from->sctx.segments_ctx);
                __splitstack_setcontext( sctx.segments_ctx);
                // context switch from parent context to `this`-context
                jump_fcontext( & from->fctx, fctx, reinterpret_cast< intptr_t >( this), fpu);
                // parent context resumed
                // adjust segmented stack properties
                __splitstack_setcontext( from->sctx.segments_ctx);
            } else {
                // context switch from parent context to `this`-context
                jump_fcontext( & from->fctx, fctx, reinterpret_cast< intptr_t >( this), fpu);
                // parent context resumed
            }
# else
            // context switch from parent context to `this`-context
            jump_fcontext( & from->fctx, fctx, reinterpret_cast< intptr_t >( this), fpu);
            // parent context resumed
# endif
        }

        virtual void deallocate() {}

        friend void intrusive_ptr_add_ref( activation_record * ar) {
            ++ar->use_count;
        }

        friend void intrusive_ptr_release( activation_record * ar) {
            BOOST_ASSERT( nullptr != ar);

            if ( 0 == --ar->use_count) {
                ar->deallocate();
            }
        }
    };

    template< typename Fn, typename StackAlloc >
    class capture_record : public activation_record {
    private:
        StackAlloc      salloc_;
        Fn              fn_;

        static void destroy( capture_record * p) {
            StackAlloc salloc( p->salloc_);
            stack_context sctx( p->sctx);
            // deallocate activation record
            p->~capture_record();
            // destroy stack with stack allocator
            salloc.deallocate( sctx);
        }

    public:
        explicit capture_record( stack_context sctx, StackAlloc const& salloc, fcontext_t fctx, Fn && fn, bool use_segmented_stack) noexcept :
            activation_record( fctx, sctx, use_segmented_stack),
            salloc_( salloc),
            fn_( std::forward< Fn >( fn) ) {
        }

        void deallocate() override final {
            destroy( this);
        }

        void run() noexcept {
            try {
                fn_();
            } catch (...) {
                std::terminate();
            }
            BOOST_ASSERT( 0 == (flags & flag_main_ctx) );
        }
    };

    // tampoline function
    // entered if the execution context
    // is resumed for the first time
    template< typename AR >
    static void entry_func( intptr_t p) noexcept {
        BOOST_ASSERT( 0 != p);

        AR * ar( reinterpret_cast< AR * >( p) );
        BOOST_ASSERT( nullptr != ar);

        // start execution of toplevel context-function
        ar->run();
    }

    typedef boost::intrusive_ptr< activation_record >    ptr_t;

    ptr_t   ptr_;

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( StackAlloc salloc, Fn && fn, bool use_segmented_stack) {
        typedef capture_record< Fn, StackAlloc >  capture_t;

        stack_context sctx( salloc.allocate() );
        // reserve space for control structure
#if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
        std::size_t size = sctx.size - sizeof( capture_t);
        void * sp = static_cast< char * >( sctx.sp) - sizeof( capture_t);
#else
        constexpr std::size_t func_alignment = 64; // alignof( capture_t);
        constexpr std::size_t func_size = sizeof( capture_t);
        // reserve space on stack
        void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
        // align sp pointer
        std::size_t space = func_size + func_alignment;
        sp = std::align( func_alignment, func_size, sp, space);
        BOOST_ASSERT( nullptr != sp);
        // calculate remaining size
        std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
        // create fast-context
        fcontext_t fctx = make_fcontext( sp, size, & execution_context::entry_func< capture_t >);
        BOOST_ASSERT( nullptr != fctx);
        // placment new for control structure on fast-context stack
        return new ( sp) capture_t( sctx, salloc, fctx, std::forward< Fn >( fn), use_segmented_stack);
    }

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( preallocated palloc, StackAlloc salloc, Fn && fn, bool use_segmented_stack) {
        typedef capture_record< Fn, StackAlloc >  capture_t;

        // reserve space for control structure
#if defined(BOOST_NO_CXX14_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
        std::size_t size = palloc.size - sizeof( capture_t);
        void * sp = static_cast< char * >( palloc.sp) - sizeof( capture_t);
#else
        constexpr std::size_t func_alignment = 64; // alignof( capture_t);
        constexpr std::size_t func_size = sizeof( capture_t);
        // reserve space on stack
        void * sp = static_cast< char * >( palloc.sp) - func_size - func_alignment;
        // align sp pointer
        std::size_t space = func_size + func_alignment;
        sp = std::align( func_alignment, func_size, sp, space);
        BOOST_ASSERT( nullptr != sp);
        // calculate remaining size
        std::size_t size = palloc.size - ( static_cast< char * >( palloc.sp) - static_cast< char * >( sp) );
#endif
        // create fast-context
        fcontext_t fctx = make_fcontext( sp, size, & execution_context::entry_func< capture_t >);
        BOOST_ASSERT( nullptr != fctx);
        // placment new for control structure on fast-context stack
        return new ( sp) capture_t( palloc.sctx, salloc, fctx, std::forward< Fn >( fn), use_segmented_stack);
    }

    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    static activation_record * create_capture_record( StackAlloc salloc,
                                                      Fn && fn_, Tpl && tpl_,
                                                      std::index_sequence< I ... >,
                                                      bool use_segmented_stack) {
        return create_context(
            salloc,
            // lambda, executed in new execution context
            [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable -> decltype( auto) {
                 detail::invoke( fn,
                        // non-type template parameter pack used to extract the
                        // parameters (arguments) from the tuple and pass them to fn
                        // via parameter pack expansion
                        // std::tuple_element<> does not perfect forwarding
                        std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                             std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
            },
            use_segmented_stack);
    }

    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    static activation_record * create_capture_record( preallocated palloc, StackAlloc salloc,
                                                      Fn && fn_, Tpl && tpl_,
                                                      std::index_sequence< I ... >,
                                                      bool use_segmented_stack) {
        return create_context(
            palloc, salloc,
            // lambda, executed in new execution context
            [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable -> decltype( auto) {
                 detail::invoke( fn,
                        // non-type template parameter pack used to extract the
                        // parameters (arguments) from the tuple and pass them to fn
                        // via parameter pack expansion
                        // std::tuple_element<> does not perfect forwarding
                        std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                             std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
            },
            use_segmented_stack);
    }

    execution_context() :
        // default constructed with current activation_record
        ptr_( activation_record::current_rec) {
    }

public:
    static execution_context current() noexcept {
        return execution_context();
    }

# if defined(BOOST_USE_SEGMENTED_STACKS)
    template< typename Fn, typename ... Args >
    explicit execution_context( segmented_stack salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), true) ) {
    }

    template< typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, segmented_stack salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( palloc, salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), true) ) {
    }
# endif

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( StackAlloc salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), false) ) {
    }

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, StackAlloc salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( palloc, salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), false) ) {
    }

    void operator()( bool preserve_fpu = false) noexcept {
        ptr_->resume( preserve_fpu);
    }
};

}}

# ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
# endif

#endif

#endif // BOOST_CONTEXT_EXECUTION_CONTEXT_H
