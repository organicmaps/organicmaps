// Boost.Geometry (aka GGL, Generic Geometry Library)

// Boost.SpatialIndex - rtree implementation
//
// Copyright 2008 Federico J. Fernandez.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_INDEX_RTREE_RTREE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_INDEX_RTREE_RTREE_HPP

#include <cstddef>
#include <iostream> // TODO: Remove if print() is removed
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/concept_check.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/geometry/algorithms/area.hpp>

#include <boost/geometry/extensions/index/rtree/rtree_node.hpp>
#include <boost/geometry/extensions/index/rtree/rtree_leaf.hpp>

namespace boost { namespace geometry { namespace index
{

template <typename Box, typename Value >
class rtree
{
public:

    typedef boost::shared_ptr<rtree_node<Box, Value> > node_pointer;
    typedef boost::shared_ptr<rtree_leaf<Box, Value> > leaf_pointer;

    /**
     * \brief Creates a rtree with 'maximum' elements per node and 'minimum'.
     */
    rtree(unsigned int const& maximum, unsigned int const& minimum)
        : m_count(0)
        , m_min_elems_per_node(minimum)
        , m_max_elems_per_node(maximum)
        , m_root(new rtree_node<Box, Value>(node_pointer(), 1))
    {
    }

    /**
     * \brief Creates a rtree with maximum elements per node
     *        and minimum (box is ignored).
     */
    rtree(Box const& box, unsigned int const& maximum, unsigned int const& minimum)
        : m_count(0)
        , m_min_elems_per_node(minimum)
        , m_max_elems_per_node(maximum)
        , m_root(new rtree_node<Box, Value>(node_pointer(), 1))
    {
        boost::ignore_unused_variable_warning(box);
    }

    /**
     * \brief destructor (virtual because we have virtual functions)
     */
    virtual ~rtree() {}


    /**
     * \brief Remove elements inside the 'box'
     */
    inline void remove(Box const& box)
    {
        try
        {
            node_pointer leaf(choose_exact_leaf(box));
            typename rtree_leaf<Box, Value>::leaf_map q_leaves;

            leaf->remove(box);

            if (leaf->elements() < m_min_elems_per_node && elements() > m_min_elems_per_node)
            {
                q_leaves = leaf->get_leaves();

                // we remove the leaf_node in the parent node because now it's empty
                leaf->get_parent()->remove(leaf->get_parent()->get_box(leaf));
            }

            typename rtree_node<Box, Value>::node_map q_nodes;
            condense_tree(leaf, q_nodes);

            std::vector<std::pair<Box, Value> > s;
            for (typename rtree_node<Box, Value>::node_map::const_iterator it = q_nodes.begin();
                 it != q_nodes.end(); ++it)
            {
                typename rtree_leaf<Box, Value>::leaf_map leaves = it->second->get_leaves();

                // reinserting leaves from nodes
                for (typename rtree_leaf<Box, Value>::leaf_map::const_iterator itl = leaves.begin();
                     itl != leaves.end(); ++itl)
                {
                    s.push_back(*itl);
                }
            }

            for (typename std::vector<std::pair<Box, Value> >::const_iterator it = s.begin(); it != s.end(); ++it)
            {
                m_count--;
                insert(it->first, it->second);
            }

            // if the root has only one child and the child is not a leaf,
            // make it the root
            if (m_root->elements() == 1)
            {
                if (!m_root->first_element()->is_leaf())
                {
                    m_root = m_root->first_element();
                }
            }
            // reinserting leaves
            for (typename rtree_leaf<Box, Value>::leaf_map::const_iterator it = q_leaves.begin();
                 it != q_leaves.end(); ++it)
            {
                m_count--;
                insert(it->first, it->second);
            }

            m_count--;
        }
        catch(std::logic_error & e)
        {
            // TODO: mloskot - replace with Boost.Geometry exception

            // not found
            std::cerr << e.what() << std::endl;
            return;
        }
    }

    /**
     * \brief Remove element inside the box with value
     */
    void remove(Box const& box, Value const& value)
    {
        try
        {
            node_pointer leaf;

            // find possible leaves
            typedef typename std::vector<node_pointer > node_type;
            node_type nodes;
            m_root->find_leaves(box, nodes);

            // refine the result
            for (typename node_type::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
            {
                leaf = *it;
                try
                {
                    leaf->remove(value);
                    break;
                } catch (...)
                {
                    leaf = node_pointer();
                }
            }

            if (!leaf)
                return;

            typename rtree_leaf < Box, Value >::leaf_map q_leaves;

            if (leaf->elements() < m_min_elems_per_node && elements() > m_min_elems_per_node)
            {
                q_leaves = leaf->get_leaves();

                // we remove the leaf_node in the parent node because now it's empty
                leaf->get_parent()->remove(leaf->get_parent()->get_box(leaf));
            }

            typename rtree_node<Box, Value>::node_map q_nodes;
            condense_tree(leaf, q_nodes);

            std::vector<std::pair<Box, Value> > s;
            for (typename rtree_node<Box, Value>::node_map::const_iterator it = q_nodes.begin();
                 it != q_nodes.end(); ++it)
            {
                typename rtree_leaf<Box, Value>::leaf_map leaves = it->second->get_leaves();

                // reinserting leaves from nodes
                for (typename rtree_leaf<Box, Value>::leaf_map::const_iterator itl = leaves.begin();
                     itl != leaves.end(); ++itl)
                {
                    s.push_back(*itl);
                }
            }

            for (typename std::vector<std::pair<Box, Value> >::const_iterator it = s.begin(); it != s.end(); ++it)
            {
                m_count--;
                insert(it->first, it->second);
            }

            // if the root has only one child and the child is not a leaf,
            // make it the root
            if (m_root->elements() == 1)
            {
                if (!m_root->first_element()->is_leaf())
                {
                    m_root = m_root->first_element();
                }
            }

            // reinserting leaves
            for (typename rtree_leaf<Box, Value>::leaf_map::const_iterator it = q_leaves.begin();
                 it != q_leaves.end(); ++it)
            {
                m_count--;
                insert(it->first, it->second);
            }

            m_count--;

        }
        catch(std::logic_error & e)
        {
            // TODO: mloskot - ggl exception

            // not found
            std::cerr << e.what() << std::endl;
            return;
        }
    }

    /**
     * \brief Returns the number of elements.
     */
    inline unsigned int elements() const
    {
        return m_count;
    }


    /**
     * \brief Inserts an element with 'box' as key with value.
     */
    inline void insert(Box const& box, Value const& value)
    {
        m_count++;

        node_pointer leaf(choose_corresponding_leaf(box));

        // check if the selected leaf is full to do the split if necessary
        if (leaf->elements() >= m_max_elems_per_node)
        {
            leaf->insert(box, value);

            // split!
            node_pointer n1(new rtree_leaf<Box, Value>(leaf->get_parent()));
            node_pointer n2(new rtree_leaf<Box, Value>(leaf->get_parent()));

            split_node(leaf, n1, n2);
            adjust_tree(leaf, n1, n2);
        }
        else
        {
            leaf->insert(box, value);
            adjust_tree(leaf);
        }
    }


    /**
     * \brief Returns all the values inside 'box'
     */
    inline std::deque<Value> find(Box const& box) const
    {
        std::deque<Value> result;
        m_root->find(box, result, false);
        return result;
    }

    /**
     * \brief Print Rtree (mainly for debug)
     */
    inline void print()
    {
        std::cerr << "===================================" << std::endl;
        std::cerr << " Min/Max: " << m_min_elems_per_node << " / " << m_max_elems_per_node << std::endl;
        std::cerr << "Leaves: " << m_root->get_leaves().size() << std::endl;
        m_root->print();
        std::cerr << "===================================" << std::endl;
    }

private:

    /// number of elements
    unsigned int m_count;

    /// minimum number of elements per node
    unsigned int m_min_elems_per_node;

    /// maximum number of elements per node
    unsigned int m_max_elems_per_node;

    /// tree root
    node_pointer m_root;

    /**
     * \brief Reorganize the tree after a removal. It tries to
     *        join nodes with less elements than m.
     */
    void condense_tree(node_pointer const& leaf,
        typename rtree_node<Box, Value>::node_map& q_nodes)
    {
        if (leaf.get() == m_root.get())
        {
            // if it's the root we are done
            return;
        }

        node_pointer parent = leaf->get_parent();
        parent->adjust_box(leaf);

        if (parent->elements() < m_min_elems_per_node)
        {
            if (parent.get() == m_root.get())
            {
                // if the parent is underfull and it's the root we just exit
                return;
            }

            // get the nodes that we should reinsert
            typename rtree_node<Box, Value>::node_map this_nodes = parent->get_nodes();
            for(typename rtree_node<Box, Value>::node_map::const_iterator it = this_nodes.begin();
                it != this_nodes.end(); ++it)
            {
                q_nodes.push_back(*it);
            }

            // we remove the node in the parent node because now it should be
            // re inserted
            parent->get_parent()->remove(parent->get_parent()->get_box(parent));
        }

        condense_tree(parent, q_nodes);
    }

    /**
     * \brief After an insertion splits nodes with more than 'maximum' elements.
     */
    inline void adjust_tree(node_pointer& node)
    {
        if (node.get() == m_root.get())
        {
            // we finished the adjust
            return;
        }

        // as there are no splits just adjust the box of the parent and go on
        node_pointer parent = node->get_parent();
        parent->adjust_box(node);
        adjust_tree(parent);
    }

    /**
     * \brief After an insertion splits nodes with more than maximum elements
     *        (recursive step with subtrees 'n1' and 'n2' to be joined).
     */
    void adjust_tree(node_pointer& leaf, node_pointer& n1, node_pointer& n2)
    {
        // check if we are in the root and do the split
        if (leaf.get() == m_root.get())
        {
            node_pointer new_root(new rtree_node<Box,Value>(node_pointer (), leaf->get_level() + 1));
            new_root->add_node(n1->compute_box(), n1);
            new_root->add_node(n2->compute_box(), n2);

            n1->set_parent(new_root);
            n2->set_parent(new_root);

            n1->update_parent(n1);
            n2->update_parent(n2);

            m_root = new_root;
            return;
        }

        node_pointer parent = leaf->get_parent();

        parent->replace_node(leaf, n1);
        parent->add_node(n2->compute_box(), n2);

        // if parent is full, split and readjust
        if (parent->elements() > m_max_elems_per_node)
        {
            node_pointer p1(new rtree_node<Box, Value>(parent->get_parent(), parent->get_level()));
            node_pointer p2(new rtree_node<Box, Value>(parent->get_parent(), parent->get_level()));

            split_node(parent, p1, p2);
            adjust_tree(parent, p1, p2);
        }
        else
        {
            adjust_tree(parent);
        }
    }

    /**
     * \brief Splits 'n' in 'n1' and 'n2'
     */
    void split_node(node_pointer const& n, node_pointer& n1, node_pointer& n2) const
    {
        unsigned int seed1 = 0;
        unsigned int seed2 = 0;
        std::vector<Box> boxes = n->get_boxes();

        n1->set_parent(n->get_parent());
        n2->set_parent(n->get_parent());

        linear_pick_seeds(n, seed1, seed2);

        if (n->is_leaf())
        {
            n1->add_value(boxes[seed1], n->get_value(seed1));
            n2->add_value(boxes[seed2], n->get_value(seed2));
        }
        else
        {
            n1->add_node(boxes[seed1], n->get_node(seed1));
            n2->add_node(boxes[seed2], n->get_node(seed2));
        }

        unsigned int index = 0;

        if (n->is_leaf())
        {
            // TODO: mloskot - add BOOST_GEOMETRY_ASSERT(node.size() >= 2); or similar

            typename rtree_leaf<Box, Value>::leaf_map nodes = n->get_leaves();
            unsigned int remaining = nodes.size() - 2;

            for (typename rtree_leaf<Box, Value>::leaf_map::const_iterator it = nodes.begin();
                 it != nodes.end(); ++it, index++)
            {
                if (index != seed1 && index != seed2)
                {
                    if (n1->elements() + remaining == m_min_elems_per_node)
                    {
                        n1->add_value(it->first, it->second);
                        continue;
                    }
                    if (n2->elements() + remaining == m_min_elems_per_node)
                    {
                        n2->add_value(it->first, it->second);
                        continue;
                    }

                    remaining--;

                    /// current boxes of each group
                    Box b1, b2;

                    /// enlarged boxes of each group
                    Box eb1, eb2;
                    b1 = n1->compute_box();
                    b2 = n2->compute_box();

                    /// areas
                    typedef typename coordinate_type<Box>::type coordinate_type;
                    coordinate_type b1_area, b2_area;
                    coordinate_type eb1_area, eb2_area;
                    b1_area = geometry::area(b1);
                    b2_area = geometry::area(b2);
                    eb1_area = compute_union_area(b1, it->first);
                    eb2_area = compute_union_area(b2, it->first);

                    if (eb1_area - b1_area > eb2_area - b2_area)
                    {
                        n2->add_value(it->first, it->second);
                    }
                    if (eb1_area - b1_area < eb2_area - b2_area)
                    {
                        n1->add_value(it->first, it->second);
                    }
                    if (eb1_area - b1_area == eb2_area - b2_area)
                    {
                        if (b1_area < b2_area)
                        {
                            n1->add_value(it->first, it->second);
                        }
                        if (b1_area > b2_area)
                        {
                            n2->add_value(it->first, it->second);
                        }
                        if (b1_area == b2_area)
                        {
                            if (n1->elements() > n2->elements())
                            {
                                n2->add_value(it->first, it->second);
                            }
                            else
                            {
                                n1->add_value(it->first, it->second);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // TODO: mloskot - add BOOST_GEOMETRY_ASSERT(node.size() >= 2); or similar

            typename rtree_node<Box, Value>::node_map nodes = n->get_nodes();
            unsigned int remaining = nodes.size() - 2;
            for(typename rtree_node<Box, Value>::node_map::const_iterator it = nodes.begin();
                it != nodes.end(); ++it, index++)
            {

                if (index != seed1 && index != seed2)
                {

                    if (n1->elements() + remaining == m_min_elems_per_node)
                    {
                        n1->add_node(it->first, it->second);
                        continue;
                    }
                    if (n2->elements() + remaining == m_min_elems_per_node)
                    {
                        n2->add_node(it->first, it->second);
                        continue;
                    }

                    remaining--;

                    /// current boxes of each group
                    Box b1, b2;

                    /// enlarged boxes of each group
                    Box eb1, eb2;
                    b1 = n1->compute_box();
                    b2 = n2->compute_box();

                    /// areas
                    typedef typename coordinate_type<Box>::type coordinate_type;
                    coordinate_type b1_area, b2_area;
                    coordinate_type eb1_area, eb2_area;
                    b1_area = geometry::area(b1);
                    b2_area = geometry::area(b2);

                    eb1_area = compute_union_area(b1, it->first);
                    eb2_area = compute_union_area(b2, it->first);

                    if (eb1_area - b1_area > eb2_area - b2_area)
                    {
                        n2->add_node(it->first, it->second);
                    }
                    if (eb1_area - b1_area < eb2_area - b2_area)
                    {
                        n1->add_node(it->first, it->second);
                    }
                    if (eb1_area - b1_area == eb2_area - b2_area)
                    {
                        if (b1_area < b2_area)
                        {
                            n1->add_node(it->first, it->second);
                        }
                        if (b1_area > b2_area)
                        {
                            n2->add_node(it->first, it->second);
                        }
                        if (b1_area == b2_area)
                        {
                            if (n1->elements() > n2->elements())
                            {
                                n2->add_node(it->first, it->second);
                            }
                            else
                            {
                                n1->add_node(it->first, it->second);
                            }
                        }
                    }

                }
            }
        }
    }

    /**
     * \brief Choose initial values for the split algorithm (linear version)
     */
    void linear_pick_seeds(node_pointer const& n, unsigned int &seed1, unsigned int &seed2) const
    {
        // get boxes from the node
        std::vector<Box>boxes = n->get_boxes();
        if (boxes.size() == 0)
        {
            // TODO: mloskot - throw ggl exception
            throw std::logic_error("Empty Node trying to Pick Seeds");
        }

        // only two dim for now
        // unsigned int dimensions =
        //   geometry::point_traits<Point>::coordinate_count;

        // find the first two elements
        typedef typename coordinate_type<Box>::type coordinate_type;
        coordinate_type separation_x, separation_y;
        unsigned int first_x, second_x;
        unsigned int first_y, second_y;
        find_normalized_separations<0u>(boxes, separation_x, first_x, second_x);
        find_normalized_separations<1u>(boxes, separation_y, first_y, second_y);

        if (separation_x > separation_y)
        {
            seed1 = first_x;
            seed2 = second_x;
        }
        else
        {
            seed1 = first_y;
            seed2 = second_y;
        }
    }

    /**
     * \brief Find distances between possible initial values for the
     *        pick_seeds algorithm.
     */
    template <std::size_t D, typename T>
    void find_normalized_separations(std::vector<Box> const& boxes, T& separation,
        unsigned int& first, unsigned int& second) const
    {
        if (boxes.size() < 2)
        {
            throw std::logic_error("At least two boxes needed to split");
        }

        // find the lowest high
        typename std::vector<Box>::const_iterator it = boxes.begin();
        typedef typename coordinate_type<Box>::type coordinate_type;
        coordinate_type lowest_high = geometry::get<max_corner, D>(*it);
        unsigned int lowest_high_index = 0;
        unsigned int index = 1;
        ++it;
        for(; it != boxes.end(); ++it)
        {
            if (geometry::get<max_corner, D>(*it) < lowest_high)
            {
                lowest_high = geometry::get<max_corner, D>(*it);
                lowest_high_index = index;
            }
            index++;
        }

        // find the highest low
        coordinate_type highest_low = 0;
        unsigned int highest_low_index = 0;
        if (lowest_high_index == 0)
        {
            highest_low = geometry::get<min_corner, D>(boxes[1]);
            highest_low_index = 1;
        }
        else
        {
            highest_low = geometry::get<min_corner, D>(boxes[0]);
            highest_low_index = 0;
        }

        index = 0;
        for (typename std::vector<Box>::const_iterator it = boxes.begin();
             it != boxes.end(); ++it, index++)
        {
            if (geometry::get<min_corner, D>(*it) >= highest_low && index != lowest_high_index)
            {
                highest_low = geometry::get<min_corner, D>(*it);
                highest_low_index = index;
            }
        }

        // find the lowest low
        it = boxes.begin();
        coordinate_type lowest_low = geometry::get<min_corner, D>(*it);
        ++it;
        for(; it != boxes.end(); ++it)
        {
            if (geometry::get<min_corner, D>(*it) < lowest_low)
            {
                lowest_low = geometry::get<min_corner, D>(*it);
            }
        }

        // find the highest high
        it = boxes.begin();
        coordinate_type highest_high = geometry::get<max_corner, D>(*it);
        ++it;
        for(; it != boxes.end(); ++it)
        {
            if (geometry::get<max_corner, D>(*it) > highest_high)
            {
                highest_high = geometry::get<max_corner, D>(*it);
            }
        }

        coordinate_type const width = highest_high - lowest_low;

        separation = (highest_low - lowest_high) / width;
        first = highest_low_index;
        second = lowest_high_index;
    }

    /**
     * \brief Choose one of the possible leaves to make an insertion
     */
    inline node_pointer choose_corresponding_leaf(Box const& e)
    {
        node_pointer node = m_root;

        // if the tree is empty add an initial leaf
        if (m_root->elements() == 0)
        {
            leaf_pointer new_leaf(new rtree_leaf<Box, Value>(m_root));
            m_root->add_leaf_node(Box (), new_leaf);

            return new_leaf;
        }

        while (!node->is_leaf())
        {
            /// traverse node's map to see which node we should select
            node = node->choose_node(e);
        }
        return node;
    }

    /**
     * \brief Choose the exact leaf where an insertion should be done
     */
    node_pointer choose_exact_leaf(Box const&e) const
    {
        // find possible leaves
        typedef typename std::vector<node_pointer> node_type;
        node_type nodes;
        m_root->find_leaves(e, nodes);

        // refine the result
        for (typename node_type::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
        {
            typedef std::vector<std::pair<Box, Value> > leaves_type;
            leaves_type leaves = (*it)->get_leaves();

            for (typename leaves_type::const_iterator itl = leaves.begin();
                 itl != leaves.end(); ++itl)
            {

                if (itl->first.max_corner() == e.max_corner()
                    && itl->first.min_corner() == e.min_corner())
                {
                    return *it;
                }
            }
        }

        // TODO: mloskot - ggl exception
        throw std::logic_error("Leaf not found");
    }
};

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_EXTENSIONS_INDEX_RTREE_RTREE_HPP

