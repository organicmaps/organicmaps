#pragma once

#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/tile_filter.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"

#include "base/thread_pool.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

namespace topography_generator
{
struct TileIsolinesParams
{
  Altitude m_alitudesStep = 10;
  size_t m_latLonStepFactor = 1;
  FiltersSequence<Altitude> m_filters;
  std::string m_outputDir;
};

struct CountryIsolinesParams
{
  size_t m_maxIsolineLength = 1000;
  int m_simplificationZoom = 17;
  size_t m_alitudesStepFactor = 1;
};

class Generator
{
public:
  Generator(std::string const & srtmPath, size_t threadsCount, size_t maxCachedTilesPerThread);
  ~Generator();

  void GenerateIsolines(int left, int bottom, int right, int top,
                        TileIsolinesParams const & params);

  void PackIsolinesForCountry(storage::CountryId const & countryId,
                              std::string const & isolinesPath,
                              std::string const & outDir, CountryIsolinesParams const & params);

private:
  void OnTaskFinished(threads::IRoutine * task);
  void GetCountryRegions(storage::CountryId const & countryId, m2::RectD & countryRect,
                         std::vector<m2::RegionD> & countryRegions);

  std::unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  storage::CountryInfoReader * m_infoReader = nullptr;

  std::unique_ptr<base::thread_pool::routine::ThreadPool> m_threadsPool;
  size_t m_threadsCount;
  size_t m_maxCachedTilesPerThread;
  std::string m_srtmPath;
  std::mutex m_tasksMutex;
  std::condition_variable m_tasksReadyCondition;
  size_t m_activeTasksCount = 0;
};
}  // namespace topography_generator
