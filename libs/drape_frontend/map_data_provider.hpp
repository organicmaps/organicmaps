#pragma once

#include "drape_frontend/tile_key.hpp"

#include "storage/storage_defines.hpp"

#include "drape/drape_global.hpp"

#include "geometry/rect2d.hpp"
#include "indexer/feature.hpp"
#include "indexer/terrain/isolines_tracer.hpp"

#include <functional>
#include <string>
#include <vector>

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
  using TIsolineCallback = std::function<void(terrain::Isoline &&)>;
  using TTrianglesCallback = std::function<void(terrain::Triangles const &)>;
  using THasTerrainFn = std::function<bool(m2::RectD const &)>;
  using TReadIsolinesFn = std::function<void(m2::RectD const &, int, TIsolineCallback const &)>;
  using TReadTrianglesFn = std::function<void(m2::RectD const &, int, TTrianglesCallback const &)>;

  MapDataProvider(TReadIDsFn && idsReader, TReadFeaturesFn && featureReader,
                  TIsCountryLoadedByNameFn && isCountryLoadedByNameFn,
                  TUpdateCurrentCountryFn && updateCurrentCountryFn, TTileBackgroundReadFn && tileBackgroundReadFn,
                  TCancelTileBackgroundReadingFn && cancelTileBackgroundReadingFn, THasTerrainFn && hasTerrainFn,
                  TReadIsolinesFn && readIsolinesFn, TReadTrianglesFn && readTrianglesFn);

  void ReadFeaturesID(TReadCallback<FeatureID const> const & fn, m2::RectD const & r, int scale) const;
  void ReadFeatures(TReadCallback<FeatureType> const & fn, std::vector<FeatureID> const & ids) const;

  // Dynamic isolines availability and reading; safe to call from the tile reading threads.
  bool HasTerrain(m2::RectD const & rect) const;
  void ReadIsolines(m2::RectD const & rect, int zoom, TIsolineCallback const & fn) const;
  void ReadTriangles(m2::RectD const & rect, int zoom, TTrianglesCallback const & fn) const;

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
  TReadIsolinesFn m_readIsolines;
  TReadTrianglesFn m_readTriangles;
};
}  // namespace df
