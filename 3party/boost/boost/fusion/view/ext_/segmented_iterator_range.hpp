/*=============================================================================
 Copyright (c) 2006 Eric Niebler

 Use, modification and distribution is subject to the Boost Software
 License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_SEGMENTED_ITERATOR_RANGE_EAN_05032006_1027
#define FUSION_SEGMENTED_ITERATOR_RANGE_EAN_05032006_1027

#include <boost/mpl/bool.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/and.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/fusion/iterator/mpl/convert_iterator.hpp>
#include <boost/fusion/container/list/cons.hpp>
#include <boost/fusion/view/joint_view.hpp>
#include <boost/fusion/view/single_view.hpp>
#include <boost/fusion/view/transform_view.hpp>
#include <boost/fusion/view/iterator_range.hpp>
#include <boost/fusion/view/ext_/multiple_view.hpp>
#include <boost/fusion/view/ext_/segmented_iterator.hpp>
#include <boost/fusion/adapted/mpl/mpl_iterator.hpp>

namespace boost { namespace fusion
{
    namespace detail
    {
        ////////////////////////////////////////////////////////////////////////////
        template<typename Cons, typename State = nil>
        struct reverse_cons;

        template<typename Car, typename Cdr, typename State>
        struct reverse_cons<cons<Car, Cdr>, State>
        {
            typedef reverse_cons<Cdr, cons<Car, State> > reverse;
            typedef typename reverse::type type;

            static type call(cons<Car, Cdr> const &cons, State const &state = State())
            {
                return reverse::call(cons.cdr, fusion::make_cons(cons.car, state));
            }
        };

        template<typename State>
        struct reverse_cons<nil, State>
        {
            typedef State type;

            static State const &call(nil const &, State const &state = State())
            {
                return state;
            }
        };

        ////////////////////////////////////////////////////////////////////////////
        // tags
        struct full_view {};
        struct left_view {};
        struct right_view {};
        struct center_view {};

        template<typename Tag>
        struct segmented_view_tag;

        ////////////////////////////////////////////////////////////////////////////
        // a segmented view of that includes all elements either to the
        // right or the left of a segmented iterator.
        template<typename Tag, typename Cons1, typename Cons2 = void_>
        struct segmented_view
          : sequence_base<segmented_view<Tag, Cons1, Cons2> >
        {
            typedef segmented_view_tag<Tag> fusion_tag;
            typedef fusion_sequence_tag tag; // this gets picked up by MPL
            typedef mpl::true_ is_view;
            typedef forward_traversal_tag category;

            explicit segmented_view(Cons1 const &cons)
              : cons(cons)
            {}

            typedef Cons1 cons_type;
            cons_type const &cons;
        };

        // a segmented view that contains all the elements in between
        // two segmented iterators
        template<typename Cons1, typename Cons2>
        struct segmented_view<center_view, Cons1, Cons2>
          : sequence_base<segmented_view<center_view, Cons1, Cons2> >
        {
            typedef segmented_view_tag<center_view> fusion_tag;
            typedef fusion_sequence_tag tag; // this gets picked up by MPL
            typedef mpl::true_ is_view;
            typedef forward_traversal_tag category;

            segmented_view(Cons1 const &lcons, Cons2 const &rcons)
              : left_cons(lcons)
              , right_cons(rcons)
            {}

            typedef Cons1 left_cons_type;
            typedef Cons2 right_cons_type;

            left_cons_type const &left_cons;
            right_cons_type const &right_cons;
        };

        ////////////////////////////////////////////////////////////////////////////
        // Used to transform a sequence of segments. The first segment is
        // bounded by RightCons, and the last segment is bounded by LeftCons
        // and all the others are passed through unchanged.
        template<typename RightCons, typename LeftCons = RightCons>
        struct segments_transform
        {
            explicit segments_transform(RightCons const &cons_)
              : right_cons(cons_)
              , left_cons(cons_)
            {}

            segments_transform(RightCons const &right_cons_, LeftCons const &left_cons_)
              : right_cons(right_cons_)
              , left_cons(left_cons_)
            {}

            template<typename First, typename Second>
            struct result_;

            template<typename Second>
            struct result_<right_view, Second>
            {
                typedef segmented_view<right_view, RightCons> type;
            };

            template<typename Second>
            struct result_<left_view, Second>
            {
                typedef segmented_view<left_view, LeftCons> type;
            };

            template<typename Second>
            struct result_<full_view, Second>
            {
                typedef Second type;
            };

            template<typename Sig>
            struct result;

            template<typename This, typename First, typename Second>
            struct result<This(First, Second)>
              : result_<
                    typename remove_cv<typename remove_reference<First>::type>::type
                  , typename remove_cv<typename remove_reference<Second>::type>::type
                >
            {};

            template<typename Second>
            segmented_view<right_view, RightCons> operator ()(right_view, Second &second) const
            {
                return segmented_view<right_view, RightCons>(this->right_cons);
            }

            template<typename Second>
            segmented_view<left_view, LeftCons> operator ()(left_view, Second &second) const
            {
                return segmented_view<left_view, LeftCons>(this->left_cons);
            }

            template<typename Second>
            Second &operator ()(full_view, Second &second) const
            {
                return second;
            }

        private:
            RightCons const &right_cons;
            LeftCons const &left_cons;
        };

    } // namespace detail

    namespace extension
    {
        ////////////////////////////////////////////////////////////////////////////
        template<typename Tag>
        struct is_segmented_impl<detail::segmented_view_tag<Tag> >
        {
            template<typename Sequence>
            struct apply
              : mpl::true_
            {};
        };

        ////////////////////////////////////////////////////////////////////////////
        template<>
        struct segments_impl<detail::segmented_view_tag<detail::right_view> >
        {
            template<
                typename Sequence
              , typename Cdr = typename Sequence::cons_type::cdr_type
            >
            struct apply
            {
                typedef typename Sequence::cons_type::car_type segmented_range;
                typedef typename result_of::size<segmented_range>::type size;
                typedef typename mpl::prior<size>::type size_minus_1;
                typedef detail::segments_transform<Cdr> tfx;
                typedef joint_view<
                    single_view<detail::right_view> const
                  , multiple_view<size_minus_1, detail::full_view> const
                > mask;
                typedef transform_view<mask const, segmented_range const, tfx> type;

                static type call(Sequence &seq)
                {
                    return type(
                        mask(
                            make_single_view(detail::right_view())
                          , make_multiple_view<size_minus_1>(detail::full_view())
                        )
                      , seq.cons.car
                      , tfx(seq.cons.cdr)
                    );
                }
            };

            template<typename Sequence>
            struct apply<Sequence, nil>
            {
                typedef typename Sequence::cons_type::car_type segmented_range;
                typedef typename segmented_range::iterator_type begin;
                typedef typename segmented_range::sequence_non_ref_type sequence_type;
                typedef typename result_of::end<sequence_type>::type end;
                typedef iterator_range<begin, end> range;
                typedef single_view<range> type;

                static type call(Sequence &seq)
                {
                    return type(range(seq.cons.car.where_, fusion::end(seq.cons.car.sequence)));
                }
            };
        };

        ////////////////////////////////////////////////////////////////////////////
        template<>
        struct segments_impl<detail::segmented_view_tag<detail::left_view> >
        {
            template<
                typename Sequence
              , typename Cdr = typename Sequence::cons_type::cdr_type
            >
            struct apply
            {
                typedef typename Sequence::cons_type::car_type right_segmented_range;
                typedef typename right_segmented_range::sequence_type sequence_type;
                typedef typename right_segmented_range::iterator_type iterator_type;

                typedef iterator_range<
                    typename result_of::begin<sequence_type>::type
                  , typename result_of::next<iterator_type>::type
                > segmented_range;

                typedef detail::segments_transform<Cdr> tfx;
                typedef typename result_of::size<segmented_range>::type size;
                typedef typename mpl::prior<size>::type size_minus_1;
                typedef joint_view<
                    multiple_view<size_minus_1, detail::full_view> const
                  , single_view<detail::left_view> const
                > mask;
                typedef transform_view<mask const, segmented_range const, tfx> type;

                static type call(Sequence &seq)
                {
                    return type(
                        mask(
                            make_multiple_view<size_minus_1>(detail::full_view())
                          , make_single_view(detail::left_view())
                        )
                      , segmented_range(fusion::begin(seq.cons.car.sequence), fusion::next(seq.cons.car.where_))
                      , tfx(seq.cons.cdr)
                    );
                }
            };

            template<typename Sequence>
            struct apply<Sequence, nil>
            {
                typedef typename Sequence::cons_type::car_type segmented_range;
                typedef typename segmented_range::sequence_non_ref_type sequence_type;
                typedef typename result_of::begin<sequence_type>::type begin;
                typedef typename segmented_range::iterator_type end;
                typedef iterator_range<begin, end> range;
                typedef single_view<range> type;

                static type call(Sequence &seq)
                {
                    return type(range(fusion::begin(seq.cons.car.sequence), seq.cons.car.where_));
                }
            };
        };

        ////////////////////////////////////////////////////////////////////////////
        template<>
        struct segments_impl<detail::segmented_view_tag<detail::center_view> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef typename Sequence::right_cons_type right_cons_type;
                typedef typename Sequence::left_cons_type left_cons_type;
                typedef typename right_cons_type::car_type right_segmented_range;
                typedef typename left_cons_type::car_type left_segmented_range;

                typedef iterator_range<
                    typename result_of::begin<left_segmented_range>::type
                  , typename result_of::next<typename result_of::begin<right_segmented_range>::type>::type
                > segmented_range;

                typedef typename mpl::minus<
                    typename result_of::size<segmented_range>::type
                  , mpl::int_<2>
                >::type size_minus_2;

                BOOST_MPL_ASSERT_RELATION(0, <=, size_minus_2::value);

                typedef detail::segments_transform<
                    typename left_cons_type::cdr_type
                  , typename right_cons_type::cdr_type
                > tfx;

                typedef joint_view<
                    multiple_view<size_minus_2, detail::full_view> const
                  , single_view<detail::left_view> const
                > left_mask;

                typedef joint_view<
                    single_view<detail::right_view> const
                  , left_mask const
                > mask;

                typedef transform_view<mask const, segmented_range const, tfx> type;

                static type call(Sequence &seq)
                {
                    left_mask lmask(
                        make_multiple_view<size_minus_2>(detail::full_view())
                      , make_single_view(detail::left_view())
                    );
                    return type(
                        mask(make_single_view(detail::right_view()), lmask)
                      , segmented_range(fusion::begin(seq.left_cons.car), fusion::next(fusion::begin(seq.right_cons.car)))
                      , tfx(seq.left_cons.cdr, seq.right_cons.cdr)
                    );
                }
            };
        };
    }

    // specialize iterator_range for use with segmented iterators, so that
    // it presents a segmented view of the range.
    template<typename First, typename Last>
    struct iterator_range;

    template<typename First, typename Last>
    struct iterator_range<segmented_iterator<First>, segmented_iterator<Last> >
      : sequence_base<iterator_range<segmented_iterator<First>, segmented_iterator<Last> > >
    {
        typedef typename convert_iterator<segmented_iterator<First> >::type begin_type;
        typedef typename convert_iterator<segmented_iterator<Last> >::type end_type;
        typedef typename detail::reverse_cons<First>::type begin_cons_type;
        typedef typename detail::reverse_cons<Last>::type end_cons_type;
        typedef iterator_range_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef typename traits::category_of<begin_type>::type category;
        typedef typename result_of::distance<begin_type, end_type>::type size;
        typedef mpl::true_ is_view;

        iterator_range(segmented_iterator<First> const& first_, segmented_iterator<Last> const& last_)
          : first(convert_iterator<segmented_iterator<First> >::call(first_))
          , last(convert_iterator<segmented_iterator<Last> >::call(last_))
          , first_cons(detail::reverse_cons<First>::call(first_.cons()))
          , last_cons(detail::reverse_cons<Last>::call(last_.cons()))
        {}

        begin_type first;
        end_type last;

        begin_cons_type first_cons;
        end_cons_type last_cons;
    };

    namespace detail
    {

        template<typename Cons1, typename Cons2>
        struct same_segment
          : mpl::false_
        {};

        template<typename Car1, typename Cdr1, typename Car2, typename Cdr2>
        struct same_segment<cons<Car1, Cdr1>, cons<Car2, Cdr2> >
          : mpl::and_<
                traits::is_segmented<Car1>
              , is_same<Car1, Car2>
            >
        {};

        ////////////////////////////////////////////////////////////////////////////
        template<typename Cons1, typename Cons2>
        struct segments_gen;

        ////////////////////////////////////////////////////////////////////////////
        template<typename Cons1, typename Cons2, bool SameSegment>
        struct segments_gen2
        {
            typedef segments_gen<typename Cons1::cdr_type, typename Cons2::cdr_type> gen;
            typedef typename gen::type type;

            static type call(Cons1 const &cons1, Cons2 const &cons2)
            {
                return gen::call(cons1.cdr, cons2.cdr);
            }
        };

        template<typename Cons1, typename Cons2>
        struct segments_gen2<Cons1, Cons2, false>
        {
            typedef segmented_view<center_view, Cons1, Cons2> view;
            typedef typename result_of::segments<view>::type type;

            static type call(Cons1 const &cons1, Cons2 const &cons2)
            {
                view v(cons1, cons2);
                return fusion::segments(v);
            }
        };

        template<typename Car1, typename Car2>
        struct segments_gen2<cons<Car1>, cons<Car2>, false>
        {
            typedef iterator_range<
                typename Car1::iterator_type
              , typename Car2::iterator_type
            > range;

            typedef single_view<range> type;

            static type call(cons<Car1> const &cons1, cons<Car2> const &cons2)
            {
                return type(range(cons1.car.where_, cons2.car.where_));
            }
        };

        ////////////////////////////////////////////////////////////////////////////
        template<typename Cons1, typename Cons2>
        struct segments_gen
          : segments_gen2<Cons1, Cons2, same_segment<Cons1, Cons2>::value>
        {};

        template<typename Car, typename Cdr>
        struct segments_gen<cons<Car, Cdr>, nil>
        {
            typedef segmented_view<right_view, cons<Car, Cdr> > view;
            typedef typename result_of::segments<view>::type type;

            static type call(cons<Car, Cdr> const &cons, nil const &)
            {
                view v(cons);
                return fusion::segments(v);
            }
        };

        template<>
        struct segments_gen<nil, nil>
        {
            typedef nil type;

            static type call(nil const &, nil const &)
            {
                return nil();
            }
        };
    } // namespace detail

    namespace extension
    {
        template<typename Tag>
        struct is_segmented_impl;

        // An iterator_range of segmented_iterators is segmented
        template<>
        struct is_segmented_impl<iterator_range_tag>
        {
            template<typename Iterator>
            struct is_segmented_iterator : mpl::false_ {};

            template<typename Cons>
            struct is_segmented_iterator<segmented_iterator<Cons> > : mpl::true_ {};

            template<typename Sequence>
            struct apply
              : mpl::and_<
                    is_segmented_iterator<typename Sequence::begin_type>
                  , is_segmented_iterator<typename Sequence::end_type>
                >
            {};
        };

        template<typename Sequence>
        struct segments_impl;

        template<>
        struct segments_impl<iterator_range_tag>
        {
            template<typename Sequence>
            struct apply
            {
                typedef typename Sequence::begin_cons_type begin_cons;
                typedef typename Sequence::end_cons_type end_cons;

                typedef detail::segments_gen<begin_cons, end_cons> gen;
                typedef typename gen::type type;

                static type call(Sequence &sequence)
                {
                    return gen::call(sequence.first_cons, sequence.last_cons);
                }
            };
        };
    }

}}

#endif
