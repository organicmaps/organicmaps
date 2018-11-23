#include "generator/camera_node_processor.hpp"

#include "generator/maxspeeds_parser.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <array>

namespace generator
{
size_t const CameraNodeIntermediateDataProcessor::kMaxSpeedSpeedStringLength = 32;
}  // namespace generator

namespace routing
{
void CameraNodeProcessor::Open(std::string const & writerFile, std::string const & readerFile,
                               std::string const & speedFile)
{
  m_fileWriter = std::make_unique<FileWriter>(writerFile);
  m_cameraNodeToWays = std::make_unique<Cache>(readerFile);
  m_cameraNodeToWays->ReadAll();

  FileReader maxSpeedReader(speedFile);
  ReaderSource<FileReader> src(maxSpeedReader);

  static auto constexpr kMaxSpeedSpeedStringLength =
    generator::CameraNodeIntermediateDataProcessor::kMaxSpeedSpeedStringLength;
  std::array<char, kMaxSpeedSpeedStringLength> buffer{};
  uint64_t nodeOsmId = 0;
  size_t maxSpeedStringLength = 0;
  while (src.Size() > 0)
  {
    ReadPrimitiveFromSource(src, nodeOsmId);

    ReadPrimitiveFromSource(src, maxSpeedStringLength);
    CHECK_LESS(maxSpeedStringLength, kMaxSpeedSpeedStringLength, ("Too long maxspeed string"));

    src.Read(buffer.data(), maxSpeedStringLength);
    buffer[maxSpeedStringLength] = '\0';

    m_cameraToMaxSpeed[nodeOsmId] = buffer.data();
  }
}

void CameraNodeProcessor::Process(OsmElement & p, FeatureParams const & params,
                                  generator::cache::IntermediateDataReader & cache)
{
  if (!(p.type == OsmElement::EntityType::Node && ftypes::IsSpeedCamChecker::Instance()(params.m_types)))
    return;

  std::string maxSpeedStringKmPH = "0";
  auto const it = m_cameraToMaxSpeed.find(p.id);
  if (it != m_cameraToMaxSpeed.cend())
    maxSpeedStringKmPH = it->second;

  int32_t maxSpeedKmPH = 0;
  if (!strings::to_int(maxSpeedStringKmPH.c_str(), maxSpeedKmPH))
    LOG(LWARNING, ("Bad speed format of camera:", maxSpeedStringKmPH, ", osmId:", p.id));

  CHECK_GREATER_OR_EQUAL(maxSpeedKmPH, 0, ());

  uint32_t const lat =
      DoubleToUint32(p.lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  WriteToSink(*m_fileWriter, lat);

  uint32_t const lon =
      DoubleToUint32(p.lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  WriteToSink(*m_fileWriter, lon);

  WriteToSink(*m_fileWriter, static_cast<uint32_t>(maxSpeedKmPH));

  std::vector<uint64_t> ways;
  ForEachWayByNode(p.id, [&ways](uint64_t wayId)
  {
    ways.push_back(wayId);
    return base::ControlFlow::Continue;
  });

  auto const size = static_cast<uint32_t>(ways.size());
  WriteToSink(*m_fileWriter, size);
  for (auto wayId : ways)
    WriteToSink(*m_fileWriter, wayId);
}
}  // namespace routing

namespace generator
{
CameraNodeIntermediateDataProcessor::CameraNodeIntermediateDataProcessor(std::string const & nodesFile,
                                                                         std::string const & speedFile)
  : m_speedCameraNodeToWays(nodesFile),
    m_maxSpeedFileWriter(speedFile)
{
  LOG(LINFO, ("Saving intermediate data about cameras to:", nodesFile,
              ", about maxspeed:", speedFile));
}

void CameraNodeIntermediateDataProcessor::ProcessWay(uint64_t id, WayElement const & way)
{
  std::vector<uint64_t> nodes;
  for (auto const node : way.nodes)
  {
    if (m_speedCameraNodes.find(node) != m_speedCameraNodes.end())
      nodes.push_back(node);
  }

  if (!nodes.empty())
    generator::cache::IntermediateDataWriter::AddToIndex(m_speedCameraNodeToWays, id, nodes);
}

std::string CameraNodeIntermediateDataProcessor::ValidateMaxSpeedString(std::string const & maxSpeedString)
{
  routing::SpeedInUnits speed;
  if (!ParseMaxspeedTag(maxSpeedString, speed) || !speed.IsNumeric())
    return std::string();

  return strings::to_string(measurement_utils::ToSpeedKmPH(speed.GetSpeed(), speed.GetUnits()));
}

void CameraNodeIntermediateDataProcessor::ProcessNode(OsmElement & em)
{
  for (auto const & tag : em.Tags())
  {
    std::string const & key(tag.key);
    std::string const & value(tag.value);
    if (key == "highway" && value == "speed_camera")
    {
      m_speedCameraNodes.insert(em.id);
    }
    else if (key == "maxspeed" && !value.empty())
    {
      WriteToSink(m_maxSpeedFileWriter, em.id);

      std::string result = ValidateMaxSpeedString(value);
      CHECK_LESS(result.size(), kMaxSpeedSpeedStringLength, ("Too long string for speed"));
      WriteToSink(m_maxSpeedFileWriter, result.size());
      m_maxSpeedFileWriter.Write(result.c_str(), result.size());
    }
  }
}
}  // namespace generator
