#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/drape_global.hpp"

#include "geometry/rect2d.hpp"
#include "indexer/feature.hpp"

#include <functional>
#include <vector>

namespace terrain
{
class TileMesh;
}

namespace df
{
class MapDataProvider
{
public:
  template <typename T>
  using TReadCallback = std::function<void(T &)>;
  using TReadFeaturesFn = std::function<void(TReadCallback<FeatureType> const &, std::vector<FeatureID> const &)>;
  using TReadIDsFn = std::function<void(TReadCallback<FeatureID const> const &, m2::RectD const &, int)>;
  using TIsCountryLoadedFn = std::function<bool(m2::PointD const &)>;
  using TIsCountryLoadedByNameFn = std::function<bool(std::string_view)>;
  using TUpdateCurrentCountryFn = std::function<void(m2::PointD const &, int)>;
  using TTileBackgroundReadFn = std::function<bool(df::TileKey const &, dp::BackgroundMode)>;
  using TCancelTileBackgroundReadingFn = std::function<void(df::TileKey const &, dp::BackgroundMode)>;
  // Dynamic isolines and hillshading from the TWM terrain files.
  using THasTerrainFn = std::function<bool(m2::RectD const &)>;
  using TReadTerrainFn = std::function<void(m2::RectD const &, int, terrain::TileMesh &)>;

  MapDataProvider(TReadIDsFn && idsReader, TReadFeaturesFn && featureReader,
                  TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                  TUpdateCurrentCountryFn && updateCurrentCountryFn, TTileBackgroundReadFn && tileBackgroundReadFn,
                  TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn, THasTerrainFn && hasTerrainFn,
                  TReadTerrainFn && readTerrainFn);

  void ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadCallback<FeatureType> const & fn, std::vector<FeatureID> const & ids) const;

  // Terrain availability and the tile mesh reading; safe to call from the tile reading threads.
  bool HasTerrain(m2::RectD const & rect) const;
  void ReadTerrainMesh(m2::RectD const & rect, int zoom, terrain::TileMesh & mesh) const;

  TTileBackgroundReadFn ReadTileBackgroundFn() const;
  TCancelTileBackgroundReadingFn CancelTileBackgroundReadingFn() const;

  TUpdateCurrentCountryFn const & UpdateCurrentCountryFn() const;

  TIsCountryLoadedByNameFn m_isCountryLoadedByName;

private:
  TReadFeaturesFn m_featureReader;
  TReadIDsFn m_idsReader;
  TUpdateCurrentCountryFn m_updateCurrentCountry;
  TTileBackgroundReadFn m_tileBackgroundReader;
  TCancelTileBackgroundReadingFn m_cancelTileBackgroundReading;
  THasTerrainFn m_hasTerrain;
  TReadTerrainFn m_readTerrain;
};
}  // namespace df
