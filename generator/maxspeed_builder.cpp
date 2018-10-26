#include "generator/maxspeed_builder.hpp"

#include "generator/maxspeed_parser.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/maxspeed_conversion.hpp"
#include "routing/maxspeed_serialization.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <utility>

#include "defines.hpp"

// Important note. The code below in this file will be rewritten and covered by tests within the PR.
// Now it's written quickly to make a test map build this weekend.

namespace
{
char const kDelim[] = ", \t\r\n";

bool ParseOneSpeedValue(strings::SimpleTokenizer & iter, uint16_t & value)
{
  uint64_t parsedSpeed = 0;
  if (!strings::to_uint64(*iter, parsedSpeed))
    return false;
  if (parsedSpeed > std::numeric_limits<uint16_t>::max())
    return false;
  value = static_cast<uint16_t>(parsedSpeed);
  ++iter;
  return true;
}
}  // namespace

namespace routing
{
using namespace feature;
using namespace generator;
using namespace std;

// @TODO(bykoianko) Consider implement a maxspeed collector class instead of ParseMaxspeeds() and
// part of BuildMaxspeed() methods.
bool ParseMaxspeeds(string const & maxspeedFilename, OsmIdToMaxspeed & osmIdToMaxspeed)
{
  osmIdToMaxspeed.clear();

  ifstream stream(maxspeedFilename);
  if (stream.fail())
    return false;

  string line;
  while (std::getline(stream, line))
  {
    strings::SimpleTokenizer iter(line, kDelim);
    if (!iter)  // the line is empty
      return false;

    uint64_t osmId = 0;
    if (!strings::to_uint64(*iter, osmId))
      return false;
   ++iter;

    ParsedMaxspeed speed;
    speed.m_units = StringToUnits(*iter);
    ++iter;

    if (!ParseOneSpeedValue(iter, speed.m_forward))
      return false;

    if (iter)
    {
      // There's backward maxspeed limit.
      if (!ParseOneSpeedValue(iter, speed.m_backward))
        return false;

      if (iter)
        return false;
    }

    auto const res = osmIdToMaxspeed.insert(make_pair(base::MakeOsmWay(osmId), speed));
    if (!res.second)
      return false;
  }
  return true;
}

void SerializeMaxspeed(string const & dataPath, vector<FeatureMaxspeed> && speeds)
{
  LOG(LINFO, ("SerializeMaxspeed(", dataPath, ", ...) speeds size:", speeds.size()));
  if (speeds.empty())
    return;

  LOG(LINFO, ("SerializeMaxspeed() The fisrt speed", speeds[0]));

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(MAXSPEED_FILE_TAG);

  MaxspeedSerializer::Serialize(speeds, writer);
}

bool BuildMaxspeed(string const & dataPath, string const & osmToFeaturePath,
                   string const & maxspeedFilename)
{
  LOG(LINFO, ("BuildMaxspeed(", dataPath, ",", osmToFeaturePath, ",", maxspeedFilename, ")"));

  OsmIdToMaxspeed osmIdToMaxspeed;
  CHECK(ParseMaxspeeds(maxspeedFilename, osmIdToMaxspeed), ());
  LOG(LINFO, ("BuildMaxspeed() osmIdToMaxspeed size:", osmIdToMaxspeed.size()));

  map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  CHECK(ParseFeatureIdToOsmIdMapping(osmToFeaturePath, featureIdToOsmId), ());
  LOG(LINFO, ("BuildMaxspeed() featureIdToOsmId size:", featureIdToOsmId.size()));

  vector<FeatureMaxspeed> speeds;
  ForEachFromDat(dataPath, [&](FeatureType & ft, uint32_t fid) {
    if (!routing::IsCarRoad(TypesHolder(ft)))
      return;

    auto const osmIdIt = featureIdToOsmId.find(fid);
    if (osmIdIt == featureIdToOsmId.cend())
      return;

    // @TODO Consider adding check here. |fid| should be in |featureIdToOsmId| anyway.
    auto const maxspeedIt = osmIdToMaxspeed.find(osmIdIt->second);
    if (maxspeedIt == osmIdToMaxspeed.cend())
      return;

    auto const & parsedMaxspeed = maxspeedIt->second;
    // Note. It's wrong that by default if there's no maxspeed backward SpeedInUnits::m_units
    // is Metric and according to this code it's be saved as Imperial for country
    // with imperial metrics. Anyway this code should be rewritten before merge.
    speeds.push_back(
        FeatureMaxspeed(fid, SpeedInUnits(parsedMaxspeed.m_forward, parsedMaxspeed.m_units),
                        SpeedInUnits(parsedMaxspeed.m_backward, parsedMaxspeed.m_units)));
  });

  SerializeMaxspeed(dataPath, move(speeds));
  return true;
}
}  // namespace routing
