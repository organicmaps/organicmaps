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
#include <unordered_map>
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

  template <typename Element>
  void ProcessNode(Element & em, std::vector<OsmElement::Tag> const & tags)
  {
    for (auto const & tag : tags)
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

  void ProcessWay(uint64_t id, WayElement const & way);

  void SaveIndex() { m_speedCameraNodeToWays.WriteAll(); }

private:
  /// \brief Gets text with speed, returns formated speed string in kmh.
  /// \param maxSpeedString - text with speed. Possible format:
  ///                         "130" - means 130 kmh.
  ///                         "130 mph" - means 130 mph.
  ///                         "130 kmh" - means 130 kmh.
  /// See https://wiki.openstreetmap.org/wiki/Key:maxspeed
  /// for more details about input string.
  std::string ValidateMaxSpeedString(std::string const & maxSpeedString);

  std::set<uint64_t> m_speedCameraNodes;
  generator::cache::IndexFileWriter m_speedCameraNodeToWays;
  FileWriter m_maxSpeedFileWriter;
};
}  // namespace generator
