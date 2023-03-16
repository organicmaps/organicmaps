#include "generator/collector_mini_roundabout.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"

#include "routing/routing_helpers.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/reader_writer_ops.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <iterator>

using namespace feature;

namespace generator
{
MiniRoundaboutProcessor::MiniRoundaboutProcessor(std::string const & filename)
  : m_waysFilename(filename + MINI_ROUNDABOUT_ROADS_EXTENSION)
  , m_waysWriter(std::make_unique<FileWriter>(m_waysFilename))
{
}

MiniRoundaboutProcessor::~MiniRoundaboutProcessor()
{
  CHECK(Platform::RemoveFileIfExists(m_waysFilename), ());
}

void MiniRoundaboutProcessor::ProcessWay(OsmElement const & element)
{
  WriteToSink(*m_waysWriter, element.m_id);
  rw::WriteVectorOfPOD(*m_waysWriter, element.m_nodes);
}

void MiniRoundaboutProcessor::FillMiniRoundaboutsInWays()
{
  FileReader reader(m_waysFilename);
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    uint64_t const wayId = ReadPrimitiveFromSource<uint64_t>(src);
    std::vector<uint64_t> nodes;
    rw::ReadVectorOfPOD(src, nodes);
    for (auto const & node : nodes)
    {
      auto const itMiniRoundabout = m_miniRoundabouts.find(node);
      if (itMiniRoundabout != m_miniRoundabouts.end())
        itMiniRoundabout->second.m_ways.push_back(wayId);
    }
  }

  ForEachMiniRoundabout([](auto & rb) { rb.Normalize(); });
}

void MiniRoundaboutProcessor::ProcessNode(OsmElement const & element)
{
  m_miniRoundabouts.emplace(element.m_id, MiniRoundaboutInfo(element));
}

void MiniRoundaboutProcessor::ProcessRestriction(uint64_t osmId)
{
  m_miniRoundaboutsExceptions.insert(osmId);
}

void MiniRoundaboutProcessor::Finish() { m_waysWriter.reset(); }

void MiniRoundaboutProcessor::Merge(MiniRoundaboutProcessor const & miniRoundaboutProcessor)
{
  auto const & otherMiniRoundabouts = miniRoundaboutProcessor.m_miniRoundabouts;
  m_miniRoundabouts.insert(otherMiniRoundabouts.begin(), otherMiniRoundabouts.end());

  auto const & otherMiniRoundaboutsExceptions = miniRoundaboutProcessor.m_miniRoundaboutsExceptions;
  m_miniRoundaboutsExceptions.insert(otherMiniRoundaboutsExceptions.begin(),
                                     otherMiniRoundaboutsExceptions.end());

  base::AppendFileToFile(miniRoundaboutProcessor.m_waysFilename, m_waysFilename);
}

// MiniRoundaboutCollector -------------------------------------------------------------------------

MiniRoundaboutCollector::MiniRoundaboutCollector(std::string const & filename)
  : generator::CollectorInterface(filename), m_processor(GetTmpFilename())
{
}

std::shared_ptr<generator::CollectorInterface> MiniRoundaboutCollector::Clone(
    std::shared_ptr<generator::cache::IntermediateDataReaderInterface> const &) const
{
  return std::make_shared<MiniRoundaboutCollector>(GetFilename());
}

void MiniRoundaboutCollector::Collect(OsmElement const & element)
{
  if (element.IsNode() && element.HasTag("highway", "mini_roundabout"))
  {
    m_processor.ProcessNode(element);
    return;
  }

  // Skip mini_roundabouts with role="via" in restrictions
  if (element.IsRelation())
  {
    for (auto const & member : element.m_members)
    {
      if (member.m_type == OsmElement::EntityType::Node && member.m_role == "via")
      {
        m_processor.ProcessRestriction(member.m_ref);
        return;
      }
    }
  }
}

void MiniRoundaboutCollector::CollectFeature(FeatureBuilder const & feature,
                                             OsmElement const & element)
{
  if (feature.IsLine() && routing::IsCarRoad(feature.GetTypes()))
    m_processor.ProcessWay(element);
}

void MiniRoundaboutCollector::Finish() { m_processor.Finish(); }

void MiniRoundaboutCollector::Save()
{
  m_processor.FillMiniRoundaboutsInWays();
  FileWriter writer(GetFilename());
  m_processor.ForEachMiniRoundabout(
      [&](auto const & miniRoundabout) { WriteMiniRoundabout(writer, miniRoundabout); });
}

void MiniRoundaboutCollector::OrderCollectedData()
{
  auto collectedData = ReadMiniRoundabouts(GetFilename());
  std::sort(std::begin(collectedData), std::end(collectedData));
  FileWriter writer(GetFilename());
  for (auto const & miniRoundabout : collectedData)
    WriteMiniRoundabout(writer, miniRoundabout);
}

void MiniRoundaboutCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void MiniRoundaboutCollector::MergeInto(MiniRoundaboutCollector & collector) const
{
  collector.m_processor.Merge(m_processor);
}
}  // namespace generator
