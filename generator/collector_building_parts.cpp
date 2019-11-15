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

#include <mutex.h>
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

  template <class Reader>
  base::ControlFlow operator()(uint64_t id, Reader & reader)
  {
    RelationElement element;
    if (reader.Read(id, element))
      m_elements.emplace_back(id, std::move(element));
    else
      LOG(LWARNING, ("Cannot read relation with id", id));
    return base::ControlFlow::Continue;
  }

private:
  IdRelationVec & m_elements;
};
}  // namespace

namespace generator
{
BuildingPartsCollector::BuildingPartsCollector(
    std::string const & filename, std::shared_ptr<cache::IntermediateDataReader> const & cache)
  : CollectorInterface(filename)
  , m_cache(cache)
  , m_writer(std::make_unique<FileWriter>(GetTmpFilename()))
{
}

std::shared_ptr<CollectorInterface> BuildingPartsCollector::Clone(
    std::shared_ptr<cache::IntermediateDataReader> const & cache) const
{
  return std::make_shared<BuildingPartsCollector>(GetFilename(), cache ? cache : m_cache);
}

void BuildingPartsCollector::CollectFeature(feature::FeatureBuilder const & fb,
                                            OsmElement const & element)
{
  static auto const & buildingChecker = ftypes::IsBuildingChecker::Instance();
  if (!fb.IsArea() || !buildingChecker(fb.GetTypes()))
    return;

  auto const topId = FindTopRelation(element);
  if (topId == base::GeoObjectId())
    return;

  auto const parts = FindAllBuildingParts(topId);
  if (!parts.empty())
    WritePair(MakeCompositeId(fb), parts);
}

std::vector<base::GeoObjectId> BuildingPartsCollector::FindAllBuildingParts(
    base::GeoObjectId const & id)
{
  std::vector<base::GeoObjectId> buildingPatrs;
  RelationElement relation;
  if (!m_cache->GetRalation(id.GetSerialId(), relation))
  {
    LOG(LWARNING, ("Cannot read relation with id", id));
    return buildingPatrs;
  }

  for (auto const & v : relation.m_ways)
  {
    if (v.second == "part")
      buildingPatrs.emplace_back(base::MakeOsmWay(v.first));
  }

  for (auto const & v : relation.m_relations)
  {
    if (v.second == "part")
      buildingPatrs.emplace_back(base::MakeOsmRelation(v.first));
  }
  return buildingPatrs;
}

base::GeoObjectId BuildingPartsCollector::FindTopRelation(OsmElement const & element)
{
  IdRelationVec elements;
  RelationFetcher fetcher(elements);
  IdRelationVec::const_iterator it;
  if (element.IsWay())
  {
    m_cache->ForEachRelationByNodeCached(element.m_id, fetcher);
    it = base::FindIf(elements, [&](auto const & idRelation) {
      return idRelation.second.GetWayRole(element.m_id) == "outline";
    });
  }
  else if (element.IsRelation())
  {
    m_cache->ForEachRelationByRelationCached(element.m_id, fetcher);
    it = base::FindIf(elements, [&](auto const & idRelation) {
      return idRelation.second.GetRelationRole(element.m_id) == "outline";
    });
  }

  return it != std::end(elements) ? base::MakeOsmRelation(it->first) : base::GeoObjectId();
}

void BuildingPartsCollector::WritePair(CompositeId const & id,
                                       std::vector<base::GeoObjectId> const & buildingParts)
{
  WriteToSink(m_writer, id.m_mainId.GetEncodedId());
  WriteToSink(m_writer, id.m_additionalId.GetEncodedId());
  auto const contSize = base::checked_cast<uint32_t>(buildingParts.size());
  WriteVarUint(m_writer, contSize);
  for (auto const & id : buildingParts)
    WriteToSink(m_writer, id.GetEncodedId());
}

void BuildingPartsCollector::Finish() { m_writer.reset(); }

void BuildingPartsCollector::Save()
{
  CHECK(!m_writer, ("Finish() has not been called."));
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void BuildingPartsCollector::Merge(CollectorInterface const & collector)
{
  collector.MergeInto(*this);
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
    base::GeoObjectId const mainId(ReadPrimitiveFromSource<uint64_t>(src));
    CompositeId const outlineId(mainId, base::GeoObjectId(ReadPrimitiveFromSource<uint64_t>(src)));
    std::vector<base::GeoObjectId> buildingParts;
    auto contSize = ReadVarUint<uint32_t>(src);
    buildingParts.reserve(contSize);
    while (contSize--)
    {
      base::GeoObjectId const id(ReadPrimitiveFromSource<uint64_t>(src));
      m_buildingParts.emplace_back(id);
      buildingParts.emplace_back(id);
    }
    m_outlineToBuildingPart.emplace_back(outlineId, std::move(buildingParts));
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

std::vector<base::GeoObjectId> const & BuildingToBuildingPartsMap::GetBuildingPartByOutlineId(
    CompositeId const & id)
{
  auto const it =
      std::lower_bound(std::cbegin(m_outlineToBuildingPart), std::cend(m_outlineToBuildingPart), id,
                       [](auto const & lhs, auto const & rhs) { return lhs.first < rhs; });

  if (it != std::cend(m_outlineToBuildingPart) && it->first == id)
    return it->second;

  static std::vector<base::GeoObjectId> const kEmpty;
  return kEmpty;
}
}  // namespace generator
