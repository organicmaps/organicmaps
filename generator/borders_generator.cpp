#include "generator/borders_generator.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/sstream.hpp"

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

  bool LoadBorders(string const & borderFile, vector<m2::RegionD> & outBorders)
  {
    ifstream stream(borderFile);
    string line;
    if (!getline(stream, line).good()) // skip title
    {
      LOG(LERROR, ("Polygon file is empty:", borderFile));
      return false;
    }

    m2::RegionD currentRegion;
    while (ReadPolygon(stream, currentRegion, borderFile))
    {
      CHECK(currentRegion.IsValid(), ("Invalid region in", borderFile));
      outBorders.push_back(currentRegion);
      currentRegion = m2::RegionD();
    }

    CHECK(!outBorders.empty(), ("No borders were loaded from", borderFile));
    return true;
  }
}
