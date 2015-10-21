
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_EXECUTION_CONTEXT_H
#define BOOST_CONTEXT_EXECUTION_CONTEXT_H

#include <boost/context/detail/config.hpp>

#if ! defined(BOOST_CONTEXT_NO_EXECUTION_CONTEXT)

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/context/detail/invoke.hpp>
#include <boost/context/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

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
        LPVOID                  fiber;
        stack_context           sctx;
        int                     flags;

        // used for toplevel-context
        // (e.g. main context, thread-entry context)
        activation_record() noexcept :
            use_count( 1),
            fiber( nullptr),
            sctx(),
            flags( flag_main_ctx) {
        }

        activation_record( stack_context sctx_, bool use_segmented_stack) noexcept :
            use_count( 0),
            fiber( nullptr),
            sctx( sctx_),
            flags( use_segmented_stack ? flag_segmented_stack : 0) {
        }

        virtual ~activation_record() noexcept = default;

        void resume() noexcept {
            // store current activation record in local variable
            activation_record * from = current_rec.get();
            // store `this` in static, thread local pointer
            // `this` will become the active (running) context
            // returned by execution_context::current()
            current_rec = this;
            // context switch from parent context to `this`-context
#if ( _WIN32_WINNT > 0x0600)
            if ( ::IsThreadAFiber() ) {
                from->fiber = ::GetCurrentFiber();
            } else {
                from->fiber = ::ConvertThreadToFiber( nullptr);
            }
#else
            from->fiber = ::ConvertThreadToFiber( nullptr);
            if ( nullptr == from->fiber) {
                DWORD err = ::GetLastError();
                BOOST_ASSERT( ERROR_ALREADY_FIBER == err);
                from->fiber = ::GetCurrentFiber();
                BOOST_ASSERT( nullptr != from->fiber);
                BOOST_ASSERT( reinterpret_cast< LPVOID >( 0x1E00) != from->fiber);
            }
#endif
            ::SwitchToFiber( fiber);
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
        explicit capture_record( stack_context sctx, StackAlloc const& salloc, Fn && fn, bool use_segmented_stack) noexcept :
            activation_record( sctx, use_segmented_stack),
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
    static VOID WINAPI entry_func( LPVOID p) {
        BOOST_ASSERT( 0 != p);

        AR * ar( reinterpret_cast< AR * >( p) );
        // start execution of toplevel context-function
        ar->run();
        //ctx->fn_(ctx->param_);
        ::DeleteFiber( ar->fiber);
    }

    typedef boost::intrusive_ptr< activation_record >    ptr_t;

    ptr_t   ptr_;

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( StackAlloc salloc, Fn && fn, bool use_segmented_stack) {
        typedef capture_record< Fn, StackAlloc >  capture_t;

        // hackish
        std::size_t fsize = salloc.size_;
        salloc.size_ = sizeof( capture_t);

        stack_context sctx( salloc.allocate() );
        // reserve space for control structure
        void * sp = static_cast< char * >( sctx.sp) - sizeof( capture_t);
        // placment new for control structure on fast-context stack
        capture_t * cr = new ( sp) capture_t( sctx, salloc, std::forward< Fn >( fn), use_segmented_stack);
        // create fiber
        // use default stacksize
        cr->fiber = ::CreateFiber( fsize, execution_context::entry_func< capture_t >, cr);
        BOOST_ASSERT( nullptr != cr->fiber);
        return cr;
    }

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( preallocated palloc, StackAlloc salloc, Fn && fn, bool use_segmented_stack) {
        typedef capture_record< Fn, StackAlloc >  capture_t;

        // hackish
        std::size_t fsize = salloc.size_;
        salloc.size_ = sizeof( capture_t);

        // reserve space for control structure
        void * sp = static_cast< char * >( palloc.sp) - sizeof( capture_t);
        // placment new for control structure on fast-context stack
        capture_t * cr = new ( sp) capture_t( palloc.sctx, salloc, std::forward< Fn >( fn), use_segmented_stack);
        // create fiber
        // use default stacksize
        cr->fiber = ::CreateFiber( fsize, execution_context::entry_func< capture_t >, cr);
        BOOST_ASSERT( nullptr != cr->fiber);
        return cr;
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
        ptr_->resume();
    }
};

}}

# ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
# endif

#endif

#endif // BOOST_CONTEXT_EXECUTION_CONTEXT_H
