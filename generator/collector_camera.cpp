#include "generator/collector_camera.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/maxspeeds_parser.hpp"
#include "generator/osm_element.hpp"

#include "routing/routing_helpers.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/reader_writer_ops.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;

namespace routing
{
size_t const CameraProcessor::kMaxSpeedSpeedStringLength = 32;

std::optional<double> GetMaxSpeedKmPH(std::string const & maxSpeedString)
{
  routing::SpeedInUnits speed;
  if (!generator::ParseMaxspeedTag(maxSpeedString, speed) || !speed.IsNumeric())
    return {};

  auto const speedKmPh = measurement_utils::ToSpeedKmPH(speed.GetSpeed(), speed.GetUnits());
  if (speedKmPh < 0.0)
    return {};

  return {speedKmPh};
}

CameraProcessor::CameraInfo::CameraInfo(OsmElement const & element)
  : m_id(element.m_id)
  , m_lon(element.m_lon)
  , m_lat(element.m_lat)
{
  auto const maxspeed = element.GetTag("maxspeed");
  if (maxspeed.empty() || maxspeed.size() > kMaxSpeedSpeedStringLength)
    return;

  if (auto const validatedMaxspeed = GetMaxSpeedKmPH(maxspeed))
    m_speedKmPH = static_cast<uint32_t>(*validatedMaxspeed);
  else
    LOG(LWARNING, ("Bad speed format of camera:", maxspeed, ", osmId:", element.m_id));
}

void CameraProcessor::CameraInfo::Normalize() { std::sort(std::begin(m_ways), std::end(m_ways)); }

// static
CameraProcessor::CameraInfo CameraProcessor::CameraInfo::Read(ReaderSource<FileReader> & src)
{
  CameraInfo camera;
  auto const latInt = ReadPrimitiveFromSource<uint32_t>(src);
  auto const lonInt = ReadPrimitiveFromSource<uint32_t>(src);
  camera.m_lat = Uint32ToDouble(latInt, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  camera.m_lon = Uint32ToDouble(lonInt, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  ReadPrimitiveFromSource(src, camera.m_speedKmPH);
  auto relatedWaysNumber = ReadPrimitiveFromSource<uint32_t>(src);
  camera.m_ways.reserve(relatedWaysNumber);
  while (relatedWaysNumber--)
    camera.m_ways.push_back(ReadPrimitiveFromSource<uint64_t>(src));

  return camera;
}

// static
void CameraProcessor::CameraInfo::Write(FileWriter & writer, CameraInfo const & camera)
{
  uint32_t const lat =
      DoubleToUint32(camera.m_lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  WriteToSink(writer, lat);

  uint32_t const lon =
      DoubleToUint32(camera.m_lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  WriteToSink(writer, lon);

  WriteToSink(writer, camera.m_speedKmPH);

  auto const size = static_cast<uint32_t>(camera.m_ways.size());
  WriteToSink(writer, size);
  for (auto wayId : camera.m_ways)
    WriteToSink(writer, wayId);
}

CameraProcessor::CameraProcessor(std::string const & filename)
  : m_waysFilename(filename + ".roads_ids"), m_waysWriter(std::make_unique<FileWriter>(m_waysFilename)) {}

CameraProcessor::~CameraProcessor()
{
  CHECK(Platform::RemoveFileIfExists(m_waysFilename), ());
}

void CameraProcessor::ProcessWay(OsmElement const & element)
{
  WriteToSink(*m_waysWriter, element.m_id);
  rw::WriteVectorOfPOD(*m_waysWriter, element.m_nodes);
}

void CameraProcessor::FillCameraInWays()
{
  FileReader reader(m_waysFilename);
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    uint64_t wayId = ReadPrimitiveFromSource<uint64_t>(src);
    std::vector<uint64_t> nodes;
    rw::ReadVectorOfPOD(src, nodes);
    for (auto const & node : nodes)
    {
      auto const itCamera = m_speedCameras.find(node);
      if (itCamera == m_speedCameras.cend())
        continue;

      m_cameraToWays[itCamera->first].push_back(wayId);
    }
  }

  std::vector<uint64_t> empty;
  for (auto & p : m_speedCameras)
    p.second.m_ways = m_cameraToWays.count(p.first) != 0 ? m_cameraToWays.at(p.first) : empty;

  ForEachCamera([](auto & c) { c.Normalize(); });
}

void CameraProcessor::ProcessNode(OsmElement const & element)
{
  CameraInfo camera(element);
  m_speedCameras.emplace(element.m_id, std::move(camera));
}

void CameraProcessor::Finish()
{
  m_waysWriter.reset({});
}

void CameraProcessor::Merge(CameraProcessor const & cameraProcessor)
{
  auto const & otherCameras = cameraProcessor.m_speedCameras;
  m_speedCameras.insert(std::begin(otherCameras), std::end(otherCameras));

  base::AppendFileToFile(cameraProcessor.m_waysFilename, m_waysFilename);
}

void CameraProcessor::Save(std::string const & filename)
{
  FillCameraInWays();
  FileWriter writer(filename);
  ForEachCamera([&](auto const & camera) { CameraInfo::Write(writer, camera); });
}

void CameraProcessor::OrderCollectedData(std::string const & filename)
{
  std::vector<CameraProcessor::CameraInfo> collectedData;
  {
    FileReader reader(filename);
    ReaderSource src(reader);
    while (src.Size() > 0)
      collectedData.emplace_back(CameraInfo::Read(src));
  }
  std::sort(std::begin(collectedData), std::end(collectedData));
  FileWriter writer(filename);
  for (auto const & camera : collectedData)
    CameraInfo::Write(writer, camera);
}

CameraCollector::CameraCollector(std::string const & filename) :
  generator::CollectorInterface(filename), m_processor(GetTmpFilename()) {}

std::shared_ptr<generator::CollectorInterface> CameraCollector::Clone(
    std::shared_ptr<generator::cache::IntermediateDataReaderInterface> const &) const
{
  return std::make_shared<CameraCollector>(GetFilename());
}

void CameraCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const & element)
{
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node:
  {
    if (ftypes::IsSpeedCamChecker::Instance()(feature.GetTypes()))
      m_processor.ProcessNode(element);
    break;
  }
  case OsmElement::EntityType::Way:
  {
    if (routing::IsCarRoad(feature.GetTypes()))
      m_processor.ProcessWay(element);
    break;
  }
  default:
    break;
  }
}

void CameraCollector::Finish()
{
  m_processor.Finish();
}

void CameraCollector::Save() { m_processor.Save(GetFilename()); }

void CameraCollector::OrderCollectedData() { m_processor.OrderCollectedData(GetFilename()); }

void CameraCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CameraCollector::MergeInto(CameraCollector & collector) const
{
  collector.m_processor.Merge(m_processor);
}
}  // namespace routing
