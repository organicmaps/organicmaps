#include "generator/collector_building_parts.hpp"

#include "generator/feature_builder.hpp"
#include "generator/gen_mwm_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "coding/file_reader.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/checked_cast.hpp"
#include "base/control_flow.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace
{
using IdRelationVec = std::vector<std::pair<uint64_t, RelationElement>>;

class RelationFetcher
{
public:
  RelationFetcher(IdRelationVec & elements) : m_elements(elements) {}

  void operator()(uint64_t id, generator::cache::OSMElementCacheReaderInterface & reader)
  {
    RelationElement element;
    if (reader.Read(id, element))
      m_elements.emplace_back(id, std::move(element));
    else
      LOG(LWARNING, ("Cannot read relation with id", id));
  }

private:
  IdRelationVec & m_elements;
};
}  // namespace

namespace generator
{
// static
void BuildingPartsCollector::BuildingParts::Write(FileWriter & writer, BuildingParts const & pb)
{
  WriteToSink(writer, pb.m_id.m_mainId.GetEncodedId());
  WriteToSink(writer, pb.m_id.m_additionalId.GetEncodedId());
  auto const contSize = base::checked_cast<uint32_t>(pb.m_buildingParts.size());
  WriteVarUint(writer, contSize);
  for (auto const & id : pb.m_buildingParts)
    WriteToSink(writer, id.GetEncodedId());
}

// static
BuildingPartsCollector::BuildingParts BuildingPartsCollector::BuildingParts::Read(ReaderSource<FileReader> & src)
{
  BuildingParts bp;
  auto const first = base::GeoObjectId(ReadPrimitiveFromSource<uint64_t>(src));
  auto const second = base::GeoObjectId(ReadPrimitiveFromSource<uint64_t>(src));
  bp.m_id = CompositeId(first, second);
  auto contSize = ReadVarUint<uint32_t>(src);
  bp.m_buildingParts.reserve(contSize);
  while (contSize--)
    bp.m_buildingParts.emplace_back(base::GeoObjectId(ReadPrimitiveFromSource<uint64_t>(src)));

  return bp;
}

BuildingPartsCollector::BuildingPartsCollector(std::string const & filename, IDRInterfacePtr const & cache)
  : CollectorInterface(filename)
  , m_cache(cache)
  , m_writer(std::make_unique<FileWriter>(GetTmpFilename()))
{}

std::shared_ptr<CollectorInterface> BuildingPartsCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<BuildingPartsCollector>(GetFilename(), cache ? cache : m_cache);
}

void BuildingPartsCollector::CollectFeature(feature::FeatureBuilder const & fb, OsmElement const &)
{
  static auto const & buildingChecker = ftypes::IsBuildingChecker::Instance();
  if (!fb.IsArea() || !buildingChecker(fb.GetTypes()))
    return;

  auto const topId = FindTopRelation(fb.GetMostGenericOsmId());
  if (topId == base::GeoObjectId())
    return;

  auto parts = FindAllBuildingParts(topId);
  if (!parts.empty())
  {
    BuildingParts bp;
    bp.m_id = MakeCompositeId(fb);
    bp.m_buildingParts = std::move(parts);

    BuildingParts::Write(*m_writer, bp);
  }
}

std::vector<base::GeoObjectId> BuildingPartsCollector::FindAllBuildingParts(base::GeoObjectId const & id)
{
  std::vector<base::GeoObjectId> buildingParts;
  RelationElement relation;
  if (!m_cache->GetRelation(id.GetSerialId(), relation))
  {
    LOG(LWARNING, ("Cannot read relation with id", id));
    return buildingParts;
  }

  for (auto const & v : relation.m_ways)
    if (v.second == "part")
      buildingParts.emplace_back(base::MakeOsmWay(v.first));

  for (auto const & v : relation.m_relations)
    if (v.second == "part")
      buildingParts.emplace_back(base::MakeOsmRelation(v.first));
  return buildingParts;
}

base::GeoObjectId BuildingPartsCollector::FindTopRelation(base::GeoObjectId elId)
{
  IdRelationVec elements;
  RelationFetcher fetcher(elements);
  cache::IntermediateDataReaderInterface::ForEachRelationFn wrapper =
      base::ControlFlowWrapper<RelationFetcher>(fetcher);
  IdRelationVec::const_iterator it;
  auto const serialId = elId.GetSerialId();
  if (elId.GetType() == base::GeoObjectId::Type::ObsoleteOsmWay)
  {
    m_cache->ForEachRelationByWayCached(serialId, wrapper);
    it = base::FindIf(elements,
                      [&](auto const & idRelation) { return idRelation.second.GetWayRole(serialId) == "outline"; });
  }
  else if (elId.GetType() == base::GeoObjectId::Type::ObsoleteOsmRelation)
  {
    m_cache->ForEachRelationByRelationCached(serialId, wrapper);
    it = base::FindIf(
        elements, [&](auto const & idRelation) { return idRelation.second.GetRelationRole(serialId) == "outline"; });
  }

  return it != std::end(elements) ? base::MakeOsmRelation(it->first) : base::GeoObjectId();
}

void BuildingPartsCollector::Finish()
{
  m_writer.reset();
}

void BuildingPartsCollector::Save()
{
  CHECK(!m_writer, ("Finish() has not been called."));
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void BuildingPartsCollector::OrderCollectedData()
{
  std::vector<BuildingParts> collectedData;
  {
    FileReader reader(GetFilename());
    ReaderSource src(reader);
    while (src.Size() > 0)
      collectedData.emplace_back(BuildingParts::Read(src));
  }
  std::sort(std::begin(collectedData), std::end(collectedData));
  FileWriter writer(GetFilename());
  for (auto const & bp : collectedData)
    BuildingParts::Write(writer, bp);
}

void BuildingPartsCollector::MergeInto(BuildingPartsCollector & collector) const
{
  CHECK(!m_writer || !collector.m_writer, ("Finish() has not been called."));
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}

BuildingToBuildingPartsMap::BuildingToBuildingPartsMap(std::string const & filename)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    auto const buildingParts = BuildingPartsCollector::BuildingParts::Read(src);
    m_buildingParts.insert(std::end(m_buildingParts), std::begin(buildingParts.m_buildingParts),
                           std::end(buildingParts.m_buildingParts));
    m_outlineToBuildingPart.emplace_back(buildingParts.m_id, std::move(buildingParts.m_buildingParts));
  }

  m_outlineToBuildingPart.shrink_to_fit();
  m_buildingParts.shrink_to_fit();
  std::sort(std::begin(m_outlineToBuildingPart), std::end(m_outlineToBuildingPart));
  std::sort(std::begin(m_buildingParts), std::end(m_buildingParts));
}

bool BuildingToBuildingPartsMap::HasBuildingPart(base::GeoObjectId const & id)
{
  return std::binary_search(std::cbegin(m_buildingParts), std::cend(m_buildingParts), id);
}

std::vector<base::GeoObjectId> const & BuildingToBuildingPartsMap::GetBuildingPartsByOutlineId(CompositeId const & id)
{
  auto const it = std::lower_bound(std::cbegin(m_outlineToBuildingPart), std::cend(m_outlineToBuildingPart), id,
                                   [](auto const & lhs, auto const & rhs) { return lhs.first < rhs; });

  if (it != std::cend(m_outlineToBuildingPart) && it->first == id)
    return it->second;

  static std::vector<base::GeoObjectId> const kEmpty;
  return kEmpty;
}
}  // namespace generator
