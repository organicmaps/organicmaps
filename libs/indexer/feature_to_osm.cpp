#include "indexer/feature_to_osm.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/utils.hpp"

#include "coding/reader.hpp"

#include "base/macros.hpp"

#include "defines.hpp"

namespace indexer
{
// FeatureIdToGeoObjectIdOneWay --------------------------------------------------------------------
FeatureIdToGeoObjectIdOneWay::FeatureIdToGeoObjectIdOneWay(DataSource const & dataSource)
  : m_dataSource(dataSource)
  , m_reader(std::unique_ptr<ModelReader>())
{}

bool FeatureIdToGeoObjectIdOneWay::Load()
{
  auto const handle = indexer::FindWorld(m_dataSource);
  if (!handle.IsAlive())
  {
    LOG(LWARNING, ("Can't find World map file."));
    return false;
  }

  if (handle.GetId() == m_mwmId)
    return true;

  auto const & cont = handle.GetValue()->m_cont;

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

bool FeatureIdToGeoObjectIdOneWay::GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id)
{
  if (fid.m_mwmId != m_mwmId)
  {
    LOG(LERROR, ("Wrong mwm: feature has", fid.m_mwmId, "expected:", m_mwmId));
    return false;
  }

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

// FeatureIdToGeoObjectIdTwoWay --------------------------------------------------------------------
FeatureIdToGeoObjectIdTwoWay::FeatureIdToGeoObjectIdTwoWay(DataSource const & dataSource) : m_dataSource(dataSource) {}

bool FeatureIdToGeoObjectIdTwoWay::Load()
{
  auto const handle = indexer::FindWorld(m_dataSource);
  if (!handle.IsAlive())
  {
    LOG(LWARNING, ("Can't find World map file."));
    return false;
  }

  if (handle.GetId() == m_mwmId)
    return true;

  auto const & cont = handle.GetValue()->m_cont;

  if (!cont.IsExist(FEATURE_TO_OSM_FILE_TAG))
  {
    LOG(LWARNING, ("No cities fid bimap in the world map."));
    return false;
  }

  bool success = false;
  try
  {
    FilesContainerR::TReader const reader = cont.GetReader(FEATURE_TO_OSM_FILE_TAG);
    success = FeatureIdToGeoObjectIdSerDes::Deserialize(*reader.GetPtr(), m_map);
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

bool FeatureIdToGeoObjectIdTwoWay::GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id)
{
  if (fid.m_mwmId != m_mwmId)
  {
    LOG(LERROR, ("Wrong mwm: feature has", fid.m_mwmId, "expected:", m_mwmId));
    return false;
  }

  return m_map.GetValue(fid.m_index, id);
}

bool FeatureIdToGeoObjectIdTwoWay::GetFeatureID(base::GeoObjectId const & id, FeatureID & fid)
{
  uint32_t index;
  if (!m_map.GetKey(id, index))
    return false;
  fid = FeatureID(m_mwmId, index);
  return true;
}
}  // namespace indexer
