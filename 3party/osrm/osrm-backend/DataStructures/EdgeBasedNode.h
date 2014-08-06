#ifndef EDGE_BASED_NODE_H
#define EDGE_BASED_NODE_H

#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

#include <limits>

struct EdgeBasedNode
{

    EdgeBasedNode() :
        forward_edge_based_node_id(SPECIAL_NODEID),
        reverse_edge_based_node_id(SPECIAL_NODEID),
        u(SPECIAL_NODEID),
        v(SPECIAL_NODEID),
        name_id(0),
        forward_weight(INVALID_EDGE_WEIGHT >> 1),
        reverse_weight(INVALID_EDGE_WEIGHT >> 1),
        forward_offset(0),
        reverse_offset(0),
        packed_geometry_id(SPECIAL_EDGEID),
        fwd_segment_position( std::numeric_limits<unsigned short>::max() ),
        is_in_tiny_cc(false)
    { }

    explicit EdgeBasedNode(
        NodeID forward_edge_based_node_id,
        NodeID reverse_edge_based_node_id,
        NodeID u,
        NodeID v,
        unsigned name_id,
        int forward_weight,
        int reverse_weight,
        int forward_offset,
        int reverse_offset,
        unsigned packed_geometry_id,
        unsigned short fwd_segment_position,
        bool belongs_to_tiny_component
    ) :
        forward_edge_based_node_id(forward_edge_based_node_id),
        reverse_edge_based_node_id(reverse_edge_based_node_id),
        u(u),
        v(v),
        name_id(name_id),
        forward_weight(forward_weight),
        reverse_weight(reverse_weight),
        forward_offset(forward_offset),
        reverse_offset(reverse_offset),
        packed_geometry_id(packed_geometry_id),
        fwd_segment_position(fwd_segment_position),
        is_in_tiny_cc(belongs_to_tiny_component)
    {
        BOOST_ASSERT((forward_edge_based_node_id != SPECIAL_NODEID) ||
                     (reverse_edge_based_node_id != SPECIAL_NODEID));
    }

    static inline FixedPointCoordinate Centroid(const FixedPointCoordinate & a, const FixedPointCoordinate & b)
    {
        FixedPointCoordinate centroid;
        //The coordinates of the midpoint are given by:
        centroid.lat = (a.lat + b.lat)/2;
        centroid.lon = (a.lon + b.lon)/2;
        return centroid;
    }

    bool IsCompressed() const
    {
        return packed_geometry_id != SPECIAL_EDGEID;
    }

    NodeID forward_edge_based_node_id; // needed for edge-expanded graph
    NodeID reverse_edge_based_node_id; // needed for edge-expanded graph
    NodeID u;   // indices into the coordinates array
    NodeID v;   // indices into the coordinates array
    unsigned name_id;   // id of the edge name
    int forward_weight; // weight of the edge
    int reverse_weight; // weight in the other direction (may be different)
    int forward_offset; // prefix sum of the weight up the edge TODO: short must suffice
    int reverse_offset; // prefix sum of the weight from the edge TODO: short must suffice
    unsigned packed_geometry_id; // if set, then the edge represents a packed geometry
    unsigned short fwd_segment_position; // segment id in a compressed geometry
    bool is_in_tiny_cc;
};

#endif //EDGE_BASED_NODE_H
