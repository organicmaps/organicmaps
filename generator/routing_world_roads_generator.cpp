#include "generator/routing_world_roads_generator.hpp"

#include "routing/cross_border_graph.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "coding/csv_reader.hpp"
#include "coding/files_container.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <utility>
#include <vector>

namespace routing
{
CrossBorderSegmentEnding ReadSegmentEnding(coding::CSVReader::Row const & tokens, size_t fromIndex)
{
  ms::LatLon latLon;
  latLon.m_lat = std::stod(tokens[fromIndex++]);
  latLon.m_lon = std::stod(tokens[fromIndex++]);

  auto const mwmId = static_cast<NumMwmId>(std::stol(tokens[fromIndex]));

  return CrossBorderSegmentEnding(latLon, mwmId);
}

std::pair<RegionSegmentId, CrossBorderSegment> ReadSegment(coding::CSVReader::Row const & tokens)
{
  CHECK_EQUAL(tokens.size(), 8, ());

  static size_t constexpr idxSegId = 0;
  static size_t constexpr idxSegWeight = 1;
  static size_t constexpr idxSegStart = 2;
  static size_t constexpr idxSegEnd = 5;

  auto const segId = static_cast<RegionSegmentId>(std::stol(tokens[idxSegId]));

  CrossBorderSegment seg;
  seg.m_weight = std::stod(tokens[idxSegWeight]);
  seg.m_start = ReadSegmentEnding(tokens, idxSegStart);
  seg.m_end = ReadSegmentEnding(tokens, idxSegEnd);

  return {segId, seg};
}

bool FillRoadsFromFile(CrossBorderGraph & graph, std::string const & roadsFilePath)
{
  try
  {
    auto reader = coding::CSVReader(roadsFilePath, false /* hasHeader */, ' ' /* delimiter */);

    reader.ForEachRow([&](auto const & row)
    {
      auto const & [segId, seg] = ReadSegment(row);
      graph.AddCrossBorderSegment(segId, seg);
    });

    return true;
  }
  catch (std::exception const & e)
  {
    LOG(LERROR, ("Exception while filling graph", e.what()));
  }

  return false;
}

bool BuildWorldRoads(std::string const & mwmFilePath, std::string const & roadsFilePath)
{
  CrossBorderGraph graph;

  bool const filledGraph = FillRoadsFromFile(graph, roadsFilePath);
  if (!filledGraph)
  {
    LOG(LERROR, ("Could not fill CrossBorderGraph from", roadsFilePath));
    return false;
  }

  CHECK(!graph.m_segments.empty(), ("Road segments for CrossBorderGraph should be set.", roadsFilePath));

  FilesContainerW cont(mwmFilePath, FileWriter::OP_WRITE_EXISTING);
  auto writer = cont.GetWriter(ROUTING_WORLD_FILE_TAG);

  /// @todo Default ctor loads countries.txt from data folder.
  /// But this is a "previous build" countries list!
  storage::Storage storage;
  std::shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);

  CrossBorderGraphSerializer::Serialize(graph, writer, numMwmIds);
  LOG(LINFO, ("Serialized", graph.m_segments.size(), "roads for", graph.m_mwms.size(), "regions."));

  return true;
}
}  // namespace routing
