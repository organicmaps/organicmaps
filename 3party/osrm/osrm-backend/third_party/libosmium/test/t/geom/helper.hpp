#ifndef TEST_GEOM_HELPER_HPP
#define TEST_GEOM_HELPER_HPP

#include <string>

#include <geos/io/WKBWriter.h>

inline std::string geos_to_wkb(const geos::geom::Geometry* geometry) {
    std::stringstream ss;
    geos::io::WKBWriter wkb_writer;
    wkb_writer.writeHEX(*geometry, ss);
    return ss.str();
}

#endif // TEST_GEOM_HELPER_HPP
