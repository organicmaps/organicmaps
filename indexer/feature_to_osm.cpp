#include "indexer/feature_to_osm.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/utils.hpp"

#include "coding/reader.hpp"

#include "base/macros.hpp"

#include "defines.hpp"

namespace indexer
{
FeatureIdToGeoObjectIdBimap::FeatureIdToGeoObjectIdBimap(DataSource const & dataSource)
  : m_dataSource(dataSource), m_reader(std::unique_ptr<ModelReader>())
{
}

bool FeatureIdToGeoObjectIdBimap::Load()
{
  auto const handle = indexer::FindWorld(m_dataSource);
  if (!handle.IsAlive())
  {
    LOG(LWARNING, ("Can't find World map file."));
    return false;
  }

  if (handle.GetId() == m_mwmId)
    return true;

  auto const & cont = handle.GetValue<MwmValue>()->m_cont;

  if (!cont.IsExist(FEATURE_TO_OSM_FILE_TAG))
  {
    LOG(LWARNING, ("No cities fid bimap in the world map."));
    return false;
  }

  bool success = false;
  try
  {
    m_reader = cont.GetReader(FEATURE_TO_OSM_FILE_TAG);
    success = FeatureIdToGeoObjectIdSerDes::Deserialize(*m_reader.GetPtr(), *this);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Can't read cities fid bimap from the world map:", e.Msg()));
    return false;
  }

  if (success)
  {
    m_mwmId = handle.GetId();
    return true;
  }

  return false;
}

bool FeatureIdToGeoObjectIdBimap::GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id)
{
  if (fid.m_mwmId != m_mwmId)
  {
    LOG(LERROR, ("Wrong mwm: feature has", fid.m_mwmId, "expected:", m_mwmId));
    return false;
  }

  if (m_memAll != nullptr)
    return m_memAll->GetValue(fid.m_index, id);

  if (m_mapNodes == nullptr || m_mapWays == nullptr || m_mapRelations == nullptr)
    return false;

  uint64_t serialId;
  if (m_mapNodes->Get(fid.m_index, serialId))
  {
    id = base::MakeOsmNode(serialId);
    return true;
  }

  if (m_mapWays->Get(fid.m_index, serialId))
  {
    id = base::MakeOsmWay(serialId);
    return true;
  }

  if (m_mapRelations->Get(fid.m_index, serialId))
  {
    id = base::MakeOsmRelation(serialId);
    return true;
  }

  return false;
}

bool FeatureIdToGeoObjectIdBimap::GetFeatureID(base::GeoObjectId const & id, FeatureID & fid)
{
  LOG(LWARNING, ("Getting GeoObjectId by FeatureId uses a lot of RAM in current implementation"));

  if (!m_mwmId.IsAlive())
    return false;

  if (m_memAll == nullptr)
  {
    m_memAll = std::make_unique<FeatureIdToGeoObjectIdBimapMem>();
    bool const success = FeatureIdToGeoObjectIdSerDes::Deserialize(*m_reader.GetPtr(), *m_memAll);
    if (!success)
    {
      m_memAll.reset();
      LOG(LERROR, ("Could not deserialize the FeatureId to OsmId mapping into memory."));
      return false;
    }
  }

  uint32_t index;
  if (!m_memAll->GetKey(id, index))
    return false;

  fid = FeatureID(m_mwmId, index);
  return true;
}

// static
std::string const FeatureIdToGeoObjectIdSerDes::kHeaderMagic = "mwmftosm";
FeatureIdToGeoObjectIdSerDes::Version const FeatureIdToGeoObjectIdSerDes::kLatestVersion =
    FeatureIdToGeoObjectIdSerDes::Version::V0;
size_t constexpr FeatureIdToGeoObjectIdSerDes::kMagicAndVersionSize = 9;
size_t constexpr FeatureIdToGeoObjectIdSerDes::kHeaderOffset = 16;
}  // namespace indexer
