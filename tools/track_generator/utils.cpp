#include "track_generator/utils.hpp"

#include "routing/routes_builder/routes_builder.hpp"

#include "routing/base/followed_polyline.hpp"
#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"

#include "kml/serdes.hpp"
#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <vector>

namespace track_generator_tool
{
using namespace std;

namespace
{
vector<geometry::PointWithAltitude> GetTrackPoints(std::vector<m2::PointD> const & routePoints)
{
  auto const size = routePoints.size();
  vector<geometry::PointWithAltitude> result;
  result.reserve(size);

  for (size_t i = 0; i < size;)
  {
    size_t j = i + 1;
    size_t consecutiveNumber = 1;
    /// Check number of consecutive junctions with the same point.
    while (j < size && routePoints[j] == routePoints[i])
    {
      ++j;
      ++consecutiveNumber;
    }

    ///             S3  S4
    ///             ⥀  ⥀
    ///             *   |
    ///             ⋀   S5
    ///             |   |
    ///             S2  ⋁
    ///             |   *
    /// —— S1 ——> *      —— S6 ——> *

    /// If there is 3 or more consecutive point than it's an intermediate point
    /// which is the start and the end of the subroute simultaneously.
    if (consecutiveNumber >= 3)
    {
      /// —— S1 ——> *
      ///           |
      ///           S2
      ///           |
      ///           ⋁
      ///           *

      /// Check if there are two perpendicular fake segments and get rid from both of them.
      if (!result.empty() && result.back().GetPoint() == routePoints[j])
      {
        result.pop_back();
        ++j;
      }
    }
    else
    {
      result.emplace_back(routePoints[i], geometry::kDefaultAltitudeMeters);
    }

    i = j;
  }

  return result;
}
}  // namespace

void GenerateTracks(string const & inputDir, string const & outputDir, routing::VehicleType type)
{
  CHECK(!inputDir.empty(), ());
  CHECK(!outputDir.empty(), ());
  CHECK_LESS(type, routing::VehicleType::Count, ());

  Platform::FilesList files;
  Platform::GetFilesByExt(inputDir, ".kml", files);
  CHECK(!files.empty(), ("Input directory doesn't contain kmls."));

  auto & routesBuilder = routing::routes_builder::RoutesBuilder::GetSimpleRoutesBuilder();

  size_t numberOfTracks = 0;
  size_t numberOfNotConverted = 0;
  for (auto const & file : files)
  {
    LOG(LINFO, ("Start generating tracks for file", file));
    kml::FileData data;
    try
    {
      kml::DeserializerKml des(data);
      FileReader r(base::JoinPath(inputDir, file));
      des.Deserialize(r);
    }
    catch (FileReader::Exception const & ex)
    {
      CHECK(false, ("Can't convert file", file, "\nException:", ex.what()));
    }
    catch (kml::DeserializerKml::DeserializeException const & ex)
    {
      CHECK(false, ("Can't deserialize file", file, "\nException:", ex.what()));
    }

    if (data.m_tracksData.empty())
    {
      LOG(LINFO, ("No tracks in file", file));
      continue;
    }

    numberOfTracks += data.m_tracksData.size();
    for (auto & track : data.m_tracksData)
    {
      for (auto & line : track.m_geometry.m_lines)
      {
        std::vector<m2::PointD> waypoints;
        waypoints.reserve(line.size());
        for (auto const & pt : line)
          waypoints.push_back(pt.GetPoint());

        routing::routes_builder::RoutesBuilder::Params params(type, std::move(waypoints));

        auto result = routesBuilder.ProcessTask(params);
        if (result.m_code != routing::RouterResultCode::NoError)
        {
          LOG(LINFO, ("Can't convert track", track.m_id, "from file:", file, "Error:", result.m_code));
          ++numberOfNotConverted;
          continue;
        }

        line = GetTrackPoints(result.GetRoutes().back().m_followedPolyline.GetPolyline().GetPoints());
      }
    }

    try
    {
      kml::SerializerKml ser(data);
      FileWriter sink(base::JoinPath(outputDir, file));
      ser.Serialize(sink);
    }
    catch (FileWriter::Exception const & ex)
    {
      CHECK(false, ("Can't save file", file, "\nException:", ex.what()));
    }
    catch (kml::SerializerKml::SerializeException const & ex)
    {
      CHECK(false, ("Can't serialize file", file, "\nException:", ex.what()));
    }

    LOG(LINFO, ("Finished generating tracks for file", file));
  }

  LOG(LINFO, ("Total tracks number:", numberOfTracks));
  LOG(LINFO, ("Number of not converted tracks:", numberOfNotConverted));
}
}  // namespace track_generator_tool
