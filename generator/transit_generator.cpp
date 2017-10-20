#include "generator/transit_generator.hpp"

#include "generator/utils.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/index_router.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_speed_limits.hpp"
#include "routing_common/transit_types.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/mercator.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>

#include "3party/jansson/src/jansson.h"

using namespace generator;
using namespace platform;
using namespace routing;
using namespace routing::transit;
using namespace std;

namespace
{
bool LessById(Stop const & s1, Stop const & s2)
{
  return s1.GetId() < s2.GetId();
}

/// \returns ref to a stop at |stops| by |stopId|.
Stop const & FindStopById(vector<Stop> const & stops, StopId stopId)
{
  ASSERT(is_sorted(stops.cbegin(), stops.cend(), LessById), ());
  auto s1Id = equal_range(stops.cbegin(), stops.cend(),
          Stop(stopId, FeatureIdentifiers(), kInvalidTransferId, {}, m2::PointD(), {}),
          LessById);
  CHECK(s1Id.first != stops.cend(), ("No a stop with id:", stopId, "in stops:", stops));
  CHECK_EQUAL(distance(s1Id.first, s1Id.second), 1, ("A stop with id:", stopId, "is not unique in stops:", stops));
  return *s1Id.first;
}

/// Extracts the file name from |filePath| and drops all extensions.
string GetFileName(string const & filePath)
{
  string name = filePath;
  my::GetNameFromFullPath(name);

  string nameWithExt;
  do
  {
    nameWithExt = name;
    my::GetNameWithoutExt(name);
  }
  while (nameWithExt != name);

  return name;
}

template <class Item>
void DeserializeFromJson(my::Json const & root, string const & key,
                         OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, vector<Item> & items)
{
  items.clear();
  DeserializerFromJson deserializer(root.get(), osmIdToFeatureIdsMap);
  deserializer(items, key.c_str());
}

void DeserializeGatesFromJson(my::Json const & root, string const & mwmDir, string const & countryId,
                              OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, vector<Gate> & gates)
{
  DeserializeFromJson(root, "gates", osmIdToFeatureIdsMap, gates);

  // Creating IndexRouter.
  SingleMwmIndex index(my::JoinFoldersToPath(mwmDir, countryId + DATA_FILE_EXTENSION));

  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(infoGetter, ());

  auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
    return infoGetter->GetRegionCountryId(pt);
  };

  auto const getMwmRectByName = [&](string const & c) -> m2::RectD {
    CHECK_EQUAL(countryId, c, ());
    return infoGetter->GetLimitRectForLeaf(c);
  };

  CHECK_EQUAL(index.GetMwmId().GetInfo()->GetType(), MwmInfo::COUNTRY, ());
  auto numMwmIds = make_shared<NumMwmIds>();
  numMwmIds->RegisterFile(CountryFile(countryId));

  // Note. |indexRouter| is valid while |index| is valid.
  IndexRouter indexRouter(VehicleType::Pedestrian, false /* load altitudes */,
                          CountryParentNameGetterFn(), countryFileGetter, getMwmRectByName,
                          numMwmIds, MakeNumMwmTree(*numMwmIds, *infoGetter),
                          traffic::TrafficCache(), index.GetIndex());

  // Looking for the best segment for every gate.
  for (auto & gate : gates)
  {
    // Note. For pedestrian routing all the segments are considered as twoway segments so
    // IndexRouter.FindBestSegment() method finds the same segment for |isOutgoing| == true
    // and |isOutgoing| == false.
    Segment bestSegment;
    try
    {
      if (indexRouter.FindBestSegmentInSingleMwm(gate.GetPoint(),
                                                 m2::PointD::Zero() /* direction */,
                                                 true /* isOutgoing */, bestSegment))
      {
        CHECK_EQUAL(bestSegment.GetMwmId(), 0, ());
        gate.SetBestPedestrianSegment(SingleMwmSegment(
            bestSegment.GetFeatureId(), bestSegment.GetSegmentIdx(), bestSegment.IsForward()));
      }
    }
    catch (RootException const & e)
    {
      LOG(LDEBUG, ("Point of a gate belongs to several mwms or doesn't belong to any mwm. Gate:",
                   gate, e.what()));
    }
  }
}

template <class Item>
bool IsValid(vector<Item> const & items)
{
  return all_of(items.cbegin(), items.cend(), [](Item const & item) { return item.IsValid(); });
}

/// \brief Reads from |root| (json) and serializes an array to |serializer|.
template <class Item>
void SerializeObject(my::Json const & root, string const & key,
                     OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, Serializer<FileWriter> & serializer)
{
  vector<Item> items;
  DeserializeFromJson(root, key, osmIdToFeatureIdsMap, items);
  CHECK(IsValid(items), ("key:", key, "items:", items));
  serializer(items);
}

/// \brief Updates |edges| by adding valid value for Edge::m_weight if it's not valid.
/// \note Edge::m_stop1Id and Edge::m_stop2Id should be valid for every edge in |edges| before call.
void CalculateEdgeWeight(vector<Stop> const & stops, vector<transit::Edge> & edges)
{
  CHECK(is_sorted(stops.cbegin(), stops.cend(), LessById), ());

  for (auto & e : edges)
  {
    if (e.GetWeight() != kInvalidWeight)
      continue;

    Stop const & s1 = FindStopById(stops, e.GetStop1Id());
    Stop const & s2 = FindStopById(stops, e.GetStop2Id());
    double const lengthInMeters = MercatorBounds::DistanceOnEarth(s1.GetPoint(), s2.GetPoint());
    e.SetWeight(lengthInMeters / kTransitAverageSpeedMPS);
  }
}

void FillOsmIdToFeatureIdMap(string const & osmIdsToFeatureIdPath, OsmIdToFeatureIdsMap & map)
{
  CHECK(ForEachOsmId2FeatureId(osmIdsToFeatureIdPath,
                               [&map](osm::Id const & osmId, uint32_t featureId) {
                                 map[osmId].push_back(featureId);
                               }),
        (osmIdsToFeatureIdPath));
}
}  // namespace

namespace routing
{
namespace transit
{
// DeserializerFromJson ---------------------------------------------------------------------------
DeserializerFromJson::DeserializerFromJson(json_struct_t* node,
                                           OsmIdToFeatureIdsMap const & osmIdToFeatureIds)
    : m_node(node), m_osmIdToFeatureIds(osmIdToFeatureIds)
{
}

void DeserializerFromJson::operator()(m2::PointD & p, char const * name)
{
  json_t * pointItem = nullptr;
  if (name == nullptr)
    pointItem = m_node; // Array item case
  else
    pointItem = my::GetJSONObligatoryField(m_node, name);

  CHECK(json_is_object(pointItem), ());
  FromJSONObject(pointItem, "x", p.x);
  FromJSONObject(pointItem, "y", p.y);
}

void DeserializerFromJson::operator()(FeatureIdentifiers & id, char const * name)
{
  // Conversion osm id to feature id.
  string osmIdStr;
  GetField(osmIdStr, name);
  CHECK(strings::is_number(osmIdStr), ());
  uint64_t osmIdNum;
  CHECK(strings::to_uint64(osmIdStr, osmIdNum), ());
  osm::Id const osmId(osmIdNum);
  auto const it = m_osmIdToFeatureIds.find(osmId);
  CHECK(it != m_osmIdToFeatureIds.cend(), ());
  CHECK_EQUAL(it->second.size(), 1,
              ("Osm id:", osmId, "from transit graph doesn't present by a single feature in mwm."));
  id = FeatureIdentifiers(osmId.EncodedId() /* osm id */, it->second[0] /* feature id */);
}

void BuildTransit(string const & mwmPath, string const & osmIdsToFeatureIdPath,
                  string const & transitDir)
{
  LOG(LERROR, ("This method is under construction and should not be used for building production mwm "
      "sections."));
  NOTIMPLEMENTED();

  string const countryId = GetFileName(mwmPath);
  string const graphFullPath = my::JoinFoldersToPath(transitDir, countryId + TRANSIT_FILE_EXTENSION);

  Platform::EFileType fileType;
  Platform::EError const errCode = Platform::GetFileType(graphFullPath, fileType);
  if (errCode != Platform::EError::ERR_OK || fileType != Platform::EFileType::FILE_TYPE_REGULAR)
  {
    LOG(LINFO, ("For mwm:", mwmPath, TRANSIT_FILE_EXTENSION, "file not found"));
    return;
  }

  // @TODO(bykoianko) In the future transit edges which cross mwm border will be split in the generator. Then
  // routing will support cross mwm transit routing. In current version every json with transit graph
  // should have a special name: <country id>.transit.json.

  LOG(LINFO, (TRANSIT_FILE_TAG, "section is being created. Country id:", countryId, ". Based on:", graphFullPath));

  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(graphFullPath)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & ex)
  {
    LOG(LCRITICAL, ("Can't open", graphFullPath, ex.what()));
  }

  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", graphFullPath));

  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdMap(osmIdsToFeatureIdPath, mapping);

  // Note. |gates| has to be deserialized from json before starting writing transit section to mwm since
  // the mwm is used to filled |gates|.
  vector<Gate> gates;
  DeserializeGatesFromJson(root, my::GetDirectory(mwmPath), countryId, mapping, gates);
  CHECK(IsValid(gates), (gates));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(TRANSIT_FILE_TAG);

  TransitHeader header;

  auto const startOffset = w.Pos();
  Serializer<FileWriter> serializer(w);
  header.Visit(serializer);

  vector<Stop> stops;
  DeserializeFromJson(root, "stops", mapping, stops);
  CHECK(IsValid(stops), ("stops:", stops));
  sort(stops.begin(), stops.end(), LessById);
  serializer(stops);
  header.m_gatesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(gates);
  header.m_edgesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  vector<Edge> edges;
  DeserializeFromJson(root, "edges", mapping, edges);
  CalculateEdgeWeight(stops, edges);
  CHECK(IsValid(stops), ("edges:", edges));
  serializer(edges);
  header.m_transfersOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Transfer>(root, "transfers", mapping, serializer);
  header.m_linesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Line>(root, "lines", mapping, serializer);
  header.m_shapesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Shape>(root, "shapes", mapping, serializer);
  header.m_networksOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Network>(root, "networks", mapping, serializer);
  header.m_endOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  // Rewriting header info.
  CHECK(header.IsValid(), (header));
  auto const endOffset = w.Pos();
  w.Seek(startOffset);
  header.Visit(serializer);
  w.Seek(endOffset);
  LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. Header:", header));
}
}  // namespace transit
}  // namespace routing
