#include "generator/transit_generator.hpp"

#include "generator/utils.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/index_router.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_speed_limits.hpp"

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
          Stop(stopId, kInvalidOsmId, kInvalidFeatureId, kInvalidTransferId, {}, m2::PointD(), {}),
          LessById);
  CHECK(s1Id.first != stops.cend(), ("No a stop with id:", stopId, "in stops:", stops));
  CHECK_EQUAL(distance(s1Id.first, s1Id.second), 1, ("A stop with id:", stopId, "is not unique in stops:", stops));
  return *s1Id.first;
}

template <class Item>
void DeserializeItemFromJson(my::Json const & root, string const & key,
                             OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap,
                             vector<Item> & items)
{
  items.clear();
  DeserializerFromJson deserializer(root.get(), osmIdToFeatureIdsMap);
  deserializer(items, key.c_str());
}

template <class Item>
bool IsValid(vector<Item> const & items)
{
  return all_of(items.cbegin(), items.cend(), [](Item const & item) { return item.IsValid(); });
}

void FillOsmIdToFeatureIdsMap(string const & osmIdToFeatureIdsPath, OsmIdToFeatureIdsMap & map)
{
  CHECK(ForEachOsmId2FeatureId(osmIdToFeatureIdsPath,
                               [&map](osm::Id const & osmId, uint32_t featureId) {
                                 map[osmId].push_back(featureId);
                               }),
        (osmIdToFeatureIdsPath));
}

std::string GetMwmPath(std::string const & mwmDir, std::string const & countryId)
{
  return my::JoinFoldersToPath(mwmDir, countryId + DATA_FILE_EXTENSION);
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
  // @todo(bykoianko) Instead of having a special operator() method for m2::PointD class it's necessary to
  // add Point class to transit_types.hpp and process it in DeserializerFromJson with regular method.
  json_t * item = nullptr;
  if (name == nullptr)
    item = m_node; // Array item case
  else
    item = my::GetJSONObligatoryField(m_node, name);

  CHECK(json_is_object(item), ("Item is not a json object:", name));
  FromJSONObject(item, "x", p.x);
  FromJSONObject(item, "y", p.y);
}

void DeserializerFromJson::operator()(FeatureIdentifiers & id, char const * name)
{
  // Conversion osm id to feature id.
  string osmIdStr;
  GetField(osmIdStr, name);
  CHECK(strings::is_number(osmIdStr), ("Osm id string is not a number:", osmIdStr));
  uint64_t osmIdNum;
  CHECK(strings::to_uint64(osmIdStr, osmIdNum),
        ("Cann't convert osm id string:", osmIdStr, "to a number."));
  osm::Id const osmId(osmIdNum);
  auto const it = m_osmIdToFeatureIds.find(osmId);
  CHECK(it != m_osmIdToFeatureIds.cend(), ("Osm id:", osmId, "(encoded", osmId.EncodedId(),
                                           ") is not found in osm id to feature ids mapping."));
  CHECK_EQUAL(it->second.size(), 1,
              ("Osm id:", osmId, "(encoded", osmId.EncodedId(),
               ") from transit graph doesn't present by a single feature in mwm."));
  id.SetFeatureId(it->second[0]);
  id.SetOsmId(osmId.EncodedId());
}

void DeserializerFromJson::operator()(StopIdRanges & rs, char const * name)
{
  vector<StopId> stopIds;
  (*this)(stopIds, name);
  rs = StopIdRanges({stopIds});
}

// GraphData --------------------------------------------------------------------------------------
void GraphData::DeserializeFromJson(my::Json const & root, OsmIdToFeatureIdsMap const & mapping)
{
  DeserializeItemFromJson(root, "stops", mapping, m_stops);
  DeserializeItemFromJson(root, "gates", mapping, m_gates);
  DeserializeItemFromJson(root, "edges", mapping, m_edges);
  DeserializeItemFromJson(root, "transfers", mapping, m_transfers);
  DeserializeItemFromJson(root, "lines", mapping, m_lines);
  DeserializeItemFromJson(root, "shapes", mapping, m_shapes);
  DeserializeItemFromJson(root, "networks", mapping, m_networks);
}

void GraphData::SerializeToMwm(std::string const & mwmPath) const
{
  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(TRANSIT_FILE_TAG);

  TransitHeader header;

  auto const startOffset = w.Pos();
  Serializer<FileWriter> serializer(w);
  FixedSizeSerializer<FileWriter> numberSerializer(w);
  numberSerializer(header);
  header.m_stopsOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_stops);
  header.m_gatesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_gates);
  header.m_edgesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_edges);
  header.m_transfersOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_transfers);
  header.m_linesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_lines);
  header.m_shapesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_shapes);
  header.m_networksOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(m_networks);
  header.m_endOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  // Rewriting header info.
  CHECK(header.IsValid(), (header));
  auto const endOffset = w.Pos();
  w.Seek(startOffset);
  numberSerializer(header);
  w.Seek(endOffset);

  LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. Header:", header));
}

void GraphData::Append(GraphData const & rhs)
{
  m_stops.insert(m_stops.begin(), rhs.m_stops.begin(), rhs.m_stops.end());
  m_gates.insert(m_gates.begin(), rhs.m_gates.begin(), rhs.m_gates.end());
  m_edges.insert(m_edges.begin(), rhs.m_edges.begin(), rhs.m_edges.end());
  m_transfers.insert(m_transfers.begin(), rhs.m_transfers.begin(), rhs.m_transfers.end());
  m_lines.insert(m_lines.begin(), rhs.m_lines.begin(), rhs.m_lines.end());
  m_shapes.insert(m_shapes.begin(), rhs.m_shapes.begin(), rhs.m_shapes.end());
  m_networks.insert(m_networks.begin(), rhs.m_networks.begin(), rhs.m_networks.end());
}

void GraphData::Clear()
{
  m_stops.clear();
  m_gates.clear();
  m_edges.clear();
  m_transfers.clear();
  m_lines.clear();
  m_shapes.clear();
  m_networks.clear();
}

bool GraphData::IsValid() const
{
  return ::IsValid(m_stops) && ::IsValid(m_gates) && ::IsValid(m_edges) && ::IsValid(m_transfers) &&
      ::IsValid(m_lines) && ::IsValid(m_shapes) && ::IsValid(m_networks);
}

void GraphData::SortStops() { sort(m_stops.begin(), m_stops.end(), LessById); }

void GraphData::CalculateEdgeWeight()
{
  CHECK(is_sorted(m_stops.cbegin(), m_stops.cend(), LessById), ());

  for (auto & e : m_edges)
  {
    if (e.GetWeight() != kInvalidWeight)
      continue;

    Stop const & s1 = FindStopById(m_stops, e.GetStop1Id());
    Stop const & s2 = FindStopById(m_stops, e.GetStop2Id());
    double const lengthInMeters = MercatorBounds::DistanceOnEarth(s1.GetPoint(), s2.GetPoint());
    e.SetWeight(lengthInMeters / kTransitAverageSpeedMPS);
  }
}

void GraphData::CalculateBestPedestrianSegment(string const & mwmPath, string const & countryId)
{
  // Creating IndexRouter.
  SingleMwmIndex index(mwmPath);

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
  for (auto & gate : m_gates)
  {
    // Note. For pedestrian routing all the segments are considered as twoway segments so
    // IndexRouter.FindBestSegment() method finds the same segment for |isOutgoing| == true
    // and |isOutgoing| == false.
    Segment bestSegment;
    try
    {
      // @todo(bykoianko) For every call of the method below WorldGraph is created. It's not
      // efficient to create WorldGraph for every gate. The code should be redesigned.
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

void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping,
                         string const & transitJsonPath, GraphData & data)
{
  Platform::EFileType fileType;
  Platform::EError const errCode = Platform::GetFileType(transitJsonPath, fileType);
  CHECK_EQUAL(errCode, Platform::EError::ERR_OK, ("Transit graph not found:", transitJsonPath));
  CHECK_EQUAL(fileType, Platform::EFileType::FILE_TYPE_REGULAR,
              ("Transit graph not found:", transitJsonPath));

  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(transitJsonPath)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & ex)
  {
    LOG(LCRITICAL, ("Can't open", transitJsonPath, ex.what()));
  }

  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", transitJsonPath));

  data.Clear();
  data.DeserializeFromJson(root, mapping);
}

void ProcessGraph(string const & mwmPath, string const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data)
{
  data.CalculateBestPedestrianSegment(mwmPath, countryId);
  data.SortStops();
  data.CalculateEdgeWeight();
  CHECK(data.IsValid(), (mwmPath));
}

void ClipGraphByMwm(string const & mwmDir, string const & countryId, GraphData & data)
{
}

void BuildTransit(string const & mwmDir, string const & countryId,
                  string const & osmIdToFeatureIdsPath, string const & transitDir)
{
  LOG(LERROR, ("This method is under construction and should not be used for building production mwm "
      "sections."));
  NOTIMPLEMENTED();

  string const graphFullPath = my::JoinFoldersToPath(transitDir, countryId + TRANSIT_FILE_EXTENSION);

  std::string const mwmPath = GetMwmPath(mwmDir, countryId);
  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdsMap(osmIdToFeatureIdsPath, mapping);
  GraphData data;

  DeserializeFromJson(mapping, graphFullPath, data);
  ProcessGraph(mwmPath, countryId, mapping, data);
  ClipGraphByMwm(mwmDir, countryId, data);
  data.SerializeToMwm(mwmPath);
}
}  // namespace transit
}  // namespace routing
