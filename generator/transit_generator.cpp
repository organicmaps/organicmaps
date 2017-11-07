#include "generator/transit_generator.hpp"

#include "generator/utils.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/index_router.hpp"
#include "routing/routing_exceptions.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_speed_limits.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/index.hpp"

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
template <class Item>
bool IsValid(vector<Item> const & items)
{
  return all_of(items.cbegin(), items.cend(), [](Item const & item) { return item.IsValid(); });
}

struct ClearVisitor
{
  template <typename Cont>
  void operator()(Cont & c, char const * /* name */) const { c.clear(); }
};

struct SortVisitor
{
  template <typename Cont>
  void operator()(Cont & c, char const * /* name */) const
  {
    sort(c.begin(), c.end());
  }
};

struct IsValidVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * /* name */ )
  {
    m_isValid = m_isValid && ::IsValid(c);
  }

  bool IsValid() const { return m_isValid; }

private:
  bool m_isValid = true;
};

struct IsUniqueVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * /* name */)
  {
    m_isUnique = m_isUnique && (adjacent_find(c.cbegin(), c.cend()) == c.cend());
  }

  bool IsUnique() const { return m_isUnique; }

private:
  bool m_isUnique = true;
};

struct IsSortedVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * /* name */)
  {
    m_isSorted = m_isSorted && is_sorted(c.cbegin(), c.cend());
  }

  bool IsSorted() const { return m_isSorted; }

private:
  bool m_isSorted = true;
};

/// \returns ref to a stop at |stops| by |stopId|.
Stop const & FindStopById(vector<Stop> const & stops, StopId stopId)
{
  auto s1Id = equal_range(stops.cbegin(), stops.cend(),
                          Stop(stopId, kInvalidOsmId, kInvalidFeatureId, kInvalidTransferId,
                               {} /* line ids */, {} /* point */, {} /* title anchors */));
  CHECK(s1Id.first != stops.cend(), ("No stop with id:", stopId, "in stops:", stops));
  CHECK_EQUAL(distance(s1Id.first, s1Id.second), 1, ("A stop with id:", stopId, "is not unique in stops:", stops));
  return *s1Id.first;
}

void FillOsmIdToFeatureIdsMap(string const & osmIdToFeatureIdsPath, OsmIdToFeatureIdsMap & map)
{
  CHECK(ForEachOsmId2FeatureId(osmIdToFeatureIdsPath,
                               [&map](osm::Id const & osmId, uint32_t featureId) {
                                 map[osmId].push_back(featureId);
                               }),
        (osmIdToFeatureIdsPath));
}

string GetMwmPath(string const & mwmDir, string const & countryId)
{
  return my::JoinPath(mwmDir, countryId + DATA_FILE_EXTENSION);
}

template <typename T>
void Append(vector<T> const & src, vector<T> & dst)
{
  dst.insert(dst.end(), src.begin(), src.end());
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
  if (it != m_osmIdToFeatureIds.cend())
  {
    CHECK_EQUAL(it->second.size(), 1, ("Osm id:", osmId, "(encoded", osmId.EncodedId(),
                 ") from transit graph corresponds to", it->second.size(), "features."
                 "But osm id should be represented be one feature."));
    id.SetFeatureId(it->second[0]);
  }
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
  DeserializerFromJson deserializer(root.get(), mapping);
  Visit(deserializer);
}

void GraphData::SerializeToMwm(string const & mwmPath) const
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

void GraphData::AppendTo(GraphData const & rhs)
{
  ::Append(rhs.m_stops, m_stops);
  ::Append(rhs.m_gates, m_gates);
  ::Append(rhs.m_edges, m_edges);
  ::Append(rhs.m_transfers, m_transfers);
  ::Append(rhs.m_lines, m_lines);
  ::Append(rhs.m_shapes, m_shapes);
  ::Append(rhs.m_networks, m_networks);
}

void GraphData::Clear()
{
  ClearVisitor const v{};
  Visit(v);
}

bool GraphData::IsValid() const
{
  if (!IsSorted() || !IsUnique())
    return false;

  IsValidVisitor v;
  Visit(v);
  return v.IsValid();
}

void GraphData::Sort()
{
  SortVisitor const v{};
  Visit(v);
}

void GraphData::CalculateEdgeWeights()
{
  CHECK(is_sorted(m_stops.cbegin(), m_stops.cend()), ());
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

void GraphData::CalculateBestPedestrianSegments(string const & mwmPath, string const & countryId)
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
  auto worldGraph = indexRouter.MakeSingleMwmWorldGraph();

  // Looking for the best segment for every gate.
  for (auto & gate : m_gates)
  {
    // Note. For pedestrian routing all the segments are considered as twoway segments so
    // IndexRouter.FindBestSegment() method finds the same segment for |isOutgoing| == true
    // and |isOutgoing| == false.
    Segment bestSegment;
    try
    {
      if (countryFileGetter(gate.GetPoint()) != countryId)
        continue;
      if (indexRouter.FindBestSegment(gate.GetPoint(), m2::PointD::Zero() /* direction */,
                                      true /* isOutgoing */, *worldGraph, bestSegment))
      {
        CHECK_EQUAL(bestSegment.GetMwmId(), 0, ());
        gate.SetBestPedestrianSegment(SingleMwmSegment(
            bestSegment.GetFeatureId(), bestSegment.GetSegmentIdx(), bestSegment.IsForward()));
      }
    }
    catch (MwmIsNotAliveException const & e)
    {
      LOG(LCRITICAL, ("Point of a gate belongs to the processed mwm:", countryId, ","
          "but the mwm is not alive. Gate:", gate, e.what()));
    }
    catch (RootException const & e)
    {
      LOG(LCRITICAL, ("Exception while looking for the best segment of a gate. CountryId:",
          countryId, ". Gate:", gate, e.what()));
    }
  }
}

bool GraphData::IsUnique() const
{
  IsUniqueVisitor v;
  Visit(v);
  return v.IsUnique();
}

bool GraphData::IsSorted() const
{
  IsSortedVisitor v;
  Visit(v);
  return v.IsSorted();
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
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't open", transitJsonPath, e.what()));
  }

  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", transitJsonPath));

  data.Clear();
  try
  {
    data.DeserializeFromJson(root, mapping);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Exception while parsing transit graph json. Json file path:", transitJsonPath, e.what()));
  }
}

void ProcessGraph(string const & mwmPath, string const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data)
{
  data.CalculateBestPedestrianSegments(mwmPath, countryId);
  data.Sort();
  data.CalculateEdgeWeights();
  CHECK(data.IsValid(), (mwmPath));
}

void BuildTransit(string const & mwmDir, string const & countryId,
                  string const & osmIdToFeatureIdsPath, string const & transitDir)
{
  // @todo(bykoianko) It's assumed that the method builds transit section based on clipped json.
  // Json clipping should be implemented at the feature generation step.

  LOG(LERROR, ("This method is under construction and should not be used for building production mwm "
      "sections."));
  NOTIMPLEMENTED();

  string const graphFullPath = my::JoinPath(transitDir, countryId + TRANSIT_FILE_EXTENSION);
  string const mwmPath = GetMwmPath(mwmDir, countryId);
  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdsMap(osmIdToFeatureIdsPath, mapping);

  GraphData data;
  DeserializeFromJson(mapping, graphFullPath, data);
  ProcessGraph(mwmPath, countryId, mapping, data);
  data.SerializeToMwm(mwmPath);
}
}  // namespace transit
}  // namespace routing
