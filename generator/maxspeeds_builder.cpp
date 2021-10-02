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

FeatureMaxspeed ToFeatureMaxspeed(uint32_t featureId, Maxspeed const & maxspeed)
{
  return FeatureMaxspeed(featureId, maxspeed.GetUnits(), maxspeed.GetForward(),
                         maxspeed.GetBackward());
}

/// \brief Collects all maxspeed tag values of specified mwm based on maxspeeds.csv file.
class MaxspeedsMwmCollector
{
  vector<FeatureMaxspeed> m_maxspeeds;

public:
  MaxspeedsMwmCollector(IndexGraph * graph, string const & dataPath,
                       map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                       string const & maxspeedCsvPath)
  {
    OsmIdToMaxspeed osmIdToMaxspeed;
    CHECK(ParseMaxspeeds(maxspeedCsvPath, osmIdToMaxspeed), (maxspeedCsvPath));

    auto const GetOsmID = [&](uint32_t fid) -> base::GeoObjectId
    {
      auto const osmIdIt = featureIdToOsmId.find(fid);
      if (osmIdIt == featureIdToOsmId.cend())
        return base::GeoObjectId();
      return osmIdIt->second;
    };
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
    auto const GetRoad = [&](uint32_t fid) -> routing::RoadGeometry const &
    {
      return graph->GetRoadGeometry(fid);
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

    ForEachFeature(dataPath, [&](FeatureType & ft, uint32_t fid)
    {
      if (!routing::IsCarRoad(TypesHolder(ft)))
        return;

      Maxspeed * maxSpeed = GetSpeed(fid);
      if (!maxSpeed)
          return;

      auto const osmid = GetOsmID(fid).GetSerialId();

      // Recalculate link speed accordint to the ingoing highway.
      // See MaxspeedsCollector::CollectFeature.
      if (maxSpeed->GetForward() == routing::kCommonMaxSpeedValue)
      {
        // Check if we are in unit tests.
        if (graph == nullptr)
          return;

        // 0 - not updated, 1 - goto next iteration, 2 - updated
        int status;

        // Check ingoing first, then - outgoing.
        for (bool direction : { false, true })
        {
          Segment seg(0, fid, 0, true);
          if (direction)
            seg = GetOpposite(seg);

          std::unordered_set<uint32_t> reviewed;
          do
          {
            status = 0;
            reviewed.insert(seg.GetFeatureId());

            IndexGraph::SegmentEdgeListT edges;
            graph->GetEdgeList(seg, direction, true /* useRoutingOptions */, edges);
            for (auto const & e : edges)
            {
              Segment const target = e.GetTarget();
              Maxspeed * s = GetSpeed(target.GetFeatureId());
              if (s)
              {
                if (s->GetForward() != routing::kCommonMaxSpeedValue)
                {
                  status = 2;
                  maxSpeed->SetForward(s->GetForward());

                  // In theory, we should iterate on forward and backward segments/speeds separately,
                  // but I don't see any reason for this complication.
                  if (!GetRoad(fid).IsOneWay())
                    maxSpeed->SetBackward(s->GetForward());

                  LOG(LINFO, ("Updated link speed for way", osmid, "with", *maxSpeed));
                  break;
                }
                else if (reviewed.find(target.GetFeatureId()) == reviewed.end())
                {
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

      m_maxspeeds.push_back(ToFeatureMaxspeed(fid, *maxSpeed));
    });
  }

  vector<FeatureMaxspeed> const & GetMaxspeeds()
  {
    CHECK(is_sorted(m_maxspeeds.cbegin(), m_maxspeeds.cend(), IsFeatureIdLess), ());
    return m_maxspeeds;
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

void SerializeMaxspeeds(string const & dataPath, vector<FeatureMaxspeed> const & speeds)
{
  if (speeds.empty())
    return;

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  auto writer = cont.GetWriter(MAXSPEEDS_FILE_TAG);

  MaxspeedsSerializer::Serialize(speeds, *writer);
  LOG(LINFO, ("SerializeMaxspeeds(", dataPath, ", ...) serialized:", speeds.size(), "maxspeed tags."));
}

void BuildMaxspeedsSection(IndexGraph * graph, string const & dataPath,
                           map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                           string const & maxspeedsFilename)
{
  MaxspeedsMwmCollector collector(graph, dataPath, featureIdToOsmId, maxspeedsFilename);
  SerializeMaxspeeds(dataPath, collector.GetMaxspeeds());
}

void BuildMaxspeedsSection(IndexGraph * graph, string const & dataPath,
                           string const & osmToFeaturePath, string const & maxspeedsFilename)
{
  map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  CHECK(ParseWaysFeatureIdToOsmIdMapping(osmToFeaturePath, featureIdToOsmId), ());
  BuildMaxspeedsSection(graph, dataPath, featureIdToOsmId, maxspeedsFilename);
}
}  // namespace routing
