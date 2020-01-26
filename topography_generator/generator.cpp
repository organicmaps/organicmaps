#include "topography_generator/generator.hpp"
#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/marching_squares/marching_squares.hpp"
#include "topography_generator/utils/contours_serdes.hpp"

#include "platform/platform.hpp"

#include "generator/srtm_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <algorithm>
#include <vector>

namespace topography_generator
{
double const kEps = 1e-7;
double constexpr kTileSizeInDegree = 1.0;
size_t constexpr kArcSecondsInDegree = 60 * 60;
int constexpr kAsterTilesLatTop = 60;
int constexpr kAsterTilesLatBottom = -60;

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
    if (alt != kInvalidAltitude)
      return alt;
    return GetMedianValue(pos);
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

private:
  Altitude GetValueImpl(ms::LatLon const & pos)
  {
    if (m_preferredTile != nullptr)
    {
      // Each SRTM tile overlaps the top row in the bottom tile and the right row in the left tile.
      // Try to prevent loading a new tile if the position can be found in the loaded one.
      auto const latDist = pos.m_lat - m_leftBottomOfPreferredTile.m_lat;
      auto const lonDist = pos.m_lon - m_leftBottomOfPreferredTile.m_lon;
      if (latDist >= 0.0 && latDist <= kTileSizeInDegree &&
          lonDist >= 0.0 && lonDist <= kTileSizeInDegree)
      {
        return m_preferredTile->GetHeight(pos);
      }
    }

    return m_srtmManager.GetHeight(pos);
  }

  Altitude GetMedianValue(ms::LatLon const & pos)
  {
    if (!m_srtmManager.GetTile(pos).IsValid())
      return kInvalidAltitude;

    // Look around the position with invalid altitude
    // and return median of surrounding valid altitudes.
    double const step = kTileSizeInDegree / kArcSecondsInDegree;
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
          if (alt == kInvalidAltitude)
            continue;
          kernel.push_back(alt);
        }
      }
    }
    CHECK(!kernel.empty(), (pos));
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
  using MoveFromBorderFn = std::function<bool (ms::LatLon & pos)>;

  SeamlessAltitudeProvider(ValuesProvider<Altitude> & originalProvider,
                           ValuesProvider<Altitude> & filteredProvider,
                           MoveFromBorderFn && moveFromBorderFn)
    : m_originalProvider(originalProvider)
    , m_filteredProvider(filteredProvider)
    , m_moveFromBorderFn(std::move(moveFromBorderFn))
  {}

  Altitude GetValue(ms::LatLon const & pos) override
  {
    auto movedPos = pos;
    if (m_moveFromBorderFn(movedPos))
    {
      // Check that we have original neighboring tile, use filtered if haven't.
      auto const alt = m_originalProvider.GetValue(movedPos);
      if (alt != kInvalidAltitude)
        return alt;
    }
    return m_filteredProvider.GetValue(pos);
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

private:
  ValuesProvider<Altitude> & m_originalProvider;
  ValuesProvider<Altitude> & m_filteredProvider;
  MoveFromBorderFn m_moveFromBorderFn;
};

class TileIsolinesTask
{
public:
  TileIsolinesTask(int left, int bottom, int right, int top,
                   std::string const & srtmDir, TileIsolinesParams const & params)
    : m_left(left)
    , m_bottom(bottom)
    , m_right(right)
    , m_top(top)
    , m_strmDir(srtmDir)
    , m_srtmProvider(srtmDir)
    , m_params(params)
  {
    CHECK(right >= -180 && right <= 179, ());
    CHECK(left >= -180 && left <= 179, ());
    CHECK(top >= -90 && top <= 89, ());
    CHECK(bottom >= -90 && bottom <= 89, ());
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
  void ProcessTile(int lat, int lon)
  {
    auto const tileName = GetIsolinesTileBase(lat, lon);

    std::ostringstream os;
    os << tileName << " (" << lat << ", " << lon << ")";
    m_debugId = os.str();

    if (!GetPlatform().IsFileExistsByFullPath(generator::SrtmTile::GetPath(m_strmDir, tileName)))
    {
      LOG(LINFO, ("SRTM tile", tileName, "doesn't exist, skip processing."));
      return;
    }

    LOG(LINFO, ("Begin generating isolines for tile", tileName));

    m_srtmProvider.SetPrefferedTile({lat + kTileSizeInDegree / 2.0, lon + kTileSizeInDegree / 2.0});

    Contours<Altitude> contours;
    if (!m_params.m_filters.empty() && (lat >= kAsterTilesLatTop || lat < kAsterTilesLatBottom))
    {
      // Filter tiles converted from ASTER, cause they are noisy enough.
      std::vector<Altitude> filteredValues = FilterTile(m_params.m_filters,
                                                        ms::LatLon(lat, lon),
                                                        kArcSecondsInDegree,
                                                        kArcSecondsInDegree + 1,
                                                        m_srtmProvider);
      RawAltitudesTile filteredProvider(filteredValues, lon, lat);
      GenerateSeamlessContours(lat, lon, filteredProvider, contours);
    }
    else
    {
      GenerateContours(lat, lon, m_srtmProvider, contours);
    }

    LOG(LINFO, ("Isolines for tile", tileName, "min altitude", contours.m_minValue,
      "max altitude", contours.m_maxValue, "invalid values count", contours.m_invalidValuesCount));

    if (m_params.m_simplificationZoom > 0)
      SimplifyContours(m_params.m_simplificationZoom, contours);
    SaveContrours(GetIsolinesFilePath(lat, lon, m_params.m_outputDir), std::move(contours));

    LOG(LINFO, ("End generating isolines for tile", tileName));
  }

  void GenerateSeamlessContours(int lat, int lon, ValuesProvider<Altitude> & altProvider,
                                Contours<Altitude> & contours)
  {
    auto const avoidSeam = lat == kAsterTilesLatTop || (lat == kAsterTilesLatBottom - 1);
    if (avoidSeam)
    {
      SeamlessAltitudeProvider seamlessAltProvider(m_srtmProvider, altProvider,
                                                   [](ms::LatLon & pos)
      {
        // In case when two altitudes sources are used for altitudes extraction,
        // for the same position on the border could be returned different altitudes.
        // Force to use altitudes near the srtm/aster border from srtm source,
        // it helps to avoid contours gaps due to different altitudes for equal positions.
        if  (fabs(pos.m_lat - kAsterTilesLatTop) < kEps)
        {
          pos.m_lat -= kEps;
          return true;
        }
        if (fabs(pos.m_lat - kAsterTilesLatBottom) < kEps)
        {
          pos.m_lat += kEps;
          return true;
        }
        return false;
      });
      GenerateContours(lat, lon, seamlessAltProvider, contours);
    }
    else
    {
      GenerateContours(lat, lon, altProvider, contours);
    }
  }

  void GenerateContours(int lat, int lon, ValuesProvider<Altitude> & altProvider,
                        Contours<Altitude> & contours)
  {
    auto const leftBottom = ms::LatLon(lat, lon);
    auto const rightTop = ms::LatLon(lat + kTileSizeInDegree, lon + kTileSizeInDegree);
    auto const squaresStep = kTileSizeInDegree / kArcSecondsInDegree * m_params.m_latLonStepFactor;

    MarchingSquares<Altitude> squares(leftBottom, rightTop,
                                      squaresStep, m_params.m_alitudesStep,
                                      altProvider);
    squares.SetDebugId(m_debugId);
    squares.GenerateContours(contours);
  }

  int m_left;
  int m_bottom;
  int m_right;
  int m_top;
  std::string m_strmDir;
  SrtmProvider m_srtmProvider;
  TileIsolinesParams const & m_params;
  std::string m_debugId;
};

Generator::Generator(std::string const & srtmPath, size_t threadsCount, size_t maxCachedTilesPerThread)
  : m_threadsCount(threadsCount)
  , m_maxCachedTilesPerThread(maxCachedTilesPerThread)
  , m_srtmPath(srtmPath)
{}

void Generator::GenerateIsolines(int left, int bottom, int right, int top,
                                 TileIsolinesParams const & params)
{
  std::vector<std::unique_ptr<TileIsolinesTask>> tasks;

  CHECK_GREATER(right, left, ());
  CHECK_GREATER(top, bottom, ());

  int tilesRowPerTask = top - bottom;
  int tilesColPerTask = right - left;

  if (tilesRowPerTask * tilesColPerTask <= m_threadsCount)
  {
    tilesRowPerTask = 1;
    tilesColPerTask = 1;
  }
  else
  {
    while (tilesRowPerTask * tilesColPerTask > m_maxCachedTilesPerThread)
    {
      if (tilesRowPerTask > tilesColPerTask)
        tilesRowPerTask = (tilesRowPerTask + 1) / 2;
      else
        tilesColPerTask = (tilesColPerTask + 1) / 2;
    }
  }

  base::thread_pool::computational::ThreadPool threadPool(m_threadsCount);

  for (int lat = bottom; lat < top; lat += tilesRowPerTask)
  {
    int const topLat = std::min(lat + tilesRowPerTask - 1, top - 1);
    for (int lon = left; lon < right; lon += tilesColPerTask)
    {
      int const rightLon = std::min(lon + tilesColPerTask - 1, right - 1);
      auto task = std::make_unique<TileIsolinesTask>(lon, lat, rightLon, topLat, m_srtmPath, params);
      threadPool.SubmitWork([task = std::move(task)](){ task->Do(); });
    }
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

void Generator::PackIsolinesForCountry(storage::CountryId const & countryId,
                                       CountryIsolinesParams const & params,
                                       std::string const & outDir)
{
  LOG(LINFO, ("Begin packing isolines for country", countryId));

  m2::RectD countryRect;
  std::vector<m2::RegionD> countryRegions;
  GetCountryRegions(countryId, countryRect, countryRegions);

  auto const leftBottom = mercator::ToLatLon(countryRect.LeftBottom());
  auto const rightTop = mercator::ToLatLon(countryRect.RightTop());

  auto const left = static_cast<int>(floor(leftBottom.m_lon));
  auto const bottom = static_cast<int>(floor(leftBottom.m_lat));
  auto const right = static_cast<int>(floor(rightTop.m_lon));
  auto const top = static_cast<int>(floor(rightTop.m_lat));

  Contours<Altitude> countryIsolines;
  countryIsolines.m_minValue = std::numeric_limits<Altitude>::max();
  countryIsolines.m_maxValue = std::numeric_limits<Altitude>::min();

  for (int lat = bottom; lat <= top; ++lat)
  {
    for (int lon = left; lon <= right; ++lon)
    {
      Contours<Altitude> isolines;
      auto const tileFilePath = GetIsolinesFilePath(lat, lon, params.m_isolinesTilesPath);
      if (!LoadContours(tileFilePath, isolines))
        continue;

      LOG(LINFO, ("Begin packing isolines from tile", tileFilePath));

      CropContours(countryRect, countryRegions, params.m_maxIsolineLength,
                   params.m_alitudesStepFactor, isolines);
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

  auto const outFile = GetIsolinesFilePath(countryId, outDir);
  SaveContrours(outFile, std::move(countryIsolines));

  LOG(LINFO, ("Isolines saved to", outFile));
}
}  // namespace topography_generator
