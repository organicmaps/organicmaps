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
  : m_dataSource(dataSource)
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

  if (!cont.IsExist(CITIES_IDS_FILE_TAG))
  {
    LOG(LWARNING, ("No cities fid bimap in the world map."));
    return false;
  }

  try
  {
    auto reader = cont.GetReader(CITIES_IDS_FILE_TAG);
    ReaderSource<ReaderPtr<ModelReader>> source(reader);
    FeatureIdToGeoObjectIdSerDes::Deserialize(source, m_map);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Can't read cities fid bimap from the world map:", e.Msg()));
    return false;
  }

  m_mwmId = handle.GetId();
  return true;
}

bool FeatureIdToGeoObjectIdBimap::GetGeoObjectId(FeatureID const & fid, base::GeoObjectId & id)
{
  if (fid.m_mwmId != m_mwmId)
  {
    LOG(LERROR, ("Wrong mwm: feature has", fid.m_mwmId, "expected:", m_mwmId));
    return false;
  }

  return m_map.GetValue(fid.m_index, id);
}

bool FeatureIdToGeoObjectIdBimap::GetFeatureID(base::GeoObjectId const & id, FeatureID & fid)
{
  uint32_t index;
  if (!m_map.GetKey(id, index))
    return false;

  fid = FeatureID(m_mwmId, index);
  return true;
}
}  // namespace indexer
