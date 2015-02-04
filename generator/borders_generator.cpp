#include "borders_generator.hpp"

#include "../indexer/mercator.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"
#include "../std/fstream.hpp"
#include "../std/iostream.hpp"
#include "../std/sstream.hpp"

namespace osm
{
  bool ReadPolygon(istream & stream, m2::RegionD & region, string const & filename)
  {
    string line, name;
    double lon, lat;

    // read ring id, fail if it's empty
    getline(stream, name);
    if (name.empty() || name == "END")
      return false;

    while (stream.good())
    {
      getline(stream, line);
      strings::Trim(line);

      if (line.empty())
        continue;

      if (line == "END")
        break;

      istringstream iss(line);
      iss >> lon >> lat;
      CHECK(!iss.fail(), ("Incorrect data in", filename));

      region.AddPoint(MercatorBounds::FromLatLon(lat, lon));
    }

    // drop inner rings
    return name[0] != '!';
  }
}
