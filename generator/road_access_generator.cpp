#include "generator/road_access_generator.hpp"

#include "generator/osm_element.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/initializer_list.hpp"

#include "defines.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <string>

using namespace std;

namespace
{
char const kAccessPrivate[] = "access=private";
char const kBarrierGate[] = "barrier=gate";
char const kDelim[] = " \t\r\n";
}  // namespace

namespace routing
{
// RoadAccessWriter ------------------------------------------------------------

void RoadAccessWriter::Open(string const & filePath)
{
  LOG(LINFO,
      ("Saving information about barriers and road access classes in osm id terms to", filePath));
  m_stream.open(filePath, ofstream::out);

  if (!IsOpened())
    LOG(LINFO, ("Cannot open file", filePath));
}

void RoadAccessWriter::Process(OsmElement * p, FeatureParams & params)
{
  if (!IsOpened())
  {
    LOG(LWARNING, ("Tried to write to a closed barriers writer"));
    return;
  }

  auto const & c = classif();

  StringIL const forbiddenRoadTypes[] = {
    {"hwtag", "private"}
  };

  for (auto const & f : forbiddenRoadTypes)
  {
    auto const t = c.GetTypeByPath(f);
    if (params.IsTypeExist(t) && p->type == OsmElement::EntityType::Way)
      m_stream << kAccessPrivate << " " << p->id << "\n";
  }

  auto t = c.GetTypeByPath({"barrier", "gate"});
  if (params.IsTypeExist(t))
    m_stream << kBarrierGate << " " << p->id << "\n";
}

bool RoadAccessWriter::IsOpened() { return m_stream.is_open() && !m_stream.fail(); }
// RoadAccessCollector ----------------------------------------------------------

RoadAccessCollector::RoadAccessCollector(string const & roadAccessPath,
                                         string const & osmIdsToFeatureIdsPath)
{
  MY_SCOPE_GUARD(clean, [this]() {
    m_osmIdToFeatureId.clear();
    m_roadAccess.Clear();
  });

  m_valid = true;

  if (!ParseOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, m_osmIdToFeatureId))
  {
    LOG(LWARNING, ("An error happened while parsing feature id to osm ids mapping from file:",
                   osmIdsToFeatureIdsPath));
    m_valid = false;
    return;
  }

  if (!ParseRoadAccess(roadAccessPath))
  {
    LOG(LWARNING, ("An error happened while parsing road access from file:", roadAccessPath));
    m_valid = false;
    return;
  }

  clean.release();
}

bool RoadAccessCollector::ParseRoadAccess(string const & roadAccessPath)
{
  ifstream stream(roadAccessPath);
  if (stream.fail())
    return false;

  string line;
  while (getline(stream, line))
  {
    strings::SimpleTokenizer iter(line, kDelim);

    // Empty line.
    if (!iter)
      return false;

    string const s = *iter;
    ++iter;

    uint64_t osmId;
    if (!iter || !strings::to_uint64(*iter, osmId))
      return false;

    auto const it = m_osmIdToFeatureId.find(osmId);
    if (it == m_osmIdToFeatureId.cend())
      return false;

    uint32_t const featureId = it->second;
    m_roadAccess.GetPrivateRoads().emplace_back(featureId);
  }

  return true;
}

// Functions ------------------------------------------------------------------

void BuildRoadAccessInfo(string const & dataFilePath, string const & roadAccessPath,
                         string const & osmIdsToFeatureIdsPath)
{
  LOG(LINFO, ("Generating road access info for", dataFilePath));

  RoadAccessCollector collector(roadAccessPath, osmIdsToFeatureIdsPath);

  if (!collector.IsValid())
  {
    LOG(LWARNING, ("Unable to parse road access in osm terms"));
    return;
  }

  FilesContainerW cont(dataFilePath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(ROAD_ACCESS_FILE_TAG);

  RoadAccessSerializer serializer;
  serializer.Serialize(writer, collector.GetRoadAccess());
}
}  // namespace routing
