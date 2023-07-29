#include "topography_generator/generator.hpp"
#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/marching_squares/marching_squares.hpp"
#include "topography_generator/utils/contours_serdes.hpp"

#include "platform/platform.hpp"

#include "generator/srtm_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <vector>

namespace topography_generator
{
namespace
{
size_t constexpr kArcSecondsInDegree = 60 * 60;
int constexpr kAsterTilesLatTop = 60;
int constexpr kAsterTilesLatBottom = -60;

void MercatorRectToTilesRange(m2::RectD const & rect,
                              int & left, int & bottom, int & right, int & top)
{
  auto const leftBottom = mercator::ToLatLon(rect.LeftBottom());
  auto const rightTop = mercator::ToLatLon(rect.RightTop());

  left = static_cast<int>(floor(leftBottom.m_lon));
  bottom = static_cast<int>(floor(leftBottom.m_lat));
  right = std::min(179, static_cast<int>(floor(rightTop.m_lon)));
  top = std::min(89, static_cast<int>(floor(rightTop.m_lat)));
}

std::string GetTileProfilesDir(std::string const & tilesDir)
{
  return base::JoinPath(tilesDir, "tiles_profiles");
}

std::string GetTileProfilesFilePath(int lat, int lon, std::string const & profilesDir)
{
  return GetIsolinesFilePath(lat, lon, profilesDir) + ".profiles";
}

std::string GetTilesDir(std::string const & tilesDir, std::string const & profileName)
{
  return base::JoinPath(tilesDir, profileName);
}

void AppendTileProfile(std::string const & fileName, std::string const & profileName)
{
  std::ofstream fout(fileName, std::ios::app);
  fout << profileName << std::endl;
}

bool LoadTileProfiles(std::string const & fileName, std::set<std::string> & profileNames)
{
  std::ifstream fin(fileName);
  if (!fin)
    return false;
  std::string line;
  while (std::getline(fin, line))
  {
    if (!line.empty())
      profileNames.insert(line);
  }
  return true;
}

class SrtmProvider : public ValuesProvider<Altitude>
{
public:
  explicit SrtmProvider(std::string const & srtmDir):
    m_srtmManager(srtmDir)
  {}

  void SetPrefferedTile(ms::LatLon const & pos)
  {
    m_preferredTile = &m_srtmManager.GetTile(pos);
    m_leftBottomOfPreferredTile = {std::floor(pos.m_lat), std::floor(pos.m_lon)};
  }

  Altitude GetValue(ms::LatLon const & pos) override
  {
    auto const alt = GetValueImpl(pos);
    if (IsValidAltitude(alt))
      return alt;
    return GetMedianValue(pos);
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

  static bool IsValidAltitude(Altitude alt)
  {
    return alt != kInvalidAltitude && alt > -435 && alt < 8850;
  }

private:
  Altitude GetValueImpl(ms::LatLon const & pos)
  {
    if (m_preferredTile != nullptr)
    {
      // Each SRTM tile overlaps the top row in the bottom tile and the right row in the left tile.
      // Try to prevent loading a new tile if the position can be found in the loaded one.
      auto const latDist = pos.m_lat - m_leftBottomOfPreferredTile.m_lat;
      auto const lonDist = pos.m_lon - m_leftBottomOfPreferredTile.m_lon;
      if (latDist > -mercator::kPointEqualityEps && latDist < 1.0 + mercator::kPointEqualityEps && lonDist > -mercator::kPointEqualityEps && lonDist < 1.0 + mercator::kPointEqualityEps)
      {
        ms::LatLon innerPos = pos;
        if (latDist < 0.0)
          innerPos.m_lat += mercator::kPointEqualityEps;
        else if (latDist >= 1.0)
          innerPos.m_lat -= mercator::kPointEqualityEps;
        if (lonDist < 0.0)
          innerPos.m_lon += mercator::kPointEqualityEps;
        else if (lonDist >= 1.0)
          innerPos.m_lon -= mercator::kPointEqualityEps;
        return m_preferredTile->GetHeight(innerPos);
      }
    }

    return m_srtmManager.GetHeight(pos);
  }

  Altitude GetMedianValue(ms::LatLon const & pos)
  {
    // Look around the position with invalid altitude
    // and return median of surrounding valid altitudes.
    double const step = 1.0 / kArcSecondsInDegree;
    int const kMaxKernelRadius = 3;
    std::vector<Altitude> kernel;
    int kernelRadius = 0;
    while (kernel.empty() && kernelRadius < kMaxKernelRadius)
    {
      ++kernelRadius;
      auto const kernelSize = static_cast<size_t>(kernelRadius * 2 + 1);
      kernel.reserve(4 * (kernelSize - 1));
      for (int i = -kernelRadius; i <= kernelRadius; ++i)
      {
        for (int j = -kernelRadius; j <= kernelRadius; ++j)
        {
          if (abs(i) != kernelRadius && abs(j) != kernelRadius)
            continue;
          auto const alt = GetValueImpl({pos.m_lat + i * step, pos.m_lon + j * step});
          if (IsValidAltitude(alt))
            kernel.push_back(alt);
        }
      }
    }

    if (kernel.empty())
    {
      LOG(LWARNING, ("Can't fix invalid value", GetValueImpl(pos), "at the position", pos));
      return kInvalidAltitude;
    }

    std::nth_element(kernel.begin(), kernel.begin() + kernel.size() / 2, kernel.end());
    return kernel[kernel.size() / 2];
  }

  generator::SrtmTileManager m_srtmManager;
  generator::SrtmTile const * m_preferredTile = nullptr;
  ms::LatLon m_leftBottomOfPreferredTile;
};

class RawAltitudesTile : public ValuesProvider<Altitude>
{
public:
  RawAltitudesTile(std::vector<Altitude> const & values,
                   int leftLon, int bottomLat)
    : m_values(values)
    , m_leftLon(leftLon)
    , m_bottomLat(bottomLat)
  {}

  Altitude GetValue(ms::LatLon const & pos) override
  {
    double ln = pos.m_lon - m_leftLon;
    double lt = pos.m_lat - m_bottomLat;
    lt = 1 - lt;  // From North to South, the same direction as inside the SRTM tiles.

    auto const row = static_cast<size_t>(std::round(kArcSecondsInDegree * lt));
    auto const col = static_cast<size_t>(std::round(kArcSecondsInDegree * ln));

    auto const ix = row * (kArcSecondsInDegree + 1) + col;
    CHECK(ix < m_values.size(), (pos));

    return m_values[ix];
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

private:
  std::vector<Altitude> const & m_values;
  int m_leftLon;
  int m_bottomLat;
};

class SeamlessAltitudeProvider : public ValuesProvider<Altitude>
{
public:
  using IsOnBorderFn = std::function<bool (ms::LatLon const & pos)>;

  SeamlessAltitudeProvider(ValuesProvider<Altitude> & originalProvider,
                           ValuesProvider<Altitude> & filteredProvider,
                           IsOnBorderFn && isOnBorderFn)
    : m_originalProvider(originalProvider)
    , m_filteredProvider(filteredProvider)
    , m_isOnBorderFn(std::move(isOnBorderFn))
  {}

  Altitude GetValue(ms::LatLon const & pos) override
  {
    if (m_isOnBorderFn(pos))
    {
      // Check that we have original neighboring tile, use filtered if haven't.
      auto const alt = m_originalProvider.GetValue(pos);
      if (alt != kInvalidAltitude)
        return alt;
    }
    return m_filteredProvider.GetValue(pos);
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

private:
  ValuesProvider<Altitude> & m_originalProvider;
  ValuesProvider<Altitude> & m_filteredProvider;
  IsOnBorderFn m_isOnBorderFn;
};

class TileIsolinesTask
{
public:
  TileIsolinesTask(int left, int bottom, int right, int top, std::string const & srtmDir,
                   TileIsolinesParams const * params, bool forceRegenerate)
    : m_strmDir(srtmDir)
    , m_srtmProvider(srtmDir)
    , m_params(params)
    , m_forceRegenerate(forceRegenerate)
  {
    CHECK(params != nullptr, ());
    Init(left, bottom, right, top);
  }

  TileIsolinesTask(int left, int bottom, int right, int top, std::string const & srtmDir,
                   TileIsolinesProfileParams const * profileParams, bool forceRegenerate)
    : m_strmDir(srtmDir)
    , m_srtmProvider(srtmDir)
    , m_profileParams(profileParams)
    , m_forceRegenerate(forceRegenerate)
  {
    CHECK(profileParams != nullptr, ());
    Init(left, bottom, right, top);
  }

  void Do()
  {
    for (int lat = m_bottom; lat <= m_top; ++lat)
    {
      for (int lon = m_left; lon <= m_right; ++lon)
        ProcessTile(lat, lon);
    }
  }

private:
  void Init(int left, int bottom, int right, int top)
  {
    CHECK(right >= -179 && right <= 180, (right));
    CHECK(left >= -180 && left <= 179, (left));
    CHECK(top >= -89 && top <= 90, (top));
    CHECK(bottom >= -90 && bottom <= 89, (bottom));

    m_left = left;
    m_bottom = bottom;
    m_right = right;
    m_top = top;
  }

  void ProcessTile(int lat, int lon)
  {
    auto const tileName = GetIsolinesTileBase(lat, lon);

    if (m_profileParams != nullptr)
    {
      auto const profilesPath = GetTileProfilesFilePath(lat, lon, m_profileParams->m_tilesProfilesDir);
      if (!GetPlatform().IsFileExistsByFullPath(profilesPath))
      {
        LOG(LINFO, ("SRTM tile", tileName, "doesn't have profiles, skip processing."));
        return;
      }
    }

    if (!GetPlatform().IsFileExistsByFullPath(generator::SrtmTile::GetPath(m_strmDir, tileName)))
    {
      LOG(LINFO, ("SRTM tile", tileName, "doesn't exist, skip processing."));
      return;
    }

    std::ostringstream os;
    os << tileName << " (" << lat << ", " << lon << ")";
    m_debugId = os.str();

    if (m_profileParams != nullptr)
    {
      auto const profilesPath = GetTileProfilesFilePath(lat, lon, m_profileParams->m_tilesProfilesDir);

      std::set<std::string> profileNames;
      CHECK(LoadTileProfiles(profilesPath, profileNames) && !profileNames.empty(), (tileName));

      for (auto const & profileName : profileNames)
      {
        auto const & params = m_profileParams->m_profiles.at(profileName);
        ProcessTile(lat, lon, tileName, profileName, params);
      }
    }
    else
    {
      ProcessTile(lat, lon, tileName, "none", *m_params);
    }
  }

  void ProcessTile(int lat, int lon, std::string const & tileName, std::string const & profileName,
                   TileIsolinesParams const & params)
  {
    auto const outFile = GetIsolinesFilePath(lat, lon, params.m_outputDir);
    if (!m_forceRegenerate && GetPlatform().IsFileExistsByFullPath(outFile))
    {
      LOG(LINFO, ("Isolines for", tileName, ", profile", profileName,
                  "are ready, skip processing."));
      return;
    }

    LOG(LINFO, ("Begin generating isolines for tile", tileName, ", profile", profileName));

    m_srtmProvider.SetPrefferedTile({lat + 0.5, lon + 0.5});

    Contours<Altitude> contours;
    if (!params.m_filters.empty() && (lat >= kAsterTilesLatTop || lat < kAsterTilesLatBottom))
    {
      // Filter tiles converted from ASTER, cause they are noisy enough.
      std::vector<Altitude> filteredValues = FilterTile(params.m_filters,
                                                        ms::LatLon(lat, lon),
                                                        kArcSecondsInDegree,
                                                        kArcSecondsInDegree + 1,
                                                        m_srtmProvider);
      RawAltitudesTile filteredProvider(filteredValues, lon, lat);
      GenerateSeamlessContours(lat, lon, params, filteredProvider, contours);
    }
    else
    {
      GenerateSeamlessContours(lat, lon, params, m_srtmProvider, contours);
    }

    LOG(LINFO, ("Isolines for tile", tileName, ", profile", profileName,
      "min altitude", contours.m_minValue, "max altitude",
      contours.m_maxValue, "invalid values count", contours.m_invalidValuesCount));

    if (params.m_simplificationZoom > 0)
      SimplifyContours(params.m_simplificationZoom, contours);
    SaveContrours(outFile, std::move(contours));

    LOG(LINFO, ("End generating isolines for tile", tileName, ", profile", profileName));
  }

  void GenerateSeamlessContours(int lat, int lon, TileIsolinesParams const & params,
                                ValuesProvider<Altitude> & altProvider,
                                Contours<Altitude> & contours)
  {
    auto const avoidSeam = lat == kAsterTilesLatTop || (lat == kAsterTilesLatBottom - 1);
    if (avoidSeam)
    {
      m_srtmProvider.SetPrefferedTile(ms::LatLon(lat == kAsterTilesLatTop ? lat - 0.5 : lat + 0.5,
                                                 lon));
      SeamlessAltitudeProvider seamlessAltProvider(m_srtmProvider, altProvider,
          [](ms::LatLon const & pos)
          {
            // In case when two altitudes sources are used for altitudes extraction,
            // for the same position on the border could be returned different altitudes.
            // Force to use altitudes near the srtm/aster border from srtm source,
            // it helps to avoid contours gaps due to different altitudes for equal positions.
            return fabs(pos.m_lat - kAsterTilesLatTop) < mercator::kPointEqualityEps ||
                   fabs(pos.m_lat - kAsterTilesLatBottom) < mercator::kPointEqualityEps;
          });
      GenerateContours(lat, lon, params, seamlessAltProvider, contours);
    }
    else
    {
      GenerateContours(lat, lon, params, altProvider, contours);
    }
  }

  void GenerateContours(int lat, int lon, TileIsolinesParams const & params,
                        ValuesProvider<Altitude> & altProvider, Contours<Altitude> & contours)
  {
    auto const leftBottom = ms::LatLon(lat, lon);
    auto const rightTop = ms::LatLon(lat + 1.0, lon + 1.0);
    auto const squaresStep = 1.0 / kArcSecondsInDegree * params.m_latLonStepFactor;

    MarchingSquares<Altitude> squares(leftBottom, rightTop,
                                      squaresStep, params.m_alitudesStep,
                                      altProvider, m_debugId);
    squares.GenerateContours(contours);
  }

  int m_left;
  int m_bottom;
  int m_right;
  int m_top;
  std::string m_strmDir;
  SrtmProvider m_srtmProvider;
  TileIsolinesParams const * m_params = nullptr;
  TileIsolinesProfileParams const * m_profileParams = nullptr;
  bool m_forceRegenerate;
  std::string m_debugId;
};

template <typename ParamsType>
void RunGenerateIsolinesTasks(int left, int bottom, int right, int top,
                              std::string const & srtmPath, ParamsType const & params,
                              long threadsCount, long maxCachedTilesPerThread,
                              bool forceRegenerate)
{
  std::vector<std::unique_ptr<TileIsolinesTask>> tasks;

  CHECK_GREATER(right, left, ());
  CHECK_GREATER(top, bottom, ());

  int tilesRowPerTask = top - bottom;
  int tilesColPerTask = right - left;

  if (tilesRowPerTask * tilesColPerTask <= threadsCount)
  {
    tilesRowPerTask = 1;
    tilesColPerTask = 1;
  }
  else
  {
    while (tilesRowPerTask * tilesColPerTask > maxCachedTilesPerThread)
    {
      if (tilesRowPerTask > tilesColPerTask)
        tilesRowPerTask = (tilesRowPerTask + 1) / 2;
      else
        tilesColPerTask = (tilesColPerTask + 1) / 2;
    }
  }

  base::thread_pool::computational::ThreadPool threadPool(threadsCount);

  for (int lat = bottom; lat < top; lat += tilesRowPerTask)
  {
    int const topLat = std::min(lat + tilesRowPerTask - 1, top - 1);
    for (int lon = left; lon < right; lon += tilesColPerTask)
    {
      int const rightLon = std::min(lon + tilesColPerTask - 1, right - 1);
      auto task = std::make_unique<TileIsolinesTask>(lon, lat, rightLon, topLat, srtmPath, &params,
                                                     forceRegenerate);
      threadPool.SubmitWork([task = std::move(task)](){ task->Do(); });
    }
  }
}
}  // namespace

Generator::Generator(std::string const & srtmPath, long threadsCount,
                     long maxCachedTilesPerThread, bool forceRegenerate)
  : m_threadsCount(threadsCount)
  , m_maxCachedTilesPerThread(maxCachedTilesPerThread)
  , m_srtmPath(srtmPath)
  , m_forceRegenerate(forceRegenerate)
{}

void Generator::GenerateIsolines(int left, int bottom, int right, int top,
                                 TileIsolinesParams const & params)
{
  RunGenerateIsolinesTasks(left, bottom, right, top, m_srtmPath, params,
                           m_threadsCount, m_maxCachedTilesPerThread, m_forceRegenerate);
}


void Generator::GenerateIsolines(int left, int bottom, int right, int top,
                                 std::string const & tilesProfilesDir)
{
  TileIsolinesProfileParams params(m_profileToTileParams, tilesProfilesDir);
  RunGenerateIsolinesTasks(left, bottom, right, top, m_srtmPath, params,
                           m_threadsCount, m_maxCachedTilesPerThread, m_forceRegenerate);
}

void Generator::GenerateIsolinesForCountries()
{
  if (!GetPlatform().IsFileExistsByFullPath(m_isolinesTilesOutDir) &&
      !GetPlatform().MkDirRecursively(m_isolinesTilesOutDir))
  {
    LOG(LERROR, ("Can't create directory", m_isolinesTilesOutDir));
    return;
  }

  std::set<std::string> checkedProfiles;
  for (auto const & countryParams : m_countriesToGenerate.m_countryParams)
  {
    auto const profileName = countryParams.second.m_profileName;
    if (checkedProfiles.find(profileName) != checkedProfiles.end())
      continue;
    checkedProfiles.insert(profileName);
    auto const profileTilesDir = GetTilesDir(m_isolinesTilesOutDir, profileName);
    if (!GetPlatform().IsFileExistsByFullPath(profileTilesDir) &&
        !GetPlatform().MkDirChecked(profileTilesDir))
    {
      LOG(LERROR, ("Can't create directory", profileTilesDir));
      return;
    }
  }

  auto const tmpTileProfilesDir = GetTileProfilesDir(m_isolinesTilesOutDir);

  Platform::RmDirRecursively(tmpTileProfilesDir);
  if (!GetPlatform().MkDirChecked(tmpTileProfilesDir))
  {
    LOG(LERROR, ("Can't create directory", tmpTileProfilesDir));
    return;
  }

  m2::RectI boundingRect;
  for (auto const & countryParams : m_countriesToGenerate.m_countryParams)
  {
    auto const & countryId = countryParams.first;
    auto const & params = countryParams.second;

    auto const countryFile = GetIsolinesFilePath(countryId, m_isolinesCountriesOutDir);

    if (!m_forceRegenerate && GetPlatform().IsFileExistsByFullPath(countryFile))
    {
      LOG(LINFO, ("Isolines for", countryId, "are ready, skip processing."));
      continue;
    }

    m2::RectD countryRect;
    std::vector<m2::RegionD> countryRegions;
    GetCountryRegions(countryId, countryRect, countryRegions);

    for (auto const & region : countryRegions)
    {
      countryRect = region.GetRect();

      int left, bottom, right, top;
      MercatorRectToTilesRange(countryRect, left, bottom, right, top);

      boundingRect.Add(m2::PointI(left, bottom));
      boundingRect.Add(m2::PointI(right, top));

      for (int lat = bottom; lat <= top; ++lat)
      {
        for (int lon = left; lon <= right; ++lon)
        {
          if (params.NeedSkipTile(lat, lon))
            continue;
          auto const tileProfilesFilePath = GetTileProfilesFilePath(lat, lon, tmpTileProfilesDir);
          AppendTileProfile(tileProfilesFilePath, params.m_profileName);
        }
      }
    }
  }

  if (!boundingRect.IsValid())
    return;

  LOG(LINFO, ("Generate isolines for tiles rect", boundingRect));

  GenerateIsolines(boundingRect.LeftBottom().x, boundingRect.LeftBottom().y,
                   boundingRect.RightTop().x + 1, boundingRect.RightTop().y + 1, tmpTileProfilesDir);
}

void Generator::PackIsolinesForCountry(storage::CountryId const & countryId,
                                       IsolinesPackingParams const & params)
{
  PackIsolinesForCountry(countryId, params, nullptr /*needSkipTileFn*/);
}

void Generator::PackIsolinesForCountry(storage::CountryId const & countryId,
                                       IsolinesPackingParams const & params,
                                       NeedSkipTileFn const & needSkipTileFn)
{
  auto const outFile = GetIsolinesFilePath(countryId, params.m_outputDir);

  if (!m_forceRegenerate && GetPlatform().IsFileExistsByFullPath(outFile))
  {
    LOG(LINFO, ("Isolines for", countryId, "are ready, skip processing."));
    return;
  }

  // TODO : prepare simplified and filtered isolones for all geom levels here
  // (ATM its the most detailed geom3 only) instead of in the generator
  // to skip re-doing it for every maps gen. And it'll be needed anyway
  // for the longer term vision to supply isolines in separately downloadable files.
  LOG(LINFO, ("Begin packing isolines for country", countryId));

  m2::RectD countryRect;
  std::vector<m2::RegionD> countryRegions;
  GetCountryRegions(countryId, countryRect, countryRegions);

  int left, bottom, right, top;
  MercatorRectToTilesRange(countryRect, left, bottom, right, top);

  Contours<Altitude> countryIsolines;
  countryIsolines.m_minValue = std::numeric_limits<Altitude>::max();
  countryIsolines.m_maxValue = std::numeric_limits<Altitude>::min();

  for (int lat = bottom; lat <= top; ++lat)
  {
    for (int lon = left; lon <= right; ++lon)
    {
      if (needSkipTileFn && needSkipTileFn(lat, lon))
        continue;

      Contours<Altitude> isolines;
      auto const tileFilePath = GetIsolinesFilePath(lat, lon, params.m_isolinesTilesPath);
      if (!LoadContours(tileFilePath, isolines))
        continue;

      LOG(LINFO, ("Begin packing isolines from tile", tileFilePath));

      CropContours(countryRect, countryRegions, params.m_maxIsolineLength,
                   params.m_alitudesStepFactor, isolines);
      // Simplification is done already while processing tiles in ProcessTile().
      // But now a different country-specific simpificationZoom could be applied.
      if (params.m_simplificationZoom > 0)
        SimplifyContours(params.m_simplificationZoom, isolines);

      countryIsolines.m_minValue = std::min(isolines.m_minValue, countryIsolines.m_minValue);
      countryIsolines.m_maxValue = std::max(isolines.m_maxValue, countryIsolines.m_maxValue);
      countryIsolines.m_valueStep = isolines.m_valueStep;
      countryIsolines.m_invalidValuesCount += isolines.m_invalidValuesCount;

      for (auto & levelIsolines : isolines.m_contours)
      {
        auto & dst = countryIsolines.m_contours[levelIsolines.first];
        std::move(levelIsolines.second.begin(), levelIsolines.second.end(),
                  std::back_inserter(dst));
      }

      LOG(LINFO, ("End packing isolines from tile", tileFilePath));
    }
  }

  LOG(LINFO, ("End packing isolines for country", countryId,
              "min altitude", countryIsolines.m_minValue,
              "max altitude", countryIsolines.m_maxValue));

  SaveContrours(outFile, std::move(countryIsolines));

  LOG(LINFO, ("Isolines saved to", outFile));
}

void Generator::PackIsolinesForCountries()
{
  if (!GetPlatform().IsFileExistsByFullPath(m_isolinesCountriesOutDir) &&
      !GetPlatform().MkDirRecursively(m_isolinesCountriesOutDir))
  {
    LOG(LERROR, ("Can't create directory", m_isolinesCountriesOutDir));
    return;
  }

  base::thread_pool::computational::ThreadPool threadPool(m_threadsCount);
  size_t taskInd = 0;
  size_t tasksCount = m_countriesToGenerate.m_countryParams.size();
  for (auto const & countryParams : m_countriesToGenerate.m_countryParams)
  {
    auto const & countryId = countryParams.first;
    auto const & params = countryParams.second;

    threadPool.SubmitWork([this, countryId, taskInd, tasksCount, params]()
    {
      LOG(LINFO, ("Begin task", taskInd, "/", tasksCount, countryId));

      auto const & packingParams = m_profileToPackingParams.at(params.m_profileName);
      PackIsolinesForCountry(countryId, packingParams,
                             [&params](int lat, int lon){ return params.NeedSkipTile(lat, lon); });

      LOG(LINFO, ("End task", taskInd, "/", tasksCount, countryId));
    });
    ++taskInd;
  }
}

void Generator::InitCountryInfoGetter(std::string const & dataDir)
{
  CHECK(m_infoReader == nullptr, ());

  GetPlatform().SetResourceDir(dataDir);

  m_infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(m_infoGetter, ());
  m_infoReader = static_cast<storage::CountryInfoReader *>(m_infoGetter.get());
}

void Generator::InitProfiles(std::string const & isolinesProfilesFileName,
                             std::string const & countriesToGenerateFileName,
                             std::string const & isolinesTilesOutDir,
                             std::string const & isolinesCountriesOutDir)
{
  CHECK(Deserialize(isolinesProfilesFileName, m_profilesCollection), ());
  CHECK(Deserialize(countriesToGenerateFileName, m_countriesToGenerate), ());

  auto const & profiles = m_profilesCollection.m_profiles;
  for (auto const & countryParams : m_countriesToGenerate.m_countryParams)
  {
    auto const & params = countryParams.second;
    CHECK(profiles.find(params.m_profileName) != profiles.end(),
          ("Unknown profile name", params.m_profileName));
  }

  m_isolinesTilesOutDir = isolinesTilesOutDir;
  m_isolinesCountriesOutDir = isolinesCountriesOutDir;

  for (auto const & profile : m_profilesCollection.m_profiles)
  {
    auto const & profileName = profile.first;
    auto const & profileParams = profile.second;

    TileIsolinesParams tileParams;
    tileParams.m_outputDir = GetTilesDir(isolinesTilesOutDir, profileName);
    tileParams.m_latLonStepFactor = profileParams.m_latLonStepFactor;
    tileParams.m_alitudesStep = profileParams.m_alitudesStep;
    tileParams.m_simplificationZoom = profileParams.m_simplificationZoom;
    if (profileParams.m_medianFilterR > 0)
      tileParams.m_filters.emplace_back(std::make_unique<MedianFilter<Altitude>>(profileParams.m_medianFilterR));
    if (profileParams.m_gaussianFilterStDev > 0.0 && profileParams.m_gaussianFilterRFactor > 0)
    {
      tileParams.m_filters.emplace_back(
        std::make_unique<GaussianFilter<Altitude>>(profileParams.m_gaussianFilterStDev,
                                                   profileParams.m_gaussianFilterRFactor));
    }
    m_profileToTileParams.emplace(profileName, std::move(tileParams));

    IsolinesPackingParams packingParams;
    packingParams.m_outputDir = isolinesCountriesOutDir;
    packingParams.m_simplificationZoom = 0;
    packingParams.m_alitudesStepFactor = 1;
    packingParams.m_isolinesTilesPath = GetTilesDir(isolinesTilesOutDir, profileName);
    packingParams.m_maxIsolineLength = profileParams.m_maxIsolinesLength;
    m_profileToPackingParams.emplace(profileName, std::move(packingParams));
  }
}

void Generator::GetCountryRegions(storage::CountryId const & countryId, m2::RectD & countryRect,
                                  std::vector<m2::RegionD> & countryRegions)
{
  countryRect = m_infoReader->GetLimitRectForLeaf(countryId);

  size_t id;
  for (id = 0; id < m_infoReader->GetCountries().size(); ++id)
  {
    if (m_infoReader->GetCountries().at(id).m_countryId == countryId)
      break;
  }
  CHECK_LESS(id, m_infoReader->GetCountries().size(), ());

  m_infoReader->LoadRegionsFromDisk(id, countryRegions);
}
}  // namespace topography_generator
