/*=============================================================================
    Copyright (c) 2006 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FIND_IF_S_05152006_1027)
#define FIND_IF_S_05152006_1027

#include <boost/mpl/not.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/fusion/algorithm/query/find_if.hpp>
#include <boost/fusion/container/list/cons.hpp>
#include <boost/fusion/sequence/intrinsic/ext_/segments.hpp>
#include <boost/fusion/view/ext_/segmented_iterator.hpp>
#include <boost/fusion/view/ext_/segmented_iterator_range.hpp>
#include <boost/fusion/support/ext_/is_segmented.hpp>

// fwd declarations
namespace boost { namespace fusion
{
    namespace detail
    {
        template<typename Sequence, typename Pred, bool IsSegmented = traits::is_segmented<Sequence>::value>
        struct static_find_if_s_recurse;
    }

    namespace result_of
    {
        template <typename Sequence, typename Pred>
        struct find_if_s;
    }
}}

namespace boost { namespace fusion { namespace detail
{

    template<typename Sequence, typename Where, bool IsSegmented = traits::is_segmented<Sequence>::value>
    struct is_found
      : mpl::not_<result_of::equal_to<Where, typename result_of::end<Sequence>::type> >
    {};

    template<typename Sequence, typename Cons>
    struct is_found<Sequence, Cons, true>
      : mpl::not_<is_same<nil, Cons> >
    {};

    template<
        typename SegmentedRange
      , typename Where
      , typename Sequence = typename remove_reference<
            typename result_of::deref<
                typename SegmentedRange::iterator_type
            >::type
        >::type
      , bool IsSegmented = traits::is_segmented<Sequence>::value
    >
    struct as_segmented_cons
    {
        typedef cons<
            SegmentedRange
          , cons<segmented_range<Sequence, Where, false> >
        > type;

        static type call(SegmentedRange const &range, Where const &where)
        {
            return fusion::make_cons(
                range
              , fusion::make_cons(
                    segmented_range<Sequence, Where, false>(*fusion::begin(range), where)
                )
            );
        }
    };

    template<
        typename SegmentedRange
      , typename Where
      , typename Sequence
    >
    struct as_segmented_cons<SegmentedRange, Where, Sequence, true>
    {
        typedef cons<SegmentedRange, Where> type;

        static type call(SegmentedRange const &range, Where const &where)
        {
            return fusion::make_cons(range, where);
        }
    };

    template<
        typename SegmentedRange
      , typename Pred
      , bool IsEmpty = is_empty<SegmentedRange>::value
    >
    struct static_find_if_s_seg
    {
        typedef typename SegmentedRange::iterator_type first;
        typedef typename result_of::deref<first>::type segment_ref;
        typedef typename remove_reference<segment_ref>::type segment;
        typedef static_find_if_s_recurse<segment, Pred> where;
        typedef range_next<SegmentedRange> next;
        typedef is_found<segment, typename where::type> is_found;
        typedef as_segmented_cons<SegmentedRange, typename where::type> found;
        typedef static_find_if_s_seg<typename next::type, Pred> not_found;
        typedef typename mpl::eval_if<is_found, found, not_found>::type type;

        static type call(SegmentedRange const &range)
        {
            return call_(range, is_found());
        }

    private:
        static type call_(SegmentedRange const &range, mpl::true_)
        {
            return found::call(range, where::call(*range.where_));
        }

        static type call_(SegmentedRange const &range, mpl::false_)
        {
            return not_found::call(next::call(range));
        }
    };

    template<
        typename SegmentedRange
      , typename Pred
    >
    struct static_find_if_s_seg<SegmentedRange, Pred, true>
    {
        typedef nil type;

        static type call(SegmentedRange const &)
        {
            return nil();
        }
    };

    template<typename Sequence, typename Pred>
    struct static_find_if_s_recurse<Sequence, Pred, true>
    {
        typedef typename as_segmented_range<Sequence>::type range;
        typedef static_find_if_s_seg<range, Pred> find_if;
        typedef typename find_if::type type;

        static type call(Sequence &seq)
        {
            return find_if::call(range(fusion::segments(seq)));
        }
    };

    template<typename Sequence, typename Pred>
    struct static_find_if_s_recurse<Sequence, Pred, false>
    {
        typedef typename result_of::find_if<Sequence, Pred>::type type;

        static type call(Sequence &seq)
        {
            return fusion::find_if<Pred>(seq);
        }
    };

    template<typename Sequence, typename Pred, bool IsSegmented = traits::is_segmented<Sequence>::value>
    struct static_find_if_s
      : static_find_if_s_recurse<Sequence, Pred, IsSegmented>
    {};
    
    template<typename Sequence, typename Pred>
    struct static_find_if_s<Sequence, Pred, true>
    {
        typedef typename as_segmented_range<Sequence>::type range;
        typedef static_find_if_s_recurse<Sequence, Pred> find_if;
        typedef typename find_if::type found;

        typedef segmented_iterator<typename reverse_cons<found>::type> type;

        static type call(Sequence &seq)
        {
            return type(reverse_cons<found>::call(find_if::call(seq)));
        }
    };
}}}

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence, typename Pred>
        struct find_if_s
        {
            typedef typename
                detail::static_find_if_s<
                    Sequence
                  , Pred
                >::type
            type;
        };
    }

    template <typename Pred, typename Sequence>
    typename lazy_disable_if<
        is_const<Sequence>
      , result_of::find_if_s<Sequence, Pred>
    >::type
    find_if_s(Sequence& seq)
    {
        return detail::static_find_if_s<Sequence, Pred>::call(seq);
    }

    template <typename Pred, typename Sequence>
    typename result_of::find_if_s<Sequence const, Pred>::type
    find_if_s(Sequence const& seq)
    {
        return detail::static_find_if_s<Sequence const, Pred>::call(seq);
    }
}}

#endif
