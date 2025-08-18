#ifndef CHECK_BASICS_HANDLER_HPP
#define CHECK_BASICS_HANDLER_HPP

#include <iostream>
#include <unordered_set>

#include <osmium/handler.hpp>
#include <osmium/osm.hpp>

/**
 * Check some basics of the input data:
 *
 * 1. Correct number of nodes, ways, and relations
 * 2. Correct ID space used by nodes, ways, and relations
 * 3. No ID used more than once
 */
class CheckBasicsHandler : public osmium::handler::Handler {

    // Lower bound for the id range allowed in this test.
    int m_id_range;

    // In the beginning these contains the number of nodes, ways, and relations
    // supposedly in the data.osm file. They will be decremented on each object
    // and have to be 0 at the end.
    int m_num_nodes;
    int m_num_ways;
    int m_num_relations;

    // All IDs encountered in the data.osm file will be stored in this set and
    // checked for duplicates.
    std::unordered_set<osmium::object_id_type> m_ids;

    // Check id is in range [min, max] and that it isn't more than once in input.
    void id_check(osmium::object_id_type id, osmium::object_id_type min, osmium::object_id_type max) {
        if (id < m_id_range + min || id > m_id_range + max) {
            std::cerr << "  id " << id << " out of range for this test case\n";
            exit(1);
        }

        auto r = m_ids.insert(id);
        if (!r.second) {
            std::cerr << "  id " << id << " contained twice in data.osm\n";
            exit(1);
        }
    }

public:

    static const int ids_per_testcase = 1000;

    CheckBasicsHandler(int testcase, int nodes, int ways, int relations) :
        osmium::handler::Handler(),
        m_id_range(testcase * ids_per_testcase),
        m_num_nodes(nodes),
        m_num_ways(ways),
        m_num_relations(relations) {
    }

    ~CheckBasicsHandler() {
        if (m_num_nodes != 0) {
            std::cerr << "  wrong number of nodes in data.osm\n";
            exit(1);
        }
        if (m_num_ways != 0) {
            std::cerr << "  wrong number of ways in data.osm\n";
            exit(1);
        }
        if (m_num_relations != 0) {
            std::cerr << "  wrong number of relations in data.osm\n";
            exit(1);
        }
    }

    void node(const osmium::Node& node) {
        id_check(node.id(), 0, 799);
        --m_num_nodes;
    }

    void way(const osmium::Way& way) {
        id_check(way.id(), 800, 899);
        --m_num_ways;
    }

    void relations(const osmium::Relation& relation) {
        id_check(relation.id(), 900, 999);
        --m_num_relations;
    }

}; // CheckBasicsHandler


#endif // CHECK_BASICS_HANDLER_HPP
