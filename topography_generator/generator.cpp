#include "topography_generator/generator.hpp"
#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/marching_squares/marching_squares.hpp"
#include "topography_generator/utils/contours_serdes.hpp"

#include "platform/platform.hpp"

#include "generator/srtm_parser.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <vector>

namespace topography_generator
{
double const kEps = 1e-7;
size_t constexpr kArcSecondsInDegree = 60 * 60;
int constexpr kAsterTilesLatTop = 60;
int constexpr kAsterTilesLatBottom = -60;

class SrtmProvider : public ValuesProvider<Altitude>
{
public:
  explicit SrtmProvider(std::string const & srtmDir):
    m_srtmManager(srtmDir)
  {}

  Altitude GetValue(ms::LatLon const & pos) override
  {
    auto const leftEdge = pos.m_lon - floor(pos.m_lon) < kEps;
    auto const bottomEdge = pos.m_lat - floor(pos.m_lat) < kEps;
    if ((leftEdge || bottomEdge) && !m_srtmManager.HasValidTile(pos))
    {
      // Each SRTM tile overlaps the top row in the bottom tile and the right row in the left tile.
      // Try to prevent loading a new tile if the position can be found in the loaded ones.
      if (leftEdge)
      {
        auto const shiftedPos = ms::LatLon(pos.m_lat, pos.m_lon - kEps);
        if (m_srtmManager.HasValidTile(shiftedPos))
          return GetSafeValue(shiftedPos);
      }
      if (bottomEdge)
      {
        auto const shiftedPos = ms::LatLon(pos.m_lat - kEps, pos.m_lon);
        if (m_srtmManager.HasValidTile(shiftedPos))
          return GetSafeValue(shiftedPos);
      }
      auto const shiftedPos = ms::LatLon(pos.m_lat - kEps, pos.m_lon - kEps);
      if (m_srtmManager.HasValidTile(shiftedPos))
        return GetSafeValue(shiftedPos);
    }
    return GetSafeValue(pos);
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

private:
  Altitude GetSafeValue(ms::LatLon const & pos)
  {
    auto const alt = m_srtmManager.GetHeight(pos);
    if (alt != kInvalidAltitude)
      return alt;

    if (m_srtmManager.HasValidTile(pos))
      return GetMedianValue(pos);

    return kInvalidAltitude;
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
          auto const alt = m_srtmManager.GetHeight({pos.m_lat + i * step, pos.m_lon + j * step});
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
    CHECK(ix < m_values.size(), ());

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
      return m_originalProvider.GetValue(movedPos);
    return m_filteredProvider.GetValue(pos);
  }

  Altitude GetInvalidValue() const override { return kInvalidAltitude; }

private:
  ValuesProvider<Altitude> & m_originalProvider;
  ValuesProvider<Altitude> & m_filteredProvider;
  MoveFromBorderFn m_moveFromBorderFn;
};

class TileIsolinesTask : public threads::IRoutine
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

  void Do() override
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
    auto const tileName = generator::SrtmTile::GetBase(ms::LatLon(lat, lon));
    if (!GetPlatform().IsFileExistsByFullPath(generator::SrtmTile::GetPath(m_strmDir, tileName)))
    {
      LOG(LINFO, ("SRTM tile", tileName, "doesn't exist, skip processing."));
      return;
    }

    LOG(LINFO, ("Begin generating isolines for tile", tileName));

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

    SaveContrours(GetIsolinesFilePath(lat, lon, m_params.m_outputDir), std::move(contours));

    LOG(LINFO, ("End generating isolines for tile", tileName));
  }

  void GenerateSeamlessContours(int lat, int lon, ValuesProvider<Altitude> & altProvider,
                                Contours<Altitude> & contours)
  {
    auto const avoidSeam = lat == kAsterTilesLatTop || lat == kAsterTilesLatBottom;
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
    ms::LatLon const leftBottom = ms::LatLon(lat, lon);
    ms::LatLon const rightTop = ms::LatLon(lat + 1.0, lon + 1.0);
    double const squaresStep = 1.0 / (kArcSecondsInDegree) * m_params.m_latLonStepFactor;

    MarchingSquares<Altitude> squares(leftBottom, rightTop,
                                      squaresStep, m_params.m_alitudesStep,
                                      altProvider);
    squares.GenerateContours(contours);
  }

  int m_left;
  int m_bottom;
  int m_right;
  int m_top;
  std::string m_strmDir;
  SrtmProvider m_srtmProvider;
  TileIsolinesParams const & m_params;
};

Generator::Generator(std::string const & srtmPath, size_t threadsCount, size_t maxCachedTilesPerThread)
  : m_threadsCount(threadsCount)
  , m_maxCachedTilesPerThread(maxCachedTilesPerThread)
  , m_srtmPath(srtmPath)
{
  m_infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(m_infoGetter, ());
  m_infoReader = static_cast<storage::CountryInfoReader *>(m_infoGetter.get());

  m_threadsPool = std::make_unique<base::thread_pool::routine::ThreadPool>(
    threadsCount, std::bind(&Generator::OnTaskFinished, this, std::placeholders::_1));
}

Generator::~Generator()
{
  m_threadsPool->Stop();
}

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

  for (int lat = bottom; lat < top; lat += tilesRowPerTask)
  {
    int const topLat = std::min(lat + tilesRowPerTask - 1, top - 1);
    for (int lon = left; lon < right; lon += tilesColPerTask)
    {
      int const rightLon = std::min(lon + tilesColPerTask - 1, right - 1);
      tasks.emplace_back(std::make_unique<TileIsolinesTask>(lon, lat, rightLon, topLat,
                                                            m_srtmPath, params));
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    CHECK(m_activeTasksCount == 0, ());
    m_activeTasksCount = tasks.size();
  }

  for (auto & task : tasks)
    m_threadsPool->PushBack(task.get());

  std::unique_lock<std::mutex> lock(m_tasksMutex);
  m_tasksReadyCondition.wait(lock, [this] { return m_activeTasksCount == 0; });
}

void Generator::OnTaskFinished(threads::IRoutine * task)
{
  {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    CHECK(m_activeTasksCount > 0, ());
    --m_activeTasksCount;
    if (m_activeTasksCount == 0)
      m_tasksReadyCondition.notify_one();
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

void Generator::PackIsolinesForCountry(storage::CountryId const & countryId,
                                       std::string const & isolinesPath,
                                       std::string const & outDir,
                                       CountryIsolinesParams const & params)
{
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
      if (!LoadContours(GetIsolinesFilePath(lat, lon, isolinesPath), isolines))
        continue;

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
    }
  }

  SaveContrours(GetIsolinesFilePath(countryId, outDir), std::move(countryIsolines));
}
}  // namespace topography_generator
