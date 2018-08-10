#include "generator/metalines_builder.hpp"

#include "generator/routing_helpers.hpp"

#include "indexer/classificator.hpp"

#include "coding/file_container.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <cstdint>
#include <map>

namespace
{
uint8_t const kMetaLinesSectionVersion = 1;

using Ways = std::vector<int32_t>;

/// A string of connected ways.
class LineString
{
  Ways m_ways;
  uint64_t m_start;
  uint64_t m_end;
  bool m_oneway;

public:
  explicit LineString(OsmElement const & way)
  {
    std::string const oneway = way.GetTag("oneway");
    m_oneway = !oneway.empty() && oneway != "no";
    int32_t const wayId = base::checked_cast<int32_t>(way.id);
    m_ways.push_back(oneway == "-1" ? -wayId : wayId);
    CHECK_GREATER_OR_EQUAL(way.Nodes().size(), 2, ());
    m_start = way.Nodes().front();
    m_end = way.Nodes().back();
  }

  Ways const & GetWays() const { return m_ways; }

  void Reverse()
  {
    ASSERT(!m_oneway, ("Trying to reverse a one-way road."));
    std::swap(m_start, m_end);
    std::reverse(m_ways.begin(), m_ways.end());
    for (auto & p : m_ways)
      p = -p;
  }

  bool Add(LineString & line)
  {
    if (m_start == line.m_start || m_end == line.m_end)
    {
      if (!line.m_oneway)
        line.Reverse();
      else if (!m_oneway)
        Reverse();
      else
        return false;
    }
    if (m_end == line.m_start)
    {
      m_ways.insert(m_ways.end(), line.m_ways.begin(), line.m_ways.end());
      m_end = line.m_end;
      m_oneway = m_oneway || line.m_oneway;
    }
    else if (m_start == line.m_end)
    {
      m_ways.insert(m_ways.begin(), line.m_ways.begin(), line.m_ways.end());
      m_start = line.m_start;
      m_oneway = m_oneway || line.m_oneway;
    }
    else
    {
      return false;
    }
    return true;
  }
};
}  // namespace

namespace feature
{
/// A list of segments, that is, LineStrings, sharing the same attributes.
class Segments
{
  std::list<LineString> m_parts;

public:
  explicit Segments(OsmElement const & way) { m_parts.emplace_back(way); }

  void Add(OsmElement const & way)
  {
    LineString line(way);
    auto found = m_parts.end();
    for (auto i = m_parts.begin(); i != m_parts.end(); ++i)
    {
      if (i->Add(line))
      {
        found = i;
        break;
      }
    }
    // If no LineString accepted the way in its Add method, create a new LineString with it.
    if (found == m_parts.cend())
    {
      m_parts.push_back(line);
      return;
    }
    // Otherwise check if the extended LineString can be merged with some other LineString.
    for (LineString & part : m_parts)
    {
      if (part.Add(*found))
      {
        m_parts.erase(found);
        break;
      }
    }
  }

  std::vector<Ways> GetLongWays() const
  {
    std::vector<Ways> result;
    for (LineString const & line : m_parts)
    {
      if (line.GetWays().size() > 1)
      {
        result.push_back(line.GetWays());
      }
    }
    return result;
  }
};

// MetalinesBuilder --------------------------------------------------------------------------------
void MetalinesBuilder::operator()(OsmElement const & el, FeatureParams const & params)
{
  static uint32_t const highwayType = classif().GetTypeByPath({"highway"});
  if (params.FindType(highwayType, 1) == ftype::GetEmptyValue() ||
      el.Nodes().front() == el.Nodes().back())
  {
    return;
  }

  std::string name;
  params.name.GetString(StringUtf8Multilang::kDefaultCode, name);
  if (name.empty() && params.ref.empty())
    return;

  size_t const key = std::hash<std::string>{}(name + '\0' + params.ref);
  auto segment = m_data.find(key);
  if (segment == m_data.cend())
    m_data.emplace(key, std::make_shared<Segments>(el));
  else
    segment->second->Add(el);
}

void MetalinesBuilder::Flush()
{
  try
  {
    uint32_t count = 0;
    FileWriter writer(m_filePath);
    for (auto const & seg : m_data)
    {
      auto const & longWays = seg.second->GetLongWays();
      for (Ways const & ways : longWays)
      {
        uint16_t size = base::checked_cast<uint16_t>(ways.size());
        WriteToSink(writer, size);
        for (int32_t const way : ways)
          WriteToSink(writer, way);
        ++count;
      }
    }
    LOG_SHORT(LINFO, ("Wrote", count, "metalines with OSM IDs for the entire planet to", m_filePath));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while saving metalines to", m_filePath, ":", e.what()));
  }
}

// Functions --------------------------------------------------------------------------------
bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath)
{
  std::map<base::GeoObjectId, uint32_t> osmIdToFeatureId;
  if (!routing::ParseOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, osmIdToFeatureId))
    return false;

  FileReader reader(metalinesPath);
  ReaderSource<FileReader> src(reader);
  std::vector<uint8_t> buffer;
  MemWriter<std::vector<uint8_t>> memWriter(buffer);
  uint32_t count = 0;

  while (src.Size() > 0)
  {
    std::vector<int32_t> featureIds;
    uint16_t size = ReadPrimitiveFromSource<uint16_t>(src);
    std::vector<int32_t> ways(size);
    src.Read(ways.data(), size * sizeof(int32_t));
    for (auto const wayId : ways)
    {
      // We get a negative wayId when a feature direction should be reversed.
      auto fid = osmIdToFeatureId.find(base::MakeOsmWay(std::abs(wayId)));
      if (fid == osmIdToFeatureId.cend())
        break;

      // Keeping the sign for the feature direction.
      int32_t const featureId = static_cast<int32_t>(fid->second);
      featureIds.push_back(wayId > 0 ? featureId : -featureId);
    }

    if (featureIds.size() > 1)
    {
      // Write vector to buffer if we have got at least two segments.
      WriteVarUint(memWriter, featureIds.size());
      for (auto const & fid : featureIds)
        WriteVarInt(memWriter, fid);
      ++count;
    }
  }

  // Write buffer to section.
  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(METALINES_FILE_TAG);
  WriteToSink(writer, kMetaLinesSectionVersion);
  WriteVarUint(writer, count);
  writer.Write(buffer.data(), buffer.size());
  LOG(LINFO, ("Finished writing metalines section, found", count, "metalines."));
  return true;
}
}  // namespace feature
