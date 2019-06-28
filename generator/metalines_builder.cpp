#include "generator/metalines_builder.hpp"

#include "generator/intermediate_data.hpp"
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
#include <limits>
#include <map>

namespace
{
uint8_t constexpr kMetaLinesSectionVersion = 1;
}  // namespace

namespace feature
{
LineString::LineString(OsmElement const & way)
{
  std::string const oneway = way.GetTag("oneway");
  m_oneway = !oneway.empty() && oneway != "no";
  int32_t const wayId = base::checked_cast<int32_t>(way.m_id);
  m_ways.push_back(oneway == "-1" ? -wayId : wayId);
  CHECK_GREATER_OR_EQUAL(way.Nodes().size(), 2, ());
  m_start = way.Nodes().front();
  m_end = way.Nodes().back();
}

void LineString::Reverse()
{
  ASSERT(!m_oneway, ("Trying to reverse a one-way road."));
  std::swap(m_start, m_end);
  std::reverse(m_ways.begin(), m_ways.end());
  for (auto & p : m_ways)
    p = -p;
}

bool LineString::Add(LineString & line)
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

// static
LineStringMerger::OutputData LineStringMerger::Merge(InputData const & data)
{
  InputData mergedLines;
  auto const intermediateData = OrderData(data);
  for (auto & p : intermediateData)
  {
    Buffer buffer;
    for (auto & lineString : p.second)
      TryMerge(lineString, buffer);

    std::unordered_set<LinePtr> uniqLineStrings;
    for (auto const & pb : buffer)
    {
      auto const & ways = pb.second->GetWays();
      if (uniqLineStrings.emplace(pb.second).second && ways.size() > 1)
        mergedLines.emplace(p.first, pb.second);
    }
  }

  return OrderData(mergedLines);
}

// static
bool LineStringMerger::TryMerge(LinePtr const & lineString, Buffer & buffer)
{
  bool merged = false;
  while(TryMergeOne(lineString, buffer))
    merged = true;

  buffer.emplace(lineString->GetStart(), lineString);
  buffer.emplace(lineString->GetEnd(), lineString);
  return merged;
}

// static
bool LineStringMerger::TryMergeOne(LinePtr const & lineString, Buffer & buffer)
{
  auto static const kUndef = std::numeric_limits<int>::max();
  uint64_t index = kUndef;
  if (buffer.count(lineString->GetStart()) != 0)
    index = lineString->GetStart();
  else if (buffer.count(lineString->GetEnd()) != 0)
    index = lineString->GetEnd();

  if (index != kUndef)
  {
    auto bufferedLineString = buffer[index];
    buffer.erase(bufferedLineString->GetStart());
    buffer.erase(bufferedLineString->GetEnd());
    if (!lineString->Add(*bufferedLineString))
    {
      buffer.emplace(bufferedLineString->GetStart(), bufferedLineString);
      buffer.emplace(bufferedLineString->GetEnd(), bufferedLineString);

      buffer.emplace(lineString->GetStart(), lineString);
      buffer.emplace(lineString->GetEnd(), lineString);
      return false;
    }
  }

  return index != kUndef;
}

// static
LineStringMerger::OutputData LineStringMerger::OrderData(InputData const & data)
{
  OutputData intermediateData;
  for (auto const & p : data)
    intermediateData[p.first].emplace_back(p.second);

  for (auto & p : intermediateData)
  {
    auto & lineStrings = intermediateData[p.first];
    std::sort(std::begin(lineStrings), std::end(lineStrings), [](auto const & l, auto const & r) {
      auto const & lways = l->GetWays();
      auto const & rways = r->GetWays();
      return lways.size() == rways.size() ? lways.front() < rways.front() : lways.size() > rways.size();
    });
  }

  return intermediateData;
}

// MetalinesBuilder --------------------------------------------------------------------------------
MetalinesBuilder::MetalinesBuilder(std::string const & filename)
  : generator::CollectorInterface(filename) {}

std::shared_ptr<generator::CollectorInterface>
MetalinesBuilder::Clone(std::shared_ptr<generator::cache::IntermediateDataReader> const &) const
{
  return std::make_shared<MetalinesBuilder>(GetFilename());
}

void MetalinesBuilder::CollectFeature(FeatureBuilder const & feature, OsmElement const & element)
{
  if (!feature.IsLine())
    return;

  static auto const highwayType = classif().GetTypeByPath({"highway"});
  if (!feature.HasType(highwayType, 1 /* level */) || element.Nodes().front() == element.Nodes().back())
    return;

  auto const & params = feature.GetParams();
  auto const name = feature.GetName();
  if (name.empty() && params.ref.empty())
    return;

  auto const key = std::hash<std::string>{}(name + '\0' + params.ref);
  m_data.emplace(key, std::make_shared<LineString>(element));
}

void MetalinesBuilder::Save()
{
  FileWriter writer(GetFilename());
  uint32_t countLines = 0;
  uint32_t countWays = 0;
  auto const mergedData = LineStringMerger::Merge(m_data);
  for (auto const & p : mergedData)
  {
    for (auto const & lineString : p.second)
    {
      auto const & ways = lineString->GetWays();
      uint16_t size = base::checked_cast<uint16_t>(ways.size());
      WriteToSink(writer, size);
      countWays += ways.size();
      for (int32_t const way : ways)
        WriteToSink(writer, way);
      ++countLines;
    }
  }

  LOG_SHORT(LINFO, ("Wrote", countLines, "metalines [with",  countWays ,
                    "ways] with OSM IDs for the entire planet to", GetFilename()));
}

void MetalinesBuilder::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void MetalinesBuilder::MergeInto(MetalinesBuilder & collector) const
{
  collector.m_data.insert(std::begin(m_data), std::end(m_data));
}

// Functions --------------------------------------------------------------------------------
bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath)
{
  std::map<base::GeoObjectId, uint32_t> osmIdToFeatureId;
  if (!routing::ParseRoadsOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, osmIdToFeatureId))
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
