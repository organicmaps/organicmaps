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

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "defines.hpp"

// Important note. The code below in this file will be rewritten and covered by tests within the PR.
// Now it's written quickly to make a test map build this weekend.

using namespace feature;
using namespace generator;
using namespace routing;
using namespace std;

namespace
{
char const kDelim[] = ", \t\r\n";

bool ParseOneSpeedValue(strings::SimpleTokenizer & iter, uint16_t & value)
{
  if (!iter)
    return false;

  uint64_t parsedSpeed = 0;
  if (!strings::to_uint64(*iter, parsedSpeed))
    return false;
  if (parsedSpeed > std::numeric_limits<uint16_t>::max())
    return false;
  value = static_cast<uint16_t>(parsedSpeed);
  ++iter;
  return true;
}

/// \brief Collects all maxspeed tag value of specified mwm based on maxspeed.csv file.
class MaxspeedMwmCollector
{
public:
  MaxspeedMwmCollector(std::string const & dataPath,
                       std::map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                       std::string const & maxspeedFilename);

  std::vector<FeatureMaxspeed> && StealMaxspeeds() { return move(m_maxspeeds); }

private:
  std::vector<FeatureMaxspeed> m_maxspeeds;
};

MaxspeedMwmCollector::MaxspeedMwmCollector(
    string const & dataPath, map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
    string const & maxspeedFilename)
{
  OsmIdToMaxspeed osmIdToMaxspeed;
  CHECK(ParseMaxspeeds(maxspeedFilename, osmIdToMaxspeed), (maxspeedFilename));

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
    m_maxspeeds.push_back(
        FeatureMaxspeed(fid, SpeedInUnits(parsedMaxspeed.m_forward, parsedMaxspeed.m_units),
                        SpeedInUnits(parsedMaxspeed.m_backward, parsedMaxspeed.m_units)));
  });
}
}  // namespace

namespace routing
{
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

    // @TODO(bykoianko) trings::to_uint64 returns not-zero value if |*iter| is equal to
    // a too long string of numbers. But ParseMaxspeeds() should return false in this case.
    uint64_t osmId = 0;
    if (!strings::to_uint64(*iter, osmId))
      return false;
    ++iter;
    if (!iter)
      return false;

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
  if (speeds.empty())
    return;

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(MAXSPEED_FILE_TAG);

  MaxspeedSerializer::Serialize(speeds, writer);
  LOG(LINFO, ("SerializeMaxspeed(", dataPath, ", ...) serialized:", speeds.size(), "maxspeed tags."));
}

void BuildMaxspeed(string const & dataPath, map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                   string const & maxspeedFilename)
{
  MaxspeedMwmCollector collector(dataPath, featureIdToOsmId, maxspeedFilename);
  SerializeMaxspeed(dataPath, collector.StealMaxspeeds());
}

void BuildMaxspeed(string const & dataPath, string const & osmToFeaturePath,
                   string const & maxspeedFilename)
{
  LOG(LINFO, ("BuildMaxspeed(", dataPath, ",", osmToFeaturePath, ",", maxspeedFilename, ")"));

  map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  CHECK(ParseFeatureIdToOsmIdMapping(osmToFeaturePath, featureIdToOsmId), ());
  BuildMaxspeed(dataPath, featureIdToOsmId, maxspeedFilename);
}

std::string DebugPrint(ParsedMaxspeed const & parsedMaxspeed)
{
  std::ostringstream oss;
  oss << "ParsedMaxspeed [ m_units:" << DebugPrint(parsedMaxspeed.m_units)
      << " m_forward:" << parsedMaxspeed.m_forward
      << " m_backward:" << parsedMaxspeed.m_backward << " ]";
  return oss.str();
}
}  // namespace routing
