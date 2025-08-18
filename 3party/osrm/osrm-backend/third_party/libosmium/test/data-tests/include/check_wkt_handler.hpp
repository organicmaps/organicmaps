#ifndef CHECK_WKT_HANDLER_HPP
#define CHECK_WKT_HANDLER_HPP

#include <cassert>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include <osmium/handler.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/types.hpp>

class CheckWKTHandler : public osmium::handler::Handler {

    std::map<osmium::object_id_type, std::string> m_geometries;
    osmium::geom::WKTFactory<> m_factory;

    void read_wkt_file(const std::string& filename) {
        std::ifstream in(filename, std::ifstream::in);
        if (in) {
            osmium::object_id_type id;
            std::string line;
            while (std::getline(in, line)) {
                size_t pos = line.find_first_of(' ');

                if (pos == std::string::npos) {
                    std::cerr << filename << " not formatted correctly\n";
                    exit(1);
                }

                std::string id_str = line.substr(0, pos);
                std::istringstream iss(id_str);
                iss >> id;

                if (m_geometries.find(id) != m_geometries.end()) {
                     std::cerr << filename + " contains id " << id << "twice\n";
                     exit(1);
                }

                m_geometries[id] = line.substr(pos+1);
            }
        }
    }

public:

    CheckWKTHandler(const std::string& dirname, int test_id) :
        osmium::handler::Handler() {

        std::string filename = dirname + "/" + std::to_string(test_id / 100) + "/" + std::to_string(test_id) + "/";
        read_wkt_file(filename + "nodes.wkt");
        read_wkt_file(filename + "ways.wkt");
    }

    ~CheckWKTHandler() {
        if (!m_geometries.empty()) {
            for (const auto& geom : m_geometries) {
                std::cerr << "geometry id " << geom.first << " not in data.osm.\n";
            }
            exit(1);
        }
    }

    void node(const osmium::Node& node) {
        const std::string wkt = m_geometries[node.id()];
        assert(wkt != "" && "Missing geometry for node in nodes.wkt");

        std::string this_wkt = m_factory.create_point(node.location());
        assert(wkt == this_wkt && "wkt geometries don't match");
        m_geometries.erase(node.id());
    }

    void way(const osmium::Way& way) {
        const std::string wkt = m_geometries[way.id()];
        assert(wkt != "" && "Missing geometry for way in ways.wkt");

        std::string this_wkt = m_factory.create_linestring(way);
        assert(wkt == this_wkt && "wkt geometries don't match");
        m_geometries.erase(way.id());
    }

}; // CheckWKTHandler


#endif // CHECK_WKT_HANDLER_HPP
