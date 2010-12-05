// Boost.Range library
//
//  Copyright Neil Groves 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ADAPTOR_STRIDED_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_STRIDED_HPP_INCLUDED

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <iterator>

namespace boost
{
    namespace range_detail
    {

        template<typename BaseIterator>
        class strided_iterator
            : public iterator_adaptor<
                        strided_iterator<BaseIterator>,
                        BaseIterator>
        {
            friend class iterator_core_access;

            typedef iterator_adaptor<strided_iterator<BaseIterator>, BaseIterator> super_t;
        
        public:
            typedef BOOST_DEDUCED_TYPENAME std::iterator_traits<BaseIterator>::difference_type          difference_type;
                
            strided_iterator() : m_stride() { }
        
            strided_iterator(const strided_iterator& other)
                : super_t(other), m_stride(other.m_stride) { }
        
            explicit strided_iterator(BaseIterator base_it, difference_type stride)
                : super_t(base_it), m_stride(stride) { }
        
            strided_iterator&
            operator=(const strided_iterator& other)
            {
                super_t::operator=(other);
            
                // Is the interoperation of the stride safe?
                m_stride = other.m_stride;
                return *this;
            }
        
            void increment() { std::advance(this->base_reference(), m_stride); }
        
            void decrement() { std::advance(this->base_reference(), -m_stride); }
        
            void advance(difference_type n) { std::advance(this->base_reference(), n * m_stride); }
        
            difference_type
            distance_to(const strided_iterator& other) const
            {
                return std::distance(this->base_reference(), other.base_reference()) / m_stride;
            }

            // Using the compiler generated copy constructor and
            // and assignment operator
        
        private:
            difference_type m_stride;
        };
    
        template<class BaseIterator> inline
        strided_iterator<BaseIterator>
        make_strided_iterator(
            const BaseIterator& first,
            BOOST_DEDUCED_TYPENAME std::iterator_traits<BaseIterator>::difference_type stride)
        {
            return strided_iterator<BaseIterator>(first, stride);
        }

        template< class Rng >
        class strided_range
            : public iterator_range<range_detail::strided_iterator<BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type> >
        {
            typedef range_detail::strided_iterator<BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type> iter_type;
            typedef iterator_range<iter_type> super_t;
        public:
            template< typename Difference >
            strided_range(Difference stride, Rng& rng)
                : super_t(make_strided_iterator(boost::begin(rng), stride),
                    make_strided_iterator(boost::end(rng), stride))
            {
            }
        };

        template<class Difference>
        class strided_holder : public holder<Difference>
        {
        public:
            strided_holder(Difference value) : holder<Difference>(value) {}
        };

        template<class Rng, class Difference>
        inline strided_range<Rng>
        operator|(Rng& rng, const strided_holder<Difference>& stride)
        {
            return strided_range<Rng>(stride.val, rng);
        }

        template<class Rng, class Difference>
        inline strided_range<const Rng>
        operator|(const Rng& rng, const strided_holder<Difference>& stride)
        {
            return strided_range<const Rng>(stride.val, rng);
        }

    } // namespace range_detail
    
    using range_detail::strided_range;

    namespace adaptors
    {
    
        namespace
        {
            const range_detail::forwarder<range_detail::strided_holder>
                strided = range_detail::forwarder<range_detail::strided_holder>();
        }
        
        template<class Range, class Difference>
        inline strided_range<Range>
        stride(Range& rng, Difference step)
        {
            return strided_range<Range>(step, rng);
        }
        
        template<class Range, class Difference>
        inline strided_range<const Range>
        stride(const Range& rng, Difference step)
        {
            return strided_range<const Range>(step, rng);
        }
        
    } // namespace 'adaptors'
} // namespace 'boost'

#endif
