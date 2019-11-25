#include "generator/collector_routing_city_boundaries.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/point_coding.cpp"

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
boost::optional<uint64_t> GetPlaceNodeFromMembers(OsmElement const & element)
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
  writer.Write(&n, sizeof(n));
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
  writer.Write(&localityData.m_population, sizeof(localityData.m_population));

  auto const placeType = static_cast<uint32_t>(localityData.m_place);
  writer.Write(&placeType, sizeof(placeType));

  auto const pointU = PointDToPointU(localityData.m_position, kPointCoordBits);
  writer.Write(&pointU.x, sizeof(pointU.x));
  writer.Write(&pointU.y, sizeof(pointU.y));
}

RoutingCityBoundariesCollector::LocalityData
RoutingCityBoundariesCollector::LocalityData::Deserialize(ReaderSource<FileReader> & reader)
{
  LocalityData localityData;
  reader.Read(&localityData.m_population, sizeof(localityData.m_population));

  uint32_t placeType = 0;
  reader.Read(&placeType, sizeof(placeType));
  localityData.m_place = static_cast<ftypes::LocalityType>(placeType);

  m2::PointU pointU;
  reader.Read(&pointU.x, sizeof(pointU.x));
  reader.Read(&pointU.y, sizeof(pointU.y));
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

  if (feature.IsArea() && IsSuitablePlaceType(GetPlaceType(feature)))
  {
    if (feature.PreSerialize())
      m_writer->Process(feature);
    return;
  }

  if (feature.IsArea())
  {
    auto const placeOsmIdOp = GetPlaceNodeFromMembers(osmElement);
    ASSERT(placeOsmIdOp, ("FilterOsmElement() should filtered such elements:", osmElement));

    auto const placeOsmId = *placeOsmIdOp;

    if (feature.PreSerialize())
      m_writer->Process(placeOsmId, feature);
    return;
  }
  else if (feature.IsPoint())
  {
    auto const placeType = GetPlaceType(feature);
    ASSERT(IsSuitablePlaceType(placeType),
           ("FilterOsmElement() should filtered such elements:", osmElement));

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
        ("Finish() has not been called."));
  base::AppendFileToFile(m_nodeOsmIdToLocalityDataFilename,
                         writer.m_nodeOsmIdToLocalityDataFilename);

  CHECK(!m_nodeOsmIdToBoundariesWriter || !writer.m_nodeOsmIdToBoundariesWriter,
        ("Finish() has not been called."));
  base::AppendFileToFile(m_nodeOsmIdToBoundariesFilename,
                         writer.m_nodeOsmIdToBoundariesFilename);

  CHECK(!m_finalBoundariesGeometryWriter || !writer.m_finalBoundariesGeometryWriter,
        ("Finish() has not been called."));
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
}  // namespace generator
