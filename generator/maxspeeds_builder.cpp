#include "generator/maxspeeds_builder.hpp"

#include "generator/maxspeeds_parser.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/maxspeeds_serialization.hpp"

#include "routing_common/maxspeed_conversion.hpp"

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
  if (parsedSpeed > numeric_limits<uint16_t>::max())
    return false;
  value = static_cast<uint16_t>(parsedSpeed);
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
public:
  MaxspeedsMwmCollector(string const & dataPath,
                       map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                       string const & maxspeedCsvPath);

  vector<FeatureMaxspeed> && StealMaxspeeds();

private:
  vector<FeatureMaxspeed> m_maxspeeds;
};

MaxspeedsMwmCollector::MaxspeedsMwmCollector(
    string const & dataPath, map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
    string const & maxspeedCsvPath)
{
  OsmIdToMaxspeed osmIdToMaxspeed;
  CHECK(ParseMaxspeeds(maxspeedCsvPath, osmIdToMaxspeed), (maxspeedCsvPath));

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

    auto const & maxspeed = maxspeedIt->second;
    m_maxspeeds.push_back(ToFeatureMaxspeed(fid, maxspeed));
  });
}

vector<FeatureMaxspeed> && MaxspeedsMwmCollector::StealMaxspeeds()
{
  CHECK(is_sorted(m_maxspeeds.cbegin(), m_maxspeeds.cend(), IsFeatureIdLess), ());
  return move(m_maxspeeds);
}
}  // namespace

namespace routing
{
bool ParseMaxspeeds(string const & maxspeedsFilename, OsmIdToMaxspeed & osmIdToMaxspeed)
{
  osmIdToMaxspeed.clear();

  ifstream stream(maxspeedsFilename);
  if (!stream)
    return false;

  string line;
  while (getline(stream, line))
  {
    strings::SimpleTokenizer iter(line, kDelim);

    if (!iter)  // the line is empty
      return false;
    // @TODO(bykoianko) strings::to_uint64 returns not-zero value if |*iter| is equal to
    // a too long string of numbers. But ParseMaxspeeds() should return false in this case.
    uint64_t osmId = 0;
    if (!strings::to_uint64(*iter, osmId))
      return false;
    ++iter;

    if (!iter)
      return false;
    Maxspeed speed;
    speed.SetUnits(StringToUnits(*iter));
    ++iter;

    uint16_t forward = 0;
    if (!ParseOneSpeedValue(iter, forward))
      return false;

    speed.SetForward(forward);

    if (iter)
    {
      // There's backward maxspeed limit.
      uint16_t backward = 0;
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

void SerializeMaxspeeds(string const & dataPath, vector<FeatureMaxspeed> && speeds)
{
  if (speeds.empty())
    return;

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(MAXSPEEDS_FILE_TAG);

  MaxspeedsSerializer::Serialize(speeds, writer);
  LOG(LINFO, ("SerializeMaxspeeds(", dataPath, ", ...) serialized:", speeds.size(), "maxspeed tags."));
}

void BuildMaxspeedsSection(string const & dataPath,
                           map<uint32_t, base::GeoObjectId> const & featureIdToOsmId,
                           string const & maxspeedsFilename)
{
  MaxspeedsMwmCollector collector(dataPath, featureIdToOsmId, maxspeedsFilename);
  SerializeMaxspeeds(dataPath, collector.StealMaxspeeds());
}

void BuildMaxspeedsSection(string const & dataPath, string const & osmToFeaturePath,
                           string const & maxspeedsFilename)
{
  LOG(LINFO, ("BuildMaxspeedsSection(", dataPath, ",", osmToFeaturePath, ",", maxspeedsFilename, ")"));

  map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  CHECK(ParseFeatureIdToOsmIdMapping(osmToFeaturePath, featureIdToOsmId), ());
  BuildMaxspeedsSection(dataPath, featureIdToOsmId, maxspeedsFilename);
}
}  // namespace routing
