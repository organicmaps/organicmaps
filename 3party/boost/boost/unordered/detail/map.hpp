
// Copyright (C) 2005-2016 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/unordered/detail/implementation.hpp>
#include <boost/unordered/unordered_map_fwd.hpp>

namespace boost {
namespace unordered {
namespace detail {
template <typename A, typename K, typename M, typename H, typename P> struct map
{
    typedef boost::unordered::detail::map<A, K, M, H, P> types;

    typedef std::pair<K const, M> value_type;
    typedef H hasher;
    typedef P key_equal;
    typedef K const const_key_type;

    typedef typename ::boost::unordered::detail::rebind_wrap<A,
        value_type>::type value_allocator;
    typedef boost::unordered::detail::allocator_traits<value_allocator>
        value_allocator_traits;

    typedef boost::unordered::detail::pick_node<A, value_type> pick;
    typedef typename pick::node node;
    typedef typename pick::bucket bucket;
    typedef typename pick::link_pointer link_pointer;
    typedef typename pick::node_algo node_algo;

    typedef boost::unordered::detail::table_impl<types> table;
    typedef boost::unordered::detail::map_extractor<value_type> extractor;

    typedef typename boost::unordered::detail::pick_policy<K>::type policy;

    typedef boost::unordered::iterator_detail::iterator<node> iterator;
    typedef boost::unordered::iterator_detail::c_iterator<node> c_iterator;
    typedef boost::unordered::iterator_detail::l_iterator<node, policy>
        l_iterator;
    typedef boost::unordered::iterator_detail::cl_iterator<node, policy>
        cl_iterator;

    typedef boost::unordered::node_handle_map<node, K, M, A> node_type;
    typedef boost::unordered::insert_return_type_map<node, K, M, A>
        insert_return_type;
};

template <typename A, typename K, typename M, typename H, typename P>
struct multimap
{
    typedef boost::unordered::detail::multimap<A, K, M, H, P> types;

    typedef std::pair<K const, M> value_type;
    typedef H hasher;
    typedef P key_equal;
    typedef K const const_key_type;

    typedef typename ::boost::unordered::detail::rebind_wrap<A,
        value_type>::type value_allocator;
    typedef boost::unordered::detail::allocator_traits<value_allocator>
        value_allocator_traits;

#if BOOST_UNORDERED_INTEROPERABLE_NODES
    typedef boost::unordered::detail::pick_node<A, value_type> pick;
#else
    typedef boost::unordered::detail::pick_grouped_node<A, value_type> pick;
#endif
    typedef typename pick::node node;
    typedef typename pick::bucket bucket;
    typedef typename pick::link_pointer link_pointer;
    typedef typename pick::node_algo node_algo;

    typedef boost::unordered::detail::grouped_table_impl<types> table;
    typedef boost::unordered::detail::map_extractor<value_type> extractor;

    typedef typename boost::unordered::detail::pick_policy<K>::type policy;

    typedef boost::unordered::iterator_detail::iterator<node> iterator;
    typedef boost::unordered::iterator_detail::c_iterator<node> c_iterator;
    typedef boost::unordered::iterator_detail::l_iterator<node, policy>
        l_iterator;
    typedef boost::unordered::iterator_detail::cl_iterator<node, policy>
        cl_iterator;

    typedef boost::unordered::node_handle_map<node, K, M, A> node_type;
};

template <typename K, typename M, typename H, typename P, typename A>
class instantiate_map
{
    typedef boost::unordered_map<K, M, H, P, A> container;
    container x;
    typename container::node_type node_type;
    typename container::insert_return_type insert_return_type;
};

template <typename K, typename M, typename H, typename P, typename A>
class instantiate_multimap
{
    typedef boost::unordered_multimap<K, M, H, P, A> container;
    container x;
    typename container::node_type node_type;
};
}
}
}
