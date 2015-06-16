// Boost.Geometry (aka GGL, Generic Geometry Library)

// Boost.SpatialIndex - rtree leaf implementation
//
// Copyright 2008 Federico J. Fernandez.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_INDEX_RTREE_RTREE_LEAF_HPP
#define BOOST_GEOMETRY_EXTENSIONS_INDEX_RTREE_RTREE_LEAF_HPP

#include <deque>
#include <iostream> // TODO: Remove if print() is removed
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/extensions/index/rtree/rtree_node.hpp>

namespace boost { namespace geometry { namespace index
{

template <typename Box, typename Value >
class rtree_leaf : public rtree_node<Box, Value>
{
public:

    /// container type for the leaves
    typedef boost::shared_ptr<rtree_node<Box, Value> > node_pointer;
    typedef std::vector<std::pair<Box, Value> > leaf_map;

    /**
     * \brief Creates an empty leaf
     */
    inline rtree_leaf()
    {
    }

    /**
     * \brief Creates a new leaf, with 'parent' as parent
     */
    inline rtree_leaf(node_pointer const& parent)
        : rtree_node<Box, Value> (parent, 0)
    {
    }

    /**
     * \brief Search for elements in 'box' in the Rtree. Add them to 'result'.
     *        If exact_match is true only return the elements having as
     *        key the 'box'. Otherwise return everything inside 'box'.
     */
    virtual void find(Box const& box, std::deque<Value>& result, bool const exact_match)
    {
        for (typename leaf_map::const_iterator it = m_nodes.begin();
             it != m_nodes.end(); ++it)
        {
            if (exact_match)
            {
                if (geometry::equals(it->first, box))
                {
                    result.push_back(it->second);
                }
            }
            else
            {
                if (is_overlapping(it->first, box))
                {
                    result.push_back(it->second);
                }
            }
        }
    }

    /**
     * \brief Compute bounding box for this leaf
     */
    virtual Box compute_box() const
    {
        if (m_nodes.empty())
        {
            return Box ();
        }

        Box r;
        geometry::assign_inverse(r);
        for(typename leaf_map::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            geometry::expand(r, it->first);
        }
        return r;
    }

    /**
     * \brief True if we are a leaf
     */
    virtual bool is_leaf() const
    {
        return true;
    }

    /**
     * \brief Number of elements in the tree
     */
    virtual unsigned int elements() const
    {
        return m_nodes.size();
    }

    /**
     * \brief Insert a new element, with key 'box' and value 'v'
     */
    virtual void insert(Box const& box, Value const& v)
    {
        m_nodes.push_back(std::make_pair(box, v));
    }

    /**
     * \brief Proyect leaves of this node.
     */
    virtual std::vector< std::pair<Box, Value> > get_leaves() const
    {
        return m_nodes;
    }

    /**
     * \brief Add a new child (node) to this node
     */
    virtual void add_node(Box const&, node_pointer const&)
    {
        // TODO: mloskot - define & use GGL exception
        throw std::logic_error("Can't add node to leaf node.");
    }

    /**
     * \brief Add a new leaf to this node
     */
    virtual void add_value(Box const& box, Value const& v)
    {
        m_nodes.push_back(std::make_pair(box, v));
    }


    /**
     * \brief Proyect value in position 'index' in the nodes container
     */
    virtual Value get_value(unsigned int index) const
    {
        return m_nodes[index].second;
    }

    /**
     * \brief Box projector for leaf
     */
    virtual Box get_box(unsigned int index) const
    {
        return m_nodes[index].first;
    }

    /**
     * \brief Remove value with key 'box' in this leaf
     */
    virtual void remove(Box const& box)
    {

        for (typename leaf_map::iterator it = m_nodes.begin();
             it != m_nodes.end(); ++it)
        {
            if (geometry::equals(it->first, box))
            {
                m_nodes.erase(it);
                return;
            }
        }

        // TODO: mloskot - use GGL exception
        throw std::logic_error("Node not found.");
    }

    /**
     * \brief Remove value in this leaf
     */
    virtual void remove(Value const& v)
    {
        for (typename leaf_map::iterator it = m_nodes.begin();
             it != m_nodes.end(); ++it)
        {
            if (it->second == v)
            {
                m_nodes.erase(it);
                return;
            }
        }

        // TODO: mloskot - use GGL exception
        throw std::logic_error("Node not found.");
    }

    /**
    * \brief Proyect boxes from this node
    */
    virtual std::vector<Box> get_boxes() const
    {
        std::vector<Box> result;
        for (typename leaf_map::const_iterator it = m_nodes.begin();
             it != m_nodes.end(); ++it)
        {
            result.push_back(it->first);
        }

        return result;
    }

    /**
    * \brief Print leaf (mainly for debug)
    */
    virtual void print() const
    {
        std::cerr << "\t" << " --> Leaf --------" << std::endl;
        std::cerr << "\t" << "  Size: " << m_nodes.size() << std::endl;
        for (typename leaf_map::const_iterator it = m_nodes.begin();
             it != m_nodes.end(); ++it)
        {
            std::cerr << "\t" << "  | ";
            std::cerr << "( " << geometry::get<min_corner, 0>
                (it->first) << " , " << geometry::get<min_corner, 1>
                (it->first) << " ) x ";
            std::cerr << "( " << geometry::get<max_corner, 0>
                (it->first) << " , " << geometry::get<max_corner, 1>
                (it->first) << " )";
            std::cerr << " -> ";
            std::cerr << it->second;
            std::cerr << " | " << std::endl;;
        }
        std::cerr << "\t" << " --< Leaf --------" << std::endl;
    }

private:

    /// leaves of this node
    leaf_map m_nodes;
};

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_EXTENSIONS_INDEX_RTREE_RTREE_LEAF_HPP

