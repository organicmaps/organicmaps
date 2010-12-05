
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2009 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This contains the basic data structure, apart from the actual values. There's
// no construction or deconstruction here. So this only depends on the pointer
// type.

#ifndef BOOST_UNORDERED_DETAIL_NODE_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_NODE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/unordered/detail/fwd.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, <= 0X0582)
#define BOOST_UNORDERED_BORLAND_BOOL(x) (bool)(x)
#else
#define BOOST_UNORDERED_BORLAND_BOOL(x) x
#endif

namespace boost { namespace unordered_detail {

    ////////////////////////////////////////////////////////////////////////////
    // ungrouped node implementation
    
    template <class A>
    inline BOOST_DEDUCED_TYPENAME ungrouped_node_base<A>::node_ptr&
        ungrouped_node_base<A>::next_group(node_ptr ptr)
    {
        return ptr->next_;
    }

    template <class A>
    inline std::size_t ungrouped_node_base<A>::group_count(node_ptr)
    {
        return 1;
    }

    template <class A>
    inline void ungrouped_node_base<A>::add_to_bucket(node_ptr n, bucket& b)
    {
        n->next_ = b.next_;
        b.next_ = n;
    }

    template <class A>
    inline void ungrouped_node_base<A>::add_after_node(node_ptr n,
        node_ptr position)
    {
        n->next_ = position->next_;
        position->next_ = position;
    }
    
    template <class A>
    inline void ungrouped_node_base<A>::unlink_nodes(bucket& b,
        node_ptr begin, node_ptr end)
    {
        node_ptr* pos = &b.next_;
        while(*pos != begin) pos = &(*pos)->next_;
        *pos = end;
    }

    template <class A>
    inline void ungrouped_node_base<A>::unlink_nodes(bucket& b, node_ptr end)
    {
        b.next_ = end;
    }

    template <class A>
    inline void ungrouped_node_base<A>::unlink_node(bucket& b, node_ptr n)
    {
        unlink_nodes(b, n, n->next_);
    }

    ////////////////////////////////////////////////////////////////////////////
    // grouped node implementation
    
    // If ptr is the first element in a group, return pointer to next group.
    // Otherwise returns a pointer to ptr.
    template <class A>
    inline BOOST_DEDUCED_TYPENAME grouped_node_base<A>::node_ptr&
        grouped_node_base<A>::next_group(node_ptr ptr)
    {
        return get(ptr).group_prev_->next_;
    }

    template <class A>
    inline BOOST_DEDUCED_TYPENAME grouped_node_base<A>::node_ptr
        grouped_node_base<A>::first_in_group(node_ptr ptr)
    {
        while(next_group(ptr) == ptr)
            ptr = get(ptr).group_prev_;
        return ptr;
    }

    template <class A>
    inline std::size_t grouped_node_base<A>::group_count(node_ptr ptr)
    {
        node_ptr start = ptr;
        std::size_t size = 0;
        do {
            ++size;
            ptr = get(ptr).group_prev_;
        } while(ptr != start);
        return size;
    }

    template <class A>
    inline void grouped_node_base<A>::add_to_bucket(node_ptr n, bucket& b)
    {
        n->next_ = b.next_;
        get(n).group_prev_ = n;
        b.next_ = n;
    }

    template <class A>
    inline void grouped_node_base<A>::add_after_node(node_ptr n, node_ptr pos)
    {
        n->next_ = next_group(pos);
        get(n).group_prev_ = get(pos).group_prev_;
        next_group(pos) = n;
        get(pos).group_prev_ = n;
    }

    // Break a ciruclar list into two, with split as the beginning
    // of the second group (if split is at the beginning then don't
    // split).
    template <class A>
    inline BOOST_DEDUCED_TYPENAME grouped_node_base<A>::node_ptr
        grouped_node_base<A>::split_group(node_ptr split)
    {
        node_ptr first = first_in_group(split);
        if(first == split) return split;

        node_ptr last = get(first).group_prev_;
        get(first).group_prev_ = get(split).group_prev_;
        get(split).group_prev_ = last;

        return first;
    }

    template <class A>
    void grouped_node_base<A>::unlink_node(bucket& b, node_ptr n)
    {
        node_ptr next = n->next_;
        node_ptr* pos = &next_group(n);

        if(*pos != n) {
            // The node is at the beginning of a group.

            // Find the previous node pointer:
            pos = &b.next_;
            while(*pos != n) pos = &next_group(*pos);

            // Remove from group
            if(BOOST_UNORDERED_BORLAND_BOOL(next) &&
                get(next).group_prev_ == n)
            {
                get(next).group_prev_ = get(n).group_prev_;
            }
        }
        else if(BOOST_UNORDERED_BORLAND_BOOL(next) &&
            get(next).group_prev_ == n)
        {
            // The deleted node is not at the end of the group, so
            // change the link from the next node.
            get(next).group_prev_ = get(n).group_prev_;
        }
        else {
            // The deleted node is at the end of the group, so the
            // first node in the group is pointing to it.
            // Find that to change its pointer.
            node_ptr x = get(n).group_prev_;
            while(get(x).group_prev_ != n) {
                x = get(x).group_prev_;
            }
            get(x).group_prev_ = get(n).group_prev_;
        }
        *pos = next;
    }

    template <class A>
    void grouped_node_base<A>::unlink_nodes(bucket& b,
        node_ptr begin, node_ptr end)
    {
        node_ptr* pos = &next_group(begin);

        if(*pos != begin) {
            // The node is at the beginning of a group.

            // Find the previous node pointer:
            pos = &b.next_;
            while(*pos != begin) pos = &next_group(*pos);

            // Remove from group
            if(BOOST_UNORDERED_BORLAND_BOOL(end)) split_group(end);
        }
        else {
            node_ptr group1 = split_group(begin);
            if(BOOST_UNORDERED_BORLAND_BOOL(end)) {
                node_ptr group2 = split_group(end);

                if(begin == group2) {
                    node_ptr end1 = get(group1).group_prev_;
                    node_ptr end2 = get(group2).group_prev_;
                    get(group1).group_prev_ = end2;
                    get(group2).group_prev_ = end1;
                }
            }
        }
        *pos = end;
    }

    template <class A>
    void grouped_node_base<A>::unlink_nodes(bucket& b, node_ptr end)
    {
        split_group(end);
        b.next_ = end;
    }
}}

#endif
