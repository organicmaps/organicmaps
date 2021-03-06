#include "generator/collector_routing_city_boundaries.hpp"

#include "generator/final_processor_utils.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/point_coding.hpp"

#include "geometry/area_on_earth.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;
using namespace feature::serialization_policy;

namespace
{
std::optional<uint64_t> GetPlaceNodeFromMembers(OsmElement const & element)
{
  uint64_t adminCentreRef = 0;
  uint64_t labelRef = 0;
  for (auto const & member : element.m_members)
  {
    if (member.m_type == OsmElement::EntityType::Node)
    {
      if (member.m_role == "admin_centre")
        adminCentreRef = member.m_ref;
      else if (member.m_role == "label")
        labelRef = member.m_ref;
    }
  }

  if (labelRef)
    return labelRef;

  if (adminCentreRef)
    return adminCentreRef;

  return {};
}

bool IsSuitablePlaceType(ftypes::LocalityType localityType)
{
  switch (localityType)
  {
  case ftypes::LocalityType::City:
  case ftypes::LocalityType::Town:
  case ftypes::LocalityType::Village: return true;
  default: return false;
  }
}

ftypes::LocalityType GetPlaceType(FeatureBuilder const & feature)
{
  return ftypes::IsLocalityChecker::Instance().GetType(feature.GetTypesHolder());
}

void TruncateAndWriteCount(std::string const & file, size_t n)
{
  FileWriter writer(file, FileWriter::Op::OP_WRITE_TRUNCATE);
  WriteToSink(writer, n);
}
}  // namespace

namespace generator
{
// RoutingCityBoundariesCollector::LocalityData ----------------------------------------------------

// static
bool RoutingCityBoundariesCollector::FilterOsmElement(OsmElement const & osmElement)
{
  static std::set<std::string> const kSuitablePlaceValues = {"city", "town", "village", "hamlet"};
  if (osmElement.IsNode())
  {
    if (!osmElement.HasTag("population"))
      return false;

    auto const place = osmElement.GetTag("place");
    if (place.empty())
      return false;

    return kSuitablePlaceValues.count(place) != 0;
  }

  auto const place = osmElement.GetTag("place");
  if (!place.empty() && kSuitablePlaceValues.count(place) != 0)
    return true;

  if (osmElement.IsWay())
    return false;

  // We don't check here "type=boundary" and "boundary=administrative" because some OSM'ers prefer
  // map boundaries without such tags in some countries (for example in Russia).
  for (auto const & member : osmElement.m_members)
  {
    if (member.m_role == "admin_centre" || member.m_role == "label")
      return true;
  }
  return false;
}

void RoutingCityBoundariesCollector::LocalityData::Serialize(FileWriter & writer,
                                                             LocalityData const & localityData)
{
  WriteToSink(writer, localityData.m_population);

  auto const placeType = static_cast<uint32_t>(localityData.m_place);
  WriteToSink(writer, placeType);

  auto const pointU = PointDToPointU(localityData.m_position, kPointCoordBits);
  WriteToSink(writer, pointU.x);
  WriteToSink(writer, pointU.y);
}

RoutingCityBoundariesCollector::LocalityData
RoutingCityBoundariesCollector::LocalityData::Deserialize(ReaderSource<FileReader> & reader)
{
  LocalityData localityData;
  ReadPrimitiveFromSource(reader, localityData.m_population);

  uint32_t placeType = 0;
  ReadPrimitiveFromSource(reader, placeType);
  localityData.m_place = static_cast<ftypes::LocalityType>(placeType);

  m2::PointU pointU;
  ReadPrimitiveFromSource(reader, pointU.x);
  ReadPrimitiveFromSource(reader, pointU.y);
  localityData.m_position = PointUToPointD(pointU, kPointCoordBits);

  return localityData;
}

// RoutingCityBoundariesCollector ------------------------------------------------------------------

RoutingCityBoundariesCollector::RoutingCityBoundariesCollector(
    std::string const & filename, std::string const & dumpFilename,
    std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache)
  : CollectorInterface(filename)
  , m_writer(std::make_unique<RoutingCityBoundariesWriter>(GetTmpFilename()))
  , m_cache(cache)
  , m_featureMakerSimple(cache)
  , m_dumpFilename(dumpFilename)
{
}

std::shared_ptr<CollectorInterface> RoutingCityBoundariesCollector::Clone(
    std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache) const
{
  return std::make_shared<RoutingCityBoundariesCollector>(GetFilename(), m_dumpFilename, 
                                                          cache ? cache : m_cache);
}

void RoutingCityBoundariesCollector::Collect(OsmElement const & osmElement)
{
  if (!FilterOsmElement(osmElement))
    return;

  auto osmElementCopy = osmElement;
  feature::FeatureBuilder feature;
  m_featureMakerSimple.Add(osmElementCopy);

  while (m_featureMakerSimple.GetNextFeature(feature))
  {
    if (feature.IsValid())
      Process(feature, osmElementCopy);
  }
}

void RoutingCityBoundariesCollector::Process(feature::FeatureBuilder & feature,
                                             OsmElement const & osmElement)
{
  ASSERT(FilterOsmElement(osmElement), ());

  if (feature.IsArea() && IsSuitablePlaceType(::GetPlaceType(feature)))
  {
    if (feature.PreSerialize())
      m_writer->Process(feature);
    return;
  }

  if (feature.IsArea())
  {
    auto const placeOsmIdOp = GetPlaceNodeFromMembers(osmElement);
    // Elements which have multiple place tags i.e. "place=country" + "place=city" and do not have place node
    // will pass FilterOsmElement() but can have bad placeType for previous case (IsArea && IsSuitablePlaceType)
    // and no place node for this case. As we do not know what's the real place type we skip such places.
    if (!placeOsmIdOp)
    {
      LOG(LWARNING, ("Have multiple place tags for", osmElement));
      return;
    }

    auto const placeOsmId = *placeOsmIdOp;

    if (feature.PreSerialize())
      m_writer->Process(placeOsmId, feature);
    return;
  }
  else if (feature.IsPoint())
  {
    auto const placeType = ::GetPlaceType(feature);

    // Elements which have multiple place tags i.e. "place=country" + "place=city" will pass FilterOsmElement()
    // but can have bad placeType here. As we do not know what's the real place type let's skip such places.
    if (!IsSuitablePlaceType(placeType))
    {
      LOG(LWARNING, ("Have multiple place tags for", osmElement));
      return;
    }

    uint64_t const population = osm_element::GetPopulation(osmElement);
    if (population == 0)
      return;

    uint64_t nodeOsmId = osmElement.m_id;
    m2::PointD const center = mercator::FromLatLon(osmElement.m_lat, osmElement.m_lon);
    m_writer->Process(nodeOsmId, LocalityData(population, placeType, center));
  }
}

void RoutingCityBoundariesCollector::Finish() { m_writer->Reset(); }

void RoutingCityBoundariesCollector::Save()
{
  m_writer->Save(GetFilename(), m_dumpFilename);
}

void RoutingCityBoundariesCollector::OrderCollectedData()
{
  m_writer->OrderCollectedData(GetFilename(), m_dumpFilename);
}

void RoutingCityBoundariesCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void RoutingCityBoundariesCollector::MergeInto(RoutingCityBoundariesCollector & collector) const
{
  m_writer->MergeInto(*collector.m_writer);
}

// RoutingCityBoundariesWriter ---------------------------------------------------------------------

// static
std::string RoutingCityBoundariesWriter::GetNodeToLocalityDataFilename(std::string const & filename)
{
  return filename + ".nodeId2locality";
}

// static
std::string RoutingCityBoundariesWriter::GetNodeToBoundariesFilename(std::string const & filename)
{
  return filename + ".nodeId2Boundaries";
}

// static
std::string RoutingCityBoundariesWriter::GetBoundariesFilename(std::string const & filename)
{
  return filename + ".boundaries";
}

RoutingCityBoundariesWriter::RoutingCityBoundariesWriter(std::string const & filename)
  : m_nodeOsmIdToLocalityDataFilename(GetNodeToLocalityDataFilename(filename))
  , m_nodeOsmIdToBoundariesFilename(GetNodeToBoundariesFilename(filename))
  , m_finalBoundariesGeometryFilename(GetBoundariesFilename(filename))
  , m_nodeOsmIdToLocalityDataWriter(std::make_unique<FileWriter>(m_nodeOsmIdToLocalityDataFilename))
  , m_nodeOsmIdToBoundariesWriter(std::make_unique<FileWriter>(m_nodeOsmIdToBoundariesFilename))
  , m_finalBoundariesGeometryWriter(std::make_unique<FileWriter>(m_finalBoundariesGeometryFilename))
{
}

RoutingCityBoundariesWriter::~RoutingCityBoundariesWriter()
{
  CHECK(Platform::RemoveFileIfExists(m_nodeOsmIdToLocalityDataFilename), (m_nodeOsmIdToLocalityDataFilename));
  CHECK(Platform::RemoveFileIfExists(m_nodeOsmIdToBoundariesFilename), (m_nodeOsmIdToBoundariesFilename));
  CHECK(Platform::RemoveFileIfExists(m_finalBoundariesGeometryFilename), (m_finalBoundariesGeometryFilename));
}

void RoutingCityBoundariesWriter::Process(uint64_t nodeOsmId, LocalityData const & localityData)
{
  m_nodeOsmIdToLocalityDataWriter->Write(&nodeOsmId, sizeof(nodeOsmId));
  LocalityData::Serialize(*m_nodeOsmIdToLocalityDataWriter, localityData);

  ++m_nodeOsmIdToLocalityDataCount;
}

void RoutingCityBoundariesWriter::Process(uint64_t nodeOsmId,
                                          feature::FeatureBuilder const & feature)
{
  m_nodeOsmIdToBoundariesWriter->Write(&nodeOsmId, sizeof(nodeOsmId));
  FeatureWriter::Write(*m_nodeOsmIdToBoundariesWriter, feature);

  ++m_nodeOsmIdToBoundariesCount;
}

void RoutingCityBoundariesWriter::Process(feature::FeatureBuilder const & feature)
{
  rw::WriteVectorOfPOD(*m_finalBoundariesGeometryWriter, feature.GetOuterGeometry());
}

void RoutingCityBoundariesWriter::Reset()
{
  m_nodeOsmIdToLocalityDataWriter.reset();
  m_nodeOsmIdToBoundariesWriter.reset();
  m_finalBoundariesGeometryWriter.reset();
}

void RoutingCityBoundariesWriter::MergeInto(RoutingCityBoundariesWriter & writer)
{
  CHECK(!m_nodeOsmIdToLocalityDataWriter || !writer.m_nodeOsmIdToLocalityDataWriter,
        ("Reset() has not been called."));
  base::AppendFileToFile(m_nodeOsmIdToLocalityDataFilename,
                         writer.m_nodeOsmIdToLocalityDataFilename);

  CHECK(!m_nodeOsmIdToBoundariesWriter || !writer.m_nodeOsmIdToBoundariesWriter,
        ("Reset() has not been called."));
  base::AppendFileToFile(m_nodeOsmIdToBoundariesFilename,
                         writer.m_nodeOsmIdToBoundariesFilename);

  CHECK(!m_finalBoundariesGeometryWriter || !writer.m_finalBoundariesGeometryWriter,
        ("Reset() has not been called."));
  base::AppendFileToFile(m_finalBoundariesGeometryFilename,
                         writer.m_finalBoundariesGeometryFilename);

  writer.m_nodeOsmIdToLocalityDataCount += m_nodeOsmIdToLocalityDataCount;
  writer.m_nodeOsmIdToBoundariesCount += m_nodeOsmIdToBoundariesCount;
}

void RoutingCityBoundariesWriter::Save(std::string const & finalFileName,
                                       std::string const & dumpFilename)
{
  auto const nodeToLocalityFilename = GetNodeToLocalityDataFilename(finalFileName);
  auto const nodeToBoundariesFilename = GetNodeToBoundariesFilename(finalFileName);

  TruncateAndWriteCount(nodeToLocalityFilename, m_nodeOsmIdToLocalityDataCount);
  TruncateAndWriteCount(nodeToBoundariesFilename, m_nodeOsmIdToBoundariesCount);

  base::AppendFileToFile(m_nodeOsmIdToLocalityDataFilename, nodeToLocalityFilename);
  base::AppendFileToFile(m_nodeOsmIdToBoundariesFilename, nodeToBoundariesFilename);

  if (Platform::IsFileExistsByFullPath(m_finalBoundariesGeometryFilename))
    CHECK(base::CopyFileX(m_finalBoundariesGeometryFilename, dumpFilename), ());
}

void RoutingCityBoundariesWriter::OrderCollectedData(std::string const & finalFileName,
                                                     std::string const & dumpFilename)
{
  {
    auto const nodeToLocalityFilename = GetNodeToLocalityDataFilename(finalFileName);
    std::vector<std::pair<uint64_t, LocalityData>> collectedData;
    uint64_t count = 0;
    {
      FileReader reader(nodeToLocalityFilename);
      ReaderSource src(reader);
      ReadPrimitiveFromSource(src, count);
      collectedData.reserve(count);
      while (src.Size() > 0)
      {
        collectedData.push_back({});
        ReadPrimitiveFromSource(src, collectedData.back().first);
        collectedData.back().second = LocalityData::Deserialize(src);
      }
      CHECK_EQUAL(collectedData.size(), count, ());
    }
    std::sort(std::begin(collectedData), std::end(collectedData));
    FileWriter writer(nodeToLocalityFilename);
    WriteToSink(writer, count);
    for (auto const & p : collectedData)
    {
      WriteToSink(writer, p.first);
      LocalityData::Serialize(writer, p.second);
    }
  }
  {
    auto const nodeToBoundariesFilename = GetNodeToBoundariesFilename(finalFileName);
    std::vector<std::pair<uint64_t, FeatureBuilder>> collectedData;
    uint64_t count = 0;
    {
      FileReader reader(nodeToBoundariesFilename);
      ReaderSource src(reader);
      ReadPrimitiveFromSource(src, count);
      collectedData.reserve(count);
      while (src.Size() > 0)
      {
        collectedData.push_back({});
        ReadPrimitiveFromSource(src, collectedData.back().first);
        ReadFromSourceRawFormat(src, collectedData.back().second);
      }
      CHECK_EQUAL(collectedData.size(), count, ());
    }
    std::sort(
        std::begin(collectedData), std::end(collectedData), [](auto const & lhs, auto const & rhs) {
          return lhs.first == rhs.first ? Less(lhs.second, rhs.second) : lhs.first < rhs.first;
        });
    FileWriter writer(nodeToBoundariesFilename);
    WriteToSink(writer, count);
    for (auto const & p : collectedData)
    {
      WriteToSink(writer, p.first);
      FeatureWriter::Write(writer, p.second);
    }
  }
  {
    std::vector<FeatureBuilder::PointSeq> collectedData;
    {
      FileReader reader(dumpFilename);
      ReaderSource src(reader);
      while (src.Size() > 0)
      {
        collectedData.push_back({});
        rw::ReadVectorOfPOD(src, collectedData.back());
      }
    }
    std::sort(std::begin(collectedData), std::end(collectedData));
    FileWriter writer(dumpFilename);
    for (auto const & p : collectedData)
      rw::WriteVectorOfPOD(writer, p);
  }
}
}  // namespace generator
