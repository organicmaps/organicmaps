#include "generator/metalines_builder.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/routing_helpers.hpp"

#include "indexer/classificator.hpp"

#include "coding/files_container.hpp"
#include "coding/read_write_utils.hpp"
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
  while (TryMergeOne(lineString, buffer))
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
    std::sort(std::begin(lineStrings), std::end(lineStrings), [](auto const & l, auto const & r)
    {
      auto const & lways = l->GetWays();
      auto const & rways = r->GetWays();
      return lways.size() == rways.size() ? lways.front() < rways.front() : lways.size() > rways.size();
    });
  }

  return intermediateData;
}

// MetalinesBuilder --------------------------------------------------------------------------------
MetalinesBuilder::MetalinesBuilder(std::string const & filename)
  : generator::CollectorInterface(filename)
  , m_writer(std::make_unique<FileWriter>(GetTmpFilename()))
{}

std::shared_ptr<generator::CollectorInterface> MetalinesBuilder::Clone(IDRInterfacePtr const &) const
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

  WriteVarUint(*m_writer, static_cast<uint64_t>(std::hash<std::string>{}(std::string(name) + '\0' + params.ref)));
  LineString(element).Serialize(*m_writer);
}

void MetalinesBuilder::Finish()
{
  m_writer.reset();
}

void MetalinesBuilder::Save()
{
  std::unordered_multimap<size_t, std::shared_ptr<LineString>> keyToLineString;
  FileReader reader(GetTmpFilename());
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    auto const key = ReadVarUint<uint64_t>(src);
    keyToLineString.emplace(key, std::make_shared<LineString>(LineString::Deserialize(src)));
  }

  FileWriter writer(GetFilename());
  uint32_t countLines = 0;
  uint32_t countWays = 0;
  auto const mergedData = LineStringMerger::Merge(keyToLineString);
  for (auto const & p : mergedData)
  {
    for (auto const & lineString : p.second)
    {
      auto const & ways = lineString->GetWays();
      rw::WriteVectorOfPOD(writer, ways);
      countWays += ways.size();
      ++countLines;
    }
  }

  LOG_SHORT(LINFO, ("Wrote", countLines, "metalines [with", countWays, "ways] with OSM IDs for the entire planet to",
                    GetFilename()));
}

void MetalinesBuilder::OrderCollectedData()
{
  std::vector<LineString::Ways> collectedData;
  {
    FileReader reader(GetFilename());
    ReaderSource src(reader);
    while (src.Size() > 0)
    {
      collectedData.push_back({});
      rw::ReadVectorOfPOD(src, collectedData.back());
    }
  }
  std::sort(std::begin(collectedData), std::end(collectedData));
  FileWriter writer(GetFilename());
  for (auto const & ways : collectedData)
    rw::WriteVectorOfPOD(writer, ways);
}

void MetalinesBuilder::MergeInto(MetalinesBuilder & collector) const
{
  CHECK(!m_writer || !collector.m_writer, ("Finish() has not been called."));
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}

// Functions --------------------------------------------------------------------------------
bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath)
{
  routing::OsmIdToFeatureIds osmIdToFeatureIds;
  routing::ParseWaysOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, osmIdToFeatureIds);

  FileReader reader(metalinesPath);
  ReaderSource<FileReader> src(reader);
  std::vector<uint8_t> buffer;
  MemWriter<std::vector<uint8_t>> memWriter(buffer);
  uint32_t count = 0;

  while (src.Size() > 0)
  {
    std::vector<int32_t> featureIds;
    std::vector<int32_t> ways;
    rw::ReadVectorOfPOD(src, ways);
    for (auto const wayId : ways)
    {
      // We get a negative wayId when a feature direction should be reversed.
      auto fids = osmIdToFeatureIds.find(base::MakeOsmWay(std::abs(wayId)));
      if (fids == osmIdToFeatureIds.cend())
        break;

      // Keeping the sign for the feature direction.
      // @TODO(bykoianko) All the feature ids should be used instead of |fids->second[0]|.
      CHECK(!fids->second.empty(), ());
      int32_t const featureId = static_cast<int32_t>(fids->second.front());
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
  auto writer = cont.GetWriter(METALINES_FILE_TAG);
  WriteToSink(writer, kMetaLinesSectionVersion);
  WriteVarUint(writer, count);
  writer->Write(buffer.data(), buffer.size());
  LOG(LINFO, ("Finished writing metalines section, found", count, "metalines."));
  return true;
}
}  // namespace feature
