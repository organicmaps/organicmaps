// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_UNIQUED_IMPL_HPP
#define BOOST_RANGE_ADAPTOR_UNIQUED_IMPL_HPP

#include <boost/range/adaptor/adjacent_filtered.hpp>

namespace boost
{

    namespace range_detail
    {
        struct unique_forwarder { };

        struct unique_not_equal_to
        {
            typedef bool result_type;

            template< class T >
            bool operator()( const T& l, const T& r ) const
            {
                return !(l == r);
            }
        };

        template<class ForwardRng>
        class unique_range : public adjacent_filter_range<unique_not_equal_to, ForwardRng, true>
        {
            typedef adjacent_filter_range<unique_not_equal_to, ForwardRng, true> base;
        public:
            explicit unique_range(ForwardRng& rng)
                : base(unique_not_equal_to(), rng)
            {
            }
        };

        template< class ForwardRng >
        inline unique_range<ForwardRng>
        operator|( ForwardRng& r,
                   unique_forwarder )
        {
            return unique_range<ForwardRng>(r);
        }

        template< class ForwardRng >
        inline unique_range<const ForwardRng>
        operator|( const ForwardRng& r,
                   unique_forwarder )
        {
            return unique_range<const ForwardRng>(r);
        }

    } // 'range_detail'

    using range_detail::unique_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::unique_forwarder uniqued =
                       range_detail::unique_forwarder();
        }

        template<class ForwardRange>
        inline unique_range<ForwardRange>
        unique(ForwardRange& rng)
        {
            return unique_range<ForwardRange>(rng);
        }

        template<class ForwardRange>
        inline unique_range<const ForwardRange>
        unique(const ForwardRange& rng)
        {
            return unique_range<const ForwardRange>(rng);
        }
    } // 'adaptors'

}

#endif
