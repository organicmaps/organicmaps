#include "generator/maxspeeds_builder.hpp"

#include "generator/maxspeeds_parser.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/index_graph.hpp"
#include "routing/maxspeeds_serialization.hpp"
#include "routing/routing_helpers.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/files_container.hpp"
#include "coding/file_writer.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "defines.hpp"

using namespace feature;
using namespace generator;
using namespace routing;
using namespace std;

namespace
{
char const kDelim[] = ", \t\r\n";

bool ParseOneSpeedValue(strings::SimpleTokenizer & iter, MaxspeedType & value)
{
  if (!iter)
    return false;

  uint64_t parsedSpeed = 0;
  if (!strings::to_uint64(*iter, parsedSpeed))
    return false;
  if (parsedSpeed > routing::kInvalidSpeed)
    return false;
  value = static_cast<MaxspeedType>(parsedSpeed);
  ++iter;
  return true;
}

/// \brief Collects all maxspeed tag values of specified mwm based on maxspeeds.csv file.
class MaxspeedsMwmCollector
{
  string const & m_dataPath;
  FeatureIdToOsmId const & m_ft2osm;
  IndexGraph * m_graph;

  MaxspeedConverter const & m_converter;

  std::vector<MaxspeedsSerializer::FeatureSpeedMacro> m_maxspeeds;

  struct AvgInfo
  {
    double m_lengthKM = 0;
    double m_timeH = 0;
  };
  std::unordered_map<HighwayType, AvgInfo> m_avgSpeeds;

  base::GeoObjectId GetOsmID(uint32_t fid) const
  {
    auto const osmIdIt = m_ft2osm.find(fid);
    if (osmIdIt == m_ft2osm.cend())
      return base::GeoObjectId();
    return osmIdIt->second;
  }

  routing::RoadGeometry const & GetRoad(uint32_t fid) const
  {
    return m_graph->GetRoadGeometry(fid);
  }

public:
  MaxspeedsMwmCollector(string const & dataPath, FeatureIdToOsmId const & ft2osm, IndexGraph * graph)
    : m_dataPath(dataPath), m_ft2osm(ft2osm), m_graph(graph)
    , m_converter(MaxspeedConverter::Instance())
  {
  }

  void Process(string const & maxspeedCsvPath)
  {
    OsmIdToMaxspeed osmIdToMaxspeed;
    CHECK(ParseMaxspeeds(maxspeedCsvPath, osmIdToMaxspeed), (maxspeedCsvPath));

    auto const GetSpeed = [&](uint32_t fid) -> Maxspeed *
    {
      auto osmid = GetOsmID(fid);
      if (osmid.GetType() == base::GeoObjectId::Type::Invalid)
        return nullptr;

      auto const maxspeedIt = osmIdToMaxspeed.find(osmid);
      if (maxspeedIt == osmIdToMaxspeed.cend())
        return nullptr;

      return &maxspeedIt->second;
    };
    auto const GetLastIndex = [&](uint32_t fid)
    {
      return GetRoad(fid).GetPointsCount() - 2;
    };
    auto const GetOpposite = [&](Segment const & seg)
    {
      // Assume that links are connected with main roads in first or last point, always.
      uint32_t const fid = seg.GetFeatureId();
      return Segment(0, fid, seg.GetSegmentIdx() > 0 ? 0 : GetLastIndex(fid), seg.IsForward());
    };

    auto const & converter = GetMaxspeedConverter();
    ForEachFeature(m_dataPath, [&](FeatureType & ft, uint32_t fid)
    {
      if (!routing::IsCarRoad(TypesHolder(ft)))
        return;

      Maxspeed * maxSpeed = GetSpeed(fid);
      if (!maxSpeed)
          return;

      auto const osmid = GetOsmID(fid).GetSerialId();

#define LOG_MAX_SPEED(msg) if (false) LOG(LINFO, msg)

      LOG_MAX_SPEED(("Start osmid =", osmid));

      // Recalculate link speed according to the ingoing highway.
      // See MaxspeedsCollector::CollectFeature.
      if (maxSpeed->GetForward() == routing::kCommonMaxSpeedValue)
      {
        // Check if we are in unit tests.
        if (m_graph == nullptr)
          return;

        // 0 - not updated, 1 - goto next iteration, 2 - updated
        int status;

        // Check ingoing first, then - outgoing.
        for (bool direction : { false, true })
        {
          LOG_MAX_SPEED(("Search dir =", direction));

          Segment seg(0, fid, 0, true);
          if (direction)
            seg = GetOpposite(seg);

          std::unordered_set<uint32_t> reviewed;
          do
          {
            LOG_MAX_SPEED(("Input seg =", seg));

            status = 0;
            reviewed.insert(seg.GetFeatureId());

            IndexGraph::SegmentEdgeListT edges;
            m_graph->GetEdgeList(seg, direction, false /* useRoutingOptions */, edges);
            for (auto const & e : edges)
            {
              LOG_MAX_SPEED(("Edge =", e));
              Segment const target = e.GetTarget();

              uint32_t const targetFID = target.GetFeatureId();
              LOG_MAX_SPEED(("Edge target =", target, "; osmid =", GetOsmID(targetFID).GetSerialId()));

              Maxspeed const * s = GetSpeed(targetFID);
              if (s)
              {
                if (routing::IsNumeric(s->GetForward()))
                {
                  status = 2;

                  // Main thing here is to reduce the speed, relative to a parent. So use 0.7 factor.
                  auto const speed = converter.ClosestValidMacro(
                        SpeedInUnits(std::lround(s->GetForward() * 0.7), s->GetUnits()));
                  maxSpeed->SetUnits(s->GetUnits());
                  maxSpeed->SetForward(speed.GetSpeed());

                  LOG(LINFO, ("Updated link speed for way", osmid, "with", *maxSpeed));
                  break;
                }
                else if (s->GetForward() == routing::kCommonMaxSpeedValue &&
                         reviewed.find(targetFID) == reviewed.end())
                {
                  LOG_MAX_SPEED(("Add reviewed"));

                  // Found another input link. Save it for the next iteration.
                  status = 1;
                  seg = GetOpposite(target);
                }
              }
            }
          } while (status == 1);

          if (status == 2)
            break;
        }

        if (status == 0)
        {
          LOG(LWARNING, ("Didn't find connected edge with speed for way", osmid));
          return;
        }
      }

      LOG_MAX_SPEED(("End osmid =", osmid));

      AddSpeed(fid, *maxSpeed);
    });
  }

  void AddSpeed(uint32_t featureID, Maxspeed const & speed)
  {
    // Add converted macro speed.
    SpeedInUnits const forward(speed.GetForward(), speed.GetUnits());
    CHECK(forward.IsValid(), ());
    SpeedInUnits const backward(speed.GetBackward(), speed.GetUnits());

    /// @todo Should we find closest macro speed here when Undefined? OSM has bad data sometimes.
    SpeedMacro const backwardMacro = m_converter.SpeedToMacro(backward);
    MaxspeedsSerializer::FeatureSpeedMacro ftSpeed{featureID, m_converter.SpeedToMacro(forward), backwardMacro};

    if (ftSpeed.m_forward == SpeedMacro::Undefined)
    {
      LOG(LWARNING, ("Undefined macro for forward speed", forward, "in", GetOsmID(featureID)));
      return;
    }
    if (backward.IsValid() && backwardMacro == SpeedMacro::Undefined)
    {
      LOG(LWARNING, ("Undefined macro for backward speed", backward, "in", GetOsmID(featureID)));
    }

    m_maxspeeds.push_back(ftSpeed);

    // Possible in unit tests.
    if (m_graph == nullptr)
      return;

    // Update average speed information.
    auto const & rd = GetRoad(featureID);
    auto & info = m_avgSpeeds[rd.GetHighwayType()];

    double const lenKM = rd.GetRoadLengthM() / 1000.0;
    for (auto const & s : { forward, backward })
    {
      if (s.IsNumeric())
      {
        info.m_lengthKM += lenKM;
        info.m_timeH += lenKM / s.GetSpeedKmPH();
      }
    }
  }

  void SerializeMaxspeeds() const
  {
    if (m_maxspeeds.empty())
      return;

    std::map<HighwayType, SpeedMacro> typeSpeeds;
    for (auto const & e : m_avgSpeeds)
    {
      long const speed = std::lround(e.second.m_lengthKM / e.second.m_timeH);
      if (speed < routing::kInvalidSpeed)
      {
        // Store type speeds in Metric system, like VehicleModel profiles.
        auto const speedInUnits = m_converter.ClosestValidMacro(
              { static_cast<MaxspeedType>(speed), measurement_utils::Units::Metric });

        LOG(LINFO, ("Average speed for", e.first, "=", speedInUnits));

        typeSpeeds[e.first] = m_converter.SpeedToMacro(speedInUnits);
      }
      else
        LOG(LWARNING, ("Large average speed for", e.first, "=", speed));
    }

    FilesContainerW cont(m_dataPath, FileWriter::OP_WRITE_EXISTING);
    auto writer = cont.GetWriter(MAXSPEEDS_FILE_TAG);
    MaxspeedsSerializer::Serialize(m_maxspeeds, typeSpeeds, *writer);

    LOG(LINFO, ("Serialized", m_maxspeeds.size(), "speeds for", m_dataPath));
  }
};

}  // namespace

namespace routing
{
bool ParseMaxspeeds(string const & filePath, OsmIdToMaxspeed & osmIdToMaxspeed)
{
  osmIdToMaxspeed.clear();

  ifstream stream(filePath);
  if (!stream)
    return false;

  string line;
  while (stream.good())
  {
    getline(stream, line);
    strings::SimpleTokenizer iter(line, kDelim);

    if (!iter)  // empty line
      continue;

    uint64_t osmId = 0;
    if (!strings::to_uint64(*iter, osmId))
      return false;
    ++iter;

    if (!iter)
      return false;
    Maxspeed speed;
    speed.SetUnits(StringToUnits(*iter));
    ++iter;

    MaxspeedType forward = 0;
    if (!ParseOneSpeedValue(iter, forward))
      return false;

    speed.SetForward(forward);

    if (iter)
    {
      // There's backward maxspeed limit.
      MaxspeedType backward = 0;
      if (!ParseOneSpeedValue(iter, backward))
        return false;

      speed.SetBackward(backward);

      if (iter)
        return false;
    }

    auto const res = osmIdToMaxspeed.insert(make_pair(base::MakeOsmWay(osmId), speed));
    if (!res.second)
      return false;
  }
  return true;
}

void BuildMaxspeedsSection(IndexGraph * graph, string const & dataPath,
                           map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                           string const & maxspeedsFilename)
{
  MaxspeedsMwmCollector collector(dataPath, featureIdToOsmId, graph);

  collector.Process(maxspeedsFilename);
  collector.SerializeMaxspeeds();
}

void BuildMaxspeedsSection(IndexGraph * graph, string const & dataPath,
                           string const & osmToFeaturePath, string const & maxspeedsFilename)
{
  map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  CHECK(ParseWaysFeatureIdToOsmIdMapping(osmToFeaturePath, featureIdToOsmId), ());
  BuildMaxspeedsSection(graph, dataPath, featureIdToOsmId, maxspeedsFilename);
}
}  // namespace routing
