#pragma once

#include "generator/osm_element.hpp"
#include "generator/intermediate_data.hpp"

#include "routing/base/followed_polyline.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/write_to_sink.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// TODO (@gmoryes) move members of m_routingTagsProcessor to generator
namespace routing
{
class CameraNodeProcessor
{
public:
  void Open(std::string const & writerFile, std::string const & readerFile,
            std::string const & speedFile);

  template <typename ToDo>
  void ForEachWayByNode(uint64_t id, ToDo && toDo)
  {
    m_cameraNodeToWays->ForEachByKey(id, std::forward<ToDo>(toDo));
  }

  void Process(OsmElement & p, FeatureParams const & params,
               generator::cache::IntermediateDataReader & cache);

private:
  using Cache = generator::cache::IndexFileReader;

  std::unique_ptr<FileWriter> m_fileWriter;
  std::unique_ptr<Cache> m_cameraNodeToWays;
  std::map<uint64_t, std::string> m_cameraToMaxSpeed;
};
}  // namespace routing

namespace generator
{
class CameraNodeIntermediateDataProcessor
{
public:
  explicit CameraNodeIntermediateDataProcessor(std::string const & nodesFile, std::string const & speedFile);

  static size_t const kMaxSpeedSpeedStringLength;

  void ProcessNode(OsmElement & em);

  void ProcessWay(uint64_t id, WayElement const & way);

  void SaveIndex() { m_speedCameraNodeToWays.WriteAll(); }

private:
  /// \brief Gets text with speed, returns formatted speed string in km per hour.
  /// \param maxSpeedString - text with speed. Possible format:
  ///                         "130" - means 130 km per hour.
  ///                         "130 mph" - means 130 miles per hour.
  ///                         "130 kmh" - means 130 km per hour.
  /// See https://wiki.openstreetmap.org/wiki/Key:maxspeed
  /// for more details about input string.
  std::string ValidateMaxSpeedString(std::string const & maxSpeedString);

  std::set<uint64_t> m_speedCameraNodes;
  generator::cache::IndexFileWriter m_speedCameraNodeToWays;
  FileWriter m_maxSpeedFileWriter;
};
}  // namespace generator
