#pragma once

#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/isolines_profile.hpp"
#include "topography_generator/tile_filter.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"

#include <memory>
#include <string>

namespace topography_generator
{
struct TileIsolinesParams
{
  Altitude m_alitudesStep = 10;
  size_t m_latLonStepFactor = 1;
  int m_simplificationZoom = 17; // Value == 0 disables simplification.
  FiltersSequence<Altitude> m_filters;
  std::string m_outputDir;
};

using ProfileToTileIsolinesParams = std::map<std::string, TileIsolinesParams>;

struct TileIsolinesProfileParams
{
  TileIsolinesProfileParams(ProfileToTileIsolinesParams const & profiles,
                            std::string const & tilesProfilesDir)
    : m_profiles(profiles)
    , m_tilesProfilesDir(tilesProfilesDir)
  {}

  ProfileToTileIsolinesParams const & m_profiles;
  std::string m_tilesProfilesDir;
};

struct IsolinesPackingParams
{
  size_t m_maxIsolineLength = 1000;
  int m_simplificationZoom = 17; // Value == 0 disables simplification.
  size_t m_alitudesStepFactor = 1;
  std::string m_isolinesTilesPath;
  std::string m_outputDir;
};

using ProfileToIsolinesPackingParams = std::map<std::string, IsolinesPackingParams>;

class Generator
{
public:
  Generator(std::string const & srtmPath, long threadsCount, long maxCachedTilesPerThread,
            bool forceRegenerate);

  void InitCountryInfoGetter(std::string const & dataDir);

  void GenerateIsolines(int left, int bottom, int right, int top,
                        TileIsolinesParams const & params);

  void PackIsolinesForCountry(storage::CountryId const & countryId,
                              IsolinesPackingParams const & params);

  void InitProfiles(std::string const & isolinesProfilesFileName,
                    std::string const & countriesToGenerateFileName,
                    std::string const & isolinesTilesOutDir,
                    std::string const & isolinesCountriesOutDir);

  void GenerateIsolinesForCountries();
  void PackIsolinesForCountries();

private:
  void GenerateIsolines(int left, int bottom, int right, int top,
                        std::string const & tilesProfilesDir);

  using NeedSkipTileFn = std::function<bool(int lat, int lon)>;
  void PackIsolinesForCountry(storage::CountryId const & countryId,
                              IsolinesPackingParams const & params,
                              NeedSkipTileFn const & needSkipTileFn);

  void GetCountryRegions(storage::CountryId const & countryId, m2::RectD & countryRect,
                         std::vector<m2::RegionD> & countryRegions);

  std::string m_isolinesTilesOutDir;
  std::string m_isolinesCountriesOutDir;

  CountriesToGenerate m_countriesToGenerate;
  IsolinesProfilesCollection m_profilesCollection;

  ProfileToTileIsolinesParams m_profileToTileParams;
  ProfileToIsolinesPackingParams m_profileToPackingParams;

  std::unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  storage::CountryInfoReader * m_infoReader = nullptr;

  // They can't be negative, it is done to avoid compiler warnings.
  long m_threadsCount;
  long m_maxCachedTilesPerThread;
  std::string m_srtmPath;
  bool m_forceRegenerate;
};
}  // namespace topography_generator
