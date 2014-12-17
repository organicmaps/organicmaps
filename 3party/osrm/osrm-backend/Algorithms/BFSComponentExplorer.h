#ifndef BFS_COMPONENT_EXPLORER_H_
#define BFS_COMPONENT_EXPLORER_H_

#include "../typedefs.h"
#include "../DataStructures/RestrictionMap.h"

#include <queue>
#include <unordered_set>

// Explores the components of the given graph while respecting turn restrictions
// and barriers.
template <typename GraphT> class BFSComponentExplorer
{
  public:
    BFSComponentExplorer(const GraphT &dynamic_graph,
                         const RestrictionMap &restrictions,
                         const std::unordered_set<NodeID> &barrier_nodes)
        : m_graph(dynamic_graph), m_restriction_map(restrictions), m_barrier_nodes(barrier_nodes)
    {
        BOOST_ASSERT(m_graph.GetNumberOfNodes() > 0);
    }

    /*!
     * Returns the size of the component that the node belongs to.
     */
    unsigned int GetComponentSize(const NodeID node) const
    {
        BOOST_ASSERT(node < m_component_index_list.size());

        return m_component_index_size[m_component_index_list[node]];
    }

    unsigned int GetNumberOfComponents() { return m_component_index_size.size(); }

    /*!
     * Computes the component sizes.
     */
    void run()
    {
        std::queue<std::pair<NodeID, NodeID>> bfs_queue;
        unsigned current_component = 0;

        BOOST_ASSERT(m_component_index_list.empty());
        BOOST_ASSERT(m_component_index_size.empty());

        unsigned num_nodes = m_graph.GetNumberOfNodes();

        m_component_index_list.resize(num_nodes, std::numeric_limits<unsigned>::max());

        BOOST_ASSERT(num_nodes > 0);

        // put unexplorered node with parent pointer into queue
        for (NodeID node = 0; node < num_nodes; ++node)
        {
            if (std::numeric_limits<unsigned>::max() == m_component_index_list[node])
            {
                unsigned size = ExploreComponent(bfs_queue, node, current_component);

                // push size into vector
                m_component_index_size.emplace_back(size);
                ++current_component;
            }
        }
    }

  private:
    /*!
     * Explores the current component that starts at node using BFS.
     */
    unsigned ExploreComponent(std::queue<std::pair<NodeID, NodeID>> &bfs_queue,
                                     NodeID node,
                                     unsigned current_component)
    {
        /*
           Graphical representation of variables:

           u           v           w
           *---------->*---------->*
                            e2
        */

        bfs_queue.emplace(node, node);
        // mark node as read
        m_component_index_list[node] = current_component;

        unsigned current_component_size = 1;

        while (!bfs_queue.empty())
        {
            // fetch element from BFS queue
            std::pair<NodeID, NodeID> current_queue_item = bfs_queue.front();
            bfs_queue.pop();

            const NodeID v = current_queue_item.first; // current node
            const NodeID u = current_queue_item.second; // parent
            // increment size counter of current component
            ++current_component_size;
            const bool is_barrier_node = (m_barrier_nodes.find(v) != m_barrier_nodes.end());
            if (!is_barrier_node)
            {
                const NodeID to_node_of_only_restriction =
                    m_restriction_map.CheckForEmanatingIsOnlyTurn(u, v);

                for (auto e2 : m_graph.GetAdjacentEdgeRange(v))
                {
                    const NodeID w = m_graph.GetTarget(e2);

                    if (to_node_of_only_restriction != std::numeric_limits<unsigned>::max() &&
                        w != to_node_of_only_restriction)
                    {
                        // At an only_-restriction but not at the right turn
                        continue;
                    }

                    if (u != w)
                    {
                        // only add an edge if turn is not a U-turn except
                        // when it is at the end of a dead-end street.
                        if (!m_restriction_map.CheckIfTurnIsRestricted(u, v, w))
                        {
                            // only add an edge if turn is not prohibited
                            if (std::numeric_limits<unsigned>::max() == m_component_index_list[w])
                            {
                                // insert next (node, parent) only if w has
                                // not yet been explored
                                // mark node as read
                                m_component_index_list[w] = current_component;
                                bfs_queue.emplace(w, v);
                            }
                        }
                    }
                }
            }
        }

        return current_component_size;
    }

    std::vector<unsigned> m_component_index_list;
    std::vector<NodeID> m_component_index_size;

    const GraphT &m_graph;
    const RestrictionMap &m_restriction_map;
    const std::unordered_set<NodeID> &m_barrier_nodes;
};

#endif // BFS_COMPONENT_EXPLORER_H_
