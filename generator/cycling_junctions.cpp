#include "generator/cycling_junctions.hpp"

#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/osm_element.hpp"

#include "coding/file_reader.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/read_write_utils.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <set>
#include <unordered_set>

namespace generator
{
namespace cycling_junctions_detail
{
enum class RecordKind : uint8_t
{
  BlockedNode,
  Road,
  Connector
};
struct Record
{
  std::string m_country;
  RecordKind m_kind = RecordKind::Road;
  CyclingWay m_way;
};

bool Accessible(OsmElement const & element)
{
  auto const bicycle = element.GetTag("bicycle");
  if (bicycle == "no" || bicycle == "private" || bicycle == "dismount" || bicycle == "use_sidepath")
    return false;
  auto const access = element.GetTag("access");
  return (access != "no" && access != "private") || bicycle == "yes" || bicycle == "designated" ||
         bicycle == "official" || bicycle == "permissive";
}

bool Dedicated(OsmElement const & element)
{
  auto const highway = element.GetTag("highway");
  auto const bicycle = element.GetTag("bicycle");
  return highway == "cycleway" ||
         ((highway == "path" || highway == "footway") && (bicycle == "designated" || bicycle == "official"));
}

template <typename Sink>
void WriteRecord(Sink & sink, Record const & record)
{
  rw::Write(sink, record.m_country);
  WriteToSink(sink, record.m_kind);
  auto const & way = record.m_way;
  WriteToSink(sink, way.m_id);
  rw::Write(sink, way.m_layer);
  rw::Write(sink, way.m_level);
  rw::Write(sink, way.m_sides);
  WriteToSink(sink, uint8_t(way.m_reversed));
  WriteToSink(sink, static_cast<uint32_t>(way.m_nodes.size()));
  for (auto const & node : way.m_nodes)
  {
    WriteToSink(sink, node.m_id);
    WriteToSink(sink, std::bit_cast<uint64_t>(node.m_point.x));
    WriteToSink(sink, std::bit_cast<uint64_t>(node.m_point.y));
  }
}

template <typename Source>
Record ReadRecord(Source & source)
{
  Record record;
  rw::Read(source, record.m_country);
  record.m_kind = static_cast<RecordKind>(ReadPrimitiveFromSource<uint8_t>(source));
  CHECK_LESS_OR_EQUAL(static_cast<uint8_t>(record.m_kind), static_cast<uint8_t>(RecordKind::Connector), ());
  auto & way = record.m_way;
  way.m_id = ReadPrimitiveFromSource<uint64_t>(source);
  rw::Read(source, way.m_layer);
  rw::Read(source, way.m_level);
  rw::Read(source, way.m_sides);
  way.m_reversed = ReadPrimitiveFromSource<uint8_t>(source) != 0;
  auto const count = ReadPrimitiveFromSource<uint32_t>(source);
  CHECK_LESS_OR_EQUAL(count, source.Size() / 24, ());
  way.m_nodes.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    auto const id = ReadPrimitiveFromSource<uint64_t>(source);
    double const x = std::bit_cast<double>(ReadPrimitiveFromSource<uint64_t>(source));
    double const y = std::bit_cast<double>(ReadPrimitiveFromSource<uint64_t>(source));
    way.m_nodes.push_back({id, {x, y}});
  }
  return record;
}

std::string Grade(OsmElement const & element, char const * key)
{
  auto value = element.GetTag(key);
  return value.empty() ? "0" : value;
}
}  // namespace cycling_junctions_detail

std::map<uint64_t, cycling::Sides> FindCyclingJunctions(std::vector<CyclingWay> const & roads,
                                                        std::vector<CyclingWay> const & connectors,
                                                        std::function<bool(uint64_t)> const & blocked)
{
  struct Incident
  {
    CyclingWay const * m_way;
    m2::PointD m_point;
  };
  std::unordered_set<uint64_t> wanted;
  for (auto const & road : roads)
    for (auto const & node : road.m_nodes)
      wanted.insert(node.m_id);
  std::map<uint64_t, std::vector<Incident>> incidents;
  for (auto const & connector : connectors)
    for (size_t i = 0; i < connector.m_nodes.size(); ++i)
    {
      auto const id = connector.m_nodes[i].m_id;
      if (!wanted.contains(id))
        continue;
      if (i > 0)
        incidents[id].push_back({&connector, connector.m_nodes[i - 1].m_point});
      if (i + 1 < connector.m_nodes.size())
        incidents[id].push_back({&connector, connector.m_nodes[i + 1].m_point});
    }
  std::map<uint64_t, cycling::Sides> result;
  std::set<uint64_t> conflictingRoads;
  for (auto const & road : roads)
  {
    auto sides = cycling::Decode(road.m_sides);
    if (!sides || road.m_nodes.size() < 2)
      continue;
    sides->m_left.m_junctions.clear();
    sides->m_right.m_junctions.clear();
    std::set<std::pair<int, uint64_t>> seen;
    for (size_t i = 0; i < road.m_nodes.size(); ++i)
    {
      auto const & node = road.m_nodes[i];
      auto const found = incidents.find(node.m_id);
      if (found == incidents.end() || blocked(node.m_id))
        continue;
      auto const tangent =
          road.m_nodes[std::min(i + 1, road.m_nodes.size() - 1)].m_point - road.m_nodes[i == 0 ? 0 : i - 1].m_point;
      for (auto const & incident : found->second)
      {
        if (incident.m_way->m_id == road.m_id || incident.m_way->m_layer != road.m_layer ||
            incident.m_way->m_level != road.m_level)
          continue;
        auto const ray = incident.m_point - node.m_point;
        double const norm = tangent.Length() * ray.Length();
        double const cross = tangent.x * ray.y - tangent.y * ray.x;
        if (norm == 0 || std::abs(cross) / norm < 0.15)
          continue;
        int const sign = (cross > 0) != road.m_reversed ? -1 : 1;
        auto & side = sign < 0 ? sides->m_left : sides->m_right;
        if ((side.m_kind == cycling::Kind::Track || side.m_kind == cycling::Kind::Lane) &&
            side.m_junctions.size() < cycling::kMaxJunctionsPerSide && seen.emplace(sign, node.m_id).second)
          side.m_junctions.push_back(node.m_point);
      }
    }
    auto [entry, inserted] = result.emplace(road.m_id, *sides);
    if (!inserted && cycling::Encode(entry->second) != cycling::Encode(*sides))
      conflictingRoads.insert(road.m_id);
  }
  for (auto id : conflictingRoads)
    result.erase(id);
  return result;
}

CyclingJunctionCollector::CyclingJunctionCollector(std::string const & filename, AffiliationInterfacePtr affiliation)
  : CollectorInterface(filename)
  , m_affiliation(std::move(affiliation))
  , m_writer(std::make_unique<FileWriter>(GetTmpFilename()))
{}

std::shared_ptr<CollectorInterface> CyclingJunctionCollector::Clone(IDRInterfacePtr const &) const
{
  return std::make_shared<CyclingJunctionCollector>(GetFilename(), m_affiliation);
}

void CyclingJunctionCollector::Collect(OsmElement const & element)
{
  if (!element.IsNode())
    return;
  if (!cycling_junctions_detail::Accessible(element) ||
      std::any_of(element.m_tags.begin(), element.m_tags.end(),
                  [](auto const & tag) { return tag.m_key == "barrier"; }))
  {
    cycling_junctions_detail::Record record;
    record.m_kind = cycling_junctions_detail::RecordKind::BlockedNode;
    record.m_way.m_id = element.m_id;
    cycling_junctions_detail::WriteRecord(*m_writer, record);
  }
}

void CyclingJunctionCollector::CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & element)
{
  if (!element.IsWay() || !fb.IsLine() || !cycling_junctions_detail::Accessible(element) ||
      element.GetTag("area") == "yes")
    return;
  auto const highway = element.GetTag("highway");
  if (highway.empty() || highway == "construction" || highway == "proposed")
    return;
  cycling_junctions_detail::Record record;
  record.m_kind = cycling_junctions_detail::Dedicated(element) ? cycling_junctions_detail::RecordKind::Connector
                                                               : cycling_junctions_detail::RecordKind::Road;
  auto & way = record.m_way;
  way.m_sides = fb.GetParams().GetCyclingInfrastructure();
  if (record.m_kind == cycling_junctions_detail::RecordKind::Road && way.m_sides.empty())
    return;
  auto points = fb.GetOuterGeometry();
  way.m_reversed = fb.GetParams().GetReversedGeometry();
  if (way.m_reversed)
    std::reverse(points.begin(), points.end());
  if (points.size() != element.m_nodes.size())
    return;
  way.m_id = element.m_id;
  way.m_layer = cycling_junctions_detail::Grade(element, "layer");
  way.m_level = cycling_junctions_detail::Grade(element, "level");
  if (way.m_layer.size() > 64 || way.m_level.size() > 64)
    return;
  for (size_t i = 0; i < points.size(); ++i)
    way.m_nodes.push_back({element.m_nodes[i], points[i]});
  for (auto const & country : m_affiliation->GetAffiliations(fb))
  {
    record.m_country = country;
    cycling_junctions_detail::WriteRecord(*m_writer, record);
  }
}

void CyclingJunctionCollector::Finish()
{
  m_writer.reset();
}

void CyclingJunctionCollector::MergeInto(CyclingJunctionCollector & other) const
{
  CHECK(!m_writer && !other.m_writer, ("Finish() must precede merging"));
  base::AppendFileToFile(GetTmpFilename(), other.GetTmpFilename());
}

void CyclingJunctionCollector::Save()
{
  CHECK(!m_writer, ("Finish() must precede saving"));
  auto const directory = GetFilename() + ".regions";
  CHECK(Platform::MkDirChecked(directory), (directory));
  std::map<std::string, std::unique_ptr<FileWriter>> writers;
  std::set<std::string> countries;
  std::vector<uint64_t> blocked;
  FileReader input(GetTmpFilename());
  ReaderSource source(input);
  while (source.Size() != 0)
  {
    auto record = cycling_junctions_detail::ReadRecord(source);
    if (record.m_kind == cycling_junctions_detail::RecordKind::BlockedNode)
    {
      blocked.push_back(record.m_way.m_id);
      continue;
    }
    if (writers.size() >= 64 && !writers.contains(record.m_country))
      writers.clear();
    auto & writer = writers[record.m_country];
    if (!writer)
    {
      bool const first = countries.insert(record.m_country).second;
      writer = std::make_unique<FileWriter>(base::JoinPath(directory, record.m_country),
                                            first ? FileWriter::OP_WRITE_TRUNCATE : FileWriter::OP_APPEND);
    }
    cycling_junctions_detail::WriteRecord(*writer, record);
  }
  writers.clear();
  base::SortUnique(blocked);
  FileWriter manifest(GetFilename());
  WriteToSink(manifest, uint32_t(1));
  WriteToSink(manifest, static_cast<uint32_t>(countries.size()));
  for (auto const & country : countries)
    rw::Write(manifest, country);
  WriteToSink(manifest, static_cast<uint64_t>(blocked.size()));
  for (uint64_t id : blocked)
    WriteToSink(manifest, id);
}

void ApplyCountryCyclingJunctions(std::string const & filename, std::string const & country,
                                  std::string const & rawFeaturesPath)
{
  if (filename.empty() || !Platform::IsFileExistsByFullPath(filename))
    return;
  FileReader manifest(filename);
  ReaderSource source(manifest);
  CHECK_EQUAL(ReadPrimitiveFromSource<uint32_t>(source), 1, ());
  auto const countryCount = ReadPrimitiveFromSource<uint32_t>(source);
  bool present = false;
  for (uint32_t i = 0; i < countryCount; ++i)
  {
    std::string name;
    rw::Read(source, name);
    present |= name == country;
  }
  if (!present)
    return;  // The manifest also prevents reading stale shards from older runs.
  uint64_t const blockedCount = ReadPrimitiveFromSource<uint64_t>(source);
  uint64_t const blockedOffset = source.Pos();
  CHECK_LESS_OR_EQUAL(blockedCount, source.Size() / sizeof(uint64_t), ());
  auto const isBlocked = [&](uint64_t id)
  {
    uint64_t low = 0, high = blockedCount;
    while (low < high)
    {
      auto const mid = low + (high - low) / 2;
      ReaderSource row(manifest);
      row.Skip(blockedOffset + mid * sizeof(uint64_t));
      auto const candidate = ReadPrimitiveFromSource<uint64_t>(row);
      if (candidate < id)
        low = mid + 1;
      else if (candidate > id)
        high = mid;
      else
        return true;
    }
    return false;
  };
  FileReader shard(base::JoinPath(filename + ".regions", country));
  std::vector<CyclingWay> roads, connectors;
  ReaderSource rows(shard);
  while (rows.Size() != 0)
  {
    auto record = cycling_junctions_detail::ReadRecord(rows);
    CHECK_EQUAL(record.m_country, country, ());
    if (record.m_kind == cycling_junctions_detail::RecordKind::Road)
      roads.push_back(std::move(record.m_way));
    else if (record.m_kind == cycling_junctions_detail::RecordKind::Connector)
      connectors.push_back(std::move(record.m_way));
  }
  auto const junctions = FindCyclingJunctions(roads, connectors, isBlocked);
  feature::FeatureBuilderWriter<feature::serialization_policy::MaxAccuracy> writer(rawFeaturesPath, true);
  feature::ForEachFeatureRawFormat<feature::serialization_policy::MaxAccuracy>(
      rawFeaturesPath, [&](feature::FeatureBuilder && fb, uint64_t)
  {
    if (fb.IsLine() && fb.HasOsmIds() && !fb.GetParams().GetCyclingInfrastructure().empty())
    {
      auto const id = fb.GetMostGenericOsmId();
      if (id.GetType() == base::GeoObjectId::Type::OsmWay || id.GetType() == base::GeoObjectId::Type::ObsoleteOsmWay)
      {
        auto const found = junctions.find(id.GetSerialId());
        if (found != junctions.end())
          fb.GetParams().SetCyclingInfrastructure(cycling::Encode(found->second));
      }
    }
    writer.Write(fb);
  });
}
}  // namespace generator
