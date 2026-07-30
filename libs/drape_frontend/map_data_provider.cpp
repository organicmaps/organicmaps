#include "drape_frontend/map_data_provider.hpp"

#include "base/assert.hpp"

namespace df
{
MapDataProvider::MapDataProvider(TReadIDsFn && idsReader, TReadFeaturesFn && featureReader,
                                 TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                                 TUpdateCurrentCountryFn && updateCurrentCountryFn,
                                 TTileBackgroundReadFn && tileBackgroundReadFn,
                                 TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn,
                                 THasTerrainFn && hasTerrainFn, TReadTerrainFn && readTerrainFn)
  : m_isCountryLoadedByName(std::move(isCountryLoadedByNameFn))
  , m_featureReader(std::move(featureReader))
  , m_idsReader(std::move(idsReader))
  , m_updateCurrentCountry(std::move(updateCurrentCountryFn))
  , m_tileBackgroundReader(std::move(tileBackgroundReadFn))
  , m_cancelTileBackgroundReading(std::move(cancelTileBackgroundReadingFn))
  , m_hasTerrain(std::move(hasTerrainFn))
  , m_readTerrain(std::move(readTerrainFn))
{
  CHECK(m_isCountryLoadedByName != nullptr, ());
  CHECK(m_featureReader != nullptr, ());
  CHECK(m_idsReader != nullptr, ());
  CHECK(m_updateCurrentCountry != nullptr, ());
  CHECK(m_tileBackgroundReader != nullptr, ());
  CHECK(m_cancelTileBackgroundReading != nullptr, ());
  CHECK(m_hasTerrain != nullptr, ());
  CHECK(m_readTerrain != nullptr, ());
}

void MapDataProvider::ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r, int scale) const
{
  m_idsReader(fn, r, scale);
}

void MapDataProvider::ReadFeatures(TReadCallback<FeatureType> const & fn, std::vector<FeatureID> const & ids) const
{
  m_featureReader(fn, ids);
}

bool MapDataProvider::HasTerrain(m2::RectD const & rect) const
{
  return m_hasTerrain(rect);
}

void MapDataProvider::ReadTerrainMesh(m2::RectD const & rect, int zoom, terrain::TileMesh & mesh) const
{
  m_readTerrain(rect, zoom, mesh);
}

MapDataProvider::TTileBackgroundReadFn MapDataProvider::ReadTileBackgroundFn() const
{
  return m_tileBackgroundReader;
}

MapDataProvider::TCancelTileBackgroundReadingFn MapDataProvider::CancelTileBackgroundReadingFn() const
{
  return m_cancelTileBackgroundReading;
}

MapDataProvider::TUpdateCurrentCountryFn const & MapDataProvider::UpdateCurrentCountryFn() const
{
  return m_updateCurrentCountry;
}
}  // namespace df
