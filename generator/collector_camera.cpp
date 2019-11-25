#include "generator/collector_camera.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/maxspeeds_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/measurement_utils.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/point_coding.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/reader_writer_ops.hpp"

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

boost::optional<double> GetMaxSpeedKmPH(std::string const & maxSpeedString)
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

CameraProcessor::CameraProcessor(std::string const & filename)
  : m_waysFilename(filename + ".roads_ids"), m_waysWriter(std::make_unique<FileWriter>(m_waysFilename)) {}

CameraProcessor::~CameraProcessor()
{
  CHECK(Platform::RemoveFileIfExists(m_waysFilename), ());
}

void CameraProcessor::ForEachCamera(Fn && toDo) const
{
  std::vector<uint64_t> empty;
  for (auto const & p : m_speedCameras)
  {
    auto const & ways = m_cameraToWays.count(p.first) != 0 ? m_cameraToWays.at(p.first) : empty;
    toDo(p.second, ways);
  }
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

void CameraCollector::Write(FileWriter & writer, CameraProcessor::CameraInfo const & camera,
                            std::vector<uint64_t> const & ways)
{
  uint32_t const lat =
      DoubleToUint32(camera.m_lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  WriteToSink(writer, lat);

  uint32_t const lon =
      DoubleToUint32(camera.m_lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  WriteToSink(writer, lon);

  WriteToSink(writer, camera.m_speedKmPH);

  auto const size = static_cast<uint32_t>(ways.size());
  WriteToSink(writer, size);
  for (auto wayId : ways)
    WriteToSink(writer, wayId);
}

void CameraCollector::Finish()
{
  m_processor.Finish();
}

void CameraCollector::Save()
{
  using namespace std::placeholders;
  m_processor.FillCameraInWays();
  FileWriter writer(GetFilename());
  m_processor.ForEachCamera([&](auto const & camera, auto const & ways) {
    Write(writer, camera, ways);
  });
}

void CameraCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CameraCollector::MergeInto(CameraCollector & collector) const
{
  collector.m_processor.Merge(m_processor);
}
}  // namespace routing
