/*=============================================================================
    Copyright (c) 2005 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MAP_07212005_1106)
#define FUSION_MAP_07212005_1106

#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/container/map/map_fwd.hpp>
#include <boost/fusion/container/map/detail/begin_impl.hpp>
#include <boost/fusion/container/map/detail/end_impl.hpp>
#include <boost/fusion/container/map/detail/value_of_impl.hpp>
#include <boost/fusion/container/map/detail/deref_data_impl.hpp>
#include <boost/fusion/container/map/detail/deref_impl.hpp>
#include <boost/fusion/container/map/detail/key_of_impl.hpp>
#include <boost/fusion/container/map/detail/value_of_data_impl.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace fusion
{
    struct void_;
    struct fusion_sequence_tag;

    template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_MAP_SIZE, typename T)>
    struct map : sequence_base<map<BOOST_PP_ENUM_PARAMS(FUSION_MAX_MAP_SIZE, T)> >
    {
        struct category : forward_traversal_tag, associative_tag {};
        
        typedef map_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef mpl::false_ is_view;

        typedef vector<
            BOOST_PP_ENUM_PARAMS(FUSION_MAX_MAP_SIZE, T)> 
        storage_type;

        typedef typename storage_type::size size;

        map()
            : data() {}
        
        template <typename Sequence>
        map(Sequence const& rhs)
            : data(rhs) {}

        #include <boost/fusion/container/map/detail/map_forward_ctor.hpp>

        template <typename T>
        map&
        operator=(T const& rhs)
        {
            data = rhs;
            return *this;
        }

        storage_type& get_data() { return data; }
        storage_type const& get_data() const { return data; }

    private:
        
        storage_type data;
    };
}}

#endif
