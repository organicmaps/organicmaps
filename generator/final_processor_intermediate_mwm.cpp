#include "generator/final_processor_intermediate_mwm.hpp"

#include "generator/affiliation.hpp"
#include "generator/booking_dataset.hpp"
#include "generator/city_boundary_processor.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_merger.hpp"
#include "generator/promo_catalog_cities.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"
#include "base/thread_pool_delayed.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iterator>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "defines.hpp"

using namespace base::thread_pool;
using namespace feature;
using namespace serialization_policy;

namespace generator
{
namespace
{
template <typename ToDo>
void ForEachCountry(std::string const & temproryMwmPath, ToDo && toDo)
{
  Platform::FilesList fileList;
  Platform::GetFilesByExt(temproryMwmPath, DATA_FILE_EXTENSION_TMP, fileList);
  for (auto const & filename : fileList)
    toDo(filename);
}

std::vector<std::vector<std::string>> GetAffilations(std::vector<FeatureBuilder> const & fbs,
                                                     AffiliationInterface const & affilation,
                                                     size_t threadsCount)
{
  computational::ThreadPool pool(threadsCount);
  std::vector<std::future<std::vector<std::string>>> futuresAffilations;
  for (auto const & fb : fbs)
  {
    auto result = pool.Submit([&]() {
      return affilation.GetAffiliations(fb);
    });
    futuresAffilations.emplace_back(std::move(result));
  }

  std::vector<std::vector<std::string>> resultAffilations;
  resultAffilations.reserve(futuresAffilations.size());
  for (auto & f : futuresAffilations)
    resultAffilations.emplace_back(f.get());

  return resultAffilations;
}

template <class SerializationPolicy = MaxAccuracy>
std::vector<std::vector<std::string>> AppendToCountries(std::vector<FeatureBuilder> const & fbs,
                                                        std::string const & temproryMwmPath,
                                                        AffiliationInterface const & affilation,
                                                        size_t threadsCount)
{
  auto const affilations = GetAffilations(fbs, affilation, threadsCount);
  std::unordered_map<std::string, std::vector<FeatureBuilder>> countryToCities;
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    for (auto const & country : affilations[i])
      countryToCities[country].emplace_back(fbs[i]);
  }

  delayed::ThreadPool pool(threadsCount, delayed::ThreadPool::Exit::ExecPending);
  for (auto & p : countryToCities)
  {
    pool.Push([temproryMwmPath, name{p.first}, countries{std::move(p.second)}]() {
      auto const path = base::JoinPath(temproryMwmPath, name + DATA_FILE_EXTENSION_TMP);
      FeatureBuilderWriter<SerializationPolicy> collector(path, FileWriter::Op::OP_APPEND);
      for (auto && fb : countries)
        collector.Write(std::move(fb));
    });
  }

  return affilations;
}

void Sort(std::vector<FeatureBuilder> & fbs)
{
  std::sort(std::begin(fbs), std::end(fbs), [](auto const & l, auto const & r) {
    auto const lGeomType = static_cast<int8_t>(l.GetGeomType());
    auto const rGeomType = static_cast<int8_t>(r.GetGeomType());

    auto const lId = l.HasOsmIds() ? l.GetMostGenericOsmId() : base::GeoObjectId();
    auto const rId = r.HasOsmIds() ? r.GetMostGenericOsmId() : base::GeoObjectId();

    auto const lPointCount = l.GetPointsCount();
    auto const rPointCount = r.GetPointsCount();

    auto const lKeyPoint = l.GetKeyPoint();
    auto const rKeyPoint = r.GetKeyPoint();

    return std::tie(lGeomType, lId, lPointCount, lKeyPoint) < std::tie(rGeomType, rId, rPointCount, rKeyPoint);
  });
}

bool FilenameIsCountry(std::string filename, AffiliationInterface const & affilation)
{
  strings::ReplaceLast(filename, DATA_FILE_EXTENSION_TMP, "");
  return affilation.HasRegionByName(filename);
}

class CityBoundariesHelper
{
public:
  CityBoundariesHelper()
    : m_table(std::make_shared<OsmIdToBoundariesTable>())
    , m_processor(m_table)
  {
  }

  explicit CityBoundariesHelper(std::string const & filename)
    : CityBoundariesHelper()
  {
    ForEachFromDatRawFormat<MaxAccuracy>(filename, [&](auto const & fb, auto const &) {
      m_processor.Add(fb);
    });
  }

  static bool IsPlace(FeatureBuilder const & fb)
  {
    auto const type = GetPlaceType(fb);
    return type != ftype::GetEmptyValue()  && !fb.GetName().empty();
  }

  bool Process(FeatureBuilder const & fb)
  {
    if (!IsPlace(fb))
      return false;

    m_processor.Replace(fb);
    return true;
  }

  std::vector<FeatureBuilder> GetFeatures() const
  {
    return m_processor.GetFeatures();
  }

  std::shared_ptr<OsmIdToBoundariesTable> GetTable() const
  {
    return m_table;
  }

private:
  std::shared_ptr<OsmIdToBoundariesTable> m_table;
  CityBoundaryProcessor m_processor;
};

class ProcessorCities
{
public:
  ProcessorCities(std::string const & temproryMwmPath, AffiliationInterface const & affilation,
                  CityBoundariesHelper & cityBoundariesHelper, size_t threadsCount = 1)
    : m_temproryMwmPath(temproryMwmPath)
    , m_affilation(affilation)
    , m_cityBoundariesHelper(cityBoundariesHelper)
    , m_threadsCount(threadsCount)
  {
  }

  void SetPromoCatalog(std::string const & filename)
  {
    m_citiesFinename = filename;
  }

  bool Process()
  {
    std::vector<std::future<std::vector<FeatureBuilder>>> citiesResults;
    computational::ThreadPool pool(m_threadsCount);
    ForEachCountry(m_temproryMwmPath, [&](auto const & filename) {
      auto cities = pool.Submit([&, filename]() {
        std::vector<FeatureBuilder> cities;
        if (!FilenameIsCountry(filename, m_affilation))
          return cities;

        auto const fullPath = base::JoinPath(m_temproryMwmPath, filename);
        auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
        FeatureBuilderWriter<MaxAccuracy> writer(fullPath);
        for (size_t i = 0; i < fbs.size(); ++i)
        {
          if (CityBoundariesHelper::IsPlace(fbs[i]))
            cities.emplace_back(std::move(fbs[i]));
          else
            writer.Write(std::move(fbs[i]));
        }

        return cities;
      });
      citiesResults.emplace_back(std::move(cities));
    });

    for (auto & v : citiesResults)
    {
      auto const cities = v.get();
      for (auto const & city : cities)
        m_cityBoundariesHelper.Process(city);
    }

    auto fbs = m_cityBoundariesHelper.GetFeatures();
    if (!m_citiesFinename.empty())
      ProcessForPromoCatalog(fbs);

    AppendToCountries(fbs, m_temproryMwmPath, m_affilation, m_threadsCount);
    return true;
  }

private:
  void ProcessForPromoCatalog(std::vector<FeatureBuilder> & fbs)
  {
    auto const cities = promo::LoadCities(m_citiesFinename);
    for (auto & fb : fbs)
    {
      if (!ftypes::IsCityTownOrVillage(fb.GetTypes()) || cities.count(fb.GetMostGenericOsmId()) == 0)
        continue;

      auto static const kPromoType = classif().GetTypeByPath({"sponsored", "promo_catalog"});
      FeatureParams & params = fb.GetParams();
      params.AddType(kPromoType);
    }
  }

  std::string m_citiesFinename;
  std::string m_temproryMwmPath;
  AffiliationInterface const & m_affilation;
  CityBoundariesHelper & m_cityBoundariesHelper;
  size_t m_threadsCount;
};
}  // namespace

bool FinalProcessorIntermediateMwmInteface::operator<(FinalProcessorIntermediateMwmInteface const & other) const
{
  return m_priority < other.m_priority;
}

bool FinalProcessorIntermediateMwmInteface::operator==(FinalProcessorIntermediateMwmInteface const & other) const
{
  return !(*this < other || other < *this);
}

bool FinalProcessorIntermediateMwmInteface::operator!=(FinalProcessorIntermediateMwmInteface const & other) const
{
  return !(*this == other);
}

CountryFinalProcessor::CountryFinalProcessor(std::string const & borderPath,
                                             std::string const & temproryMwmPath,
                                             bool haveBordersForWholeWorld,
                                             size_t threadsCount)
  : FinalProcessorIntermediateMwmInteface(FinalProcessorPriority::COUNTRIES_OR_WORLD)
  , m_borderPath(borderPath)
  , m_temproryMwmPath(temproryMwmPath)
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
  , m_threadsCount(threadsCount)
{
}

void CountryFinalProcessor::NeedBookig(std::string const & filename)
{
  m_hotelsPath = filename;
}

void CountryFinalProcessor::UseCityBoundaries(std::string const & filename)
{
  m_cityBoundariesTmpFilename = filename;
}

void CountryFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFinename = filename;
}

void CountryFinalProcessor::DumpCityBoundaries(std::string const & filename)
{
  m_citiesBoundariesFilename = filename;
}

void CountryFinalProcessor::AddCoastlines(std::string const & coastlineGeomFilename,
                                          std::string const & worldCoastsFilename)
{
  m_coastlineGeomFilename = coastlineGeomFilename;
  m_worldCoastsFilename = worldCoastsFilename;
}

bool CountryFinalProcessor::Process()
{
  if (!m_hotelsPath.empty() && !ProcessBooking())
    return false;

  if ((!m_cityBoundariesTmpFilename.empty() || !m_citiesFinename.empty()) && !ProcessCities())
    return false;

  if (!m_coastlineGeomFilename.empty() && !ProcessCoasline())
    return false;

  return CleanUp();
}

bool CountryFinalProcessor::ProcessBooking()
{
  BookingDataset dataset(m_hotelsPath);
  auto const affilation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  {
    delayed::ThreadPool pool(m_threadsCount, delayed::ThreadPool::Exit::ExecPending);
    ForEachCountry(m_temproryMwmPath, [&](auto const & filename) {
      pool.Push([&, filename]() {
        std::vector<FeatureBuilder> cities;
        if (!FilenameIsCountry(filename, affilation))
          return;

        auto const fullPath = base::JoinPath(m_temproryMwmPath, filename);
        auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
        FeatureBuilderWriter<MaxAccuracy> writer(fullPath);
        for (auto & fb : fbs)
        {
          auto const id = dataset.FindMatchingObjectId(fb);
          if (id == BookingHotel::InvalidObjectId())
          {
            writer.Write(fb);
          }
          else
          {
            dataset.PreprocessMatchedOsmObject(id, fb, [&](FeatureBuilder & newFeature) {
              writer.Write(newFeature);
            });
          }
        }
      });
    });
  }
  std::vector<FeatureBuilder> fbs;
  dataset.BuildOsmObjects([&](auto && fb) {
    fbs.emplace_back(std::move(fb));
  });
  AppendToCountries(fbs, m_temproryMwmPath, affilation, m_threadsCount);
  return true;
}

bool CountryFinalProcessor::ProcessCities()
{
  auto const affilation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto cityBoundariesHelper = m_cityBoundariesTmpFilename.empty()
                              ? CityBoundariesHelper()
                              : CityBoundariesHelper(m_cityBoundariesTmpFilename);
  ProcessorCities processorCities(m_temproryMwmPath, affilation, cityBoundariesHelper, m_threadsCount);
  processorCities.SetPromoCatalog(m_citiesFinename);
  if (!processorCities.Process())
    return false;

  if (!m_citiesBoundariesFilename.empty())
  {
    auto const cityBoundariesTable = cityBoundariesHelper.GetTable();
    LOG(LINFO, ("Dumping cities boundaries to", m_citiesBoundariesFilename));
    if (!SerializeBoundariesTable(m_citiesBoundariesFilename, *cityBoundariesTable))
    {
      LOG(LINFO, ("Error serializing boundaries table to", m_citiesBoundariesFilename));
      return false;
    }
  }

  return true;
}

bool CountryFinalProcessor::ProcessCoasline()
{
  auto const affilation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto fbs = ReadAllDatRawFormat(m_coastlineGeomFilename);
  auto const affilations = AppendToCountries(fbs, m_temproryMwmPath, affilation, m_threadsCount);
  FeatureBuilderWriter<> collector(m_worldCoastsFilename);
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    fbs[i].AddName("default", strings::JoinStrings(affilations[i], ";"));
    collector.Write(fbs[i]);
  }

  return true;
}

bool CountryFinalProcessor::CleanUp()
{
  auto const affilation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  delayed::ThreadPool pool(m_threadsCount, delayed::ThreadPool::Exit::ExecPending);
  ForEachCountry(m_temproryMwmPath, [&](auto const & filename) {
    pool.Push([&, filename]() {
      if (!FilenameIsCountry(filename, affilation))
        return;

      auto const fullPath = base::JoinPath(m_temproryMwmPath, filename);
      auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
      Sort(fbs);
      FeatureBuilderWriter<> collector(fullPath);
      for (auto & fb : fbs)
        collector.Write(fb);
    });
  });

  return true;
}

WorldFinalProcessor::WorldFinalProcessor(std::string const & temproryMwmPath,
                                         std::string const & coastlineGeomFilename,
                                         std::string const & popularPlacesFilename)
  : FinalProcessorIntermediateMwmInteface(FinalProcessorPriority::COUNTRIES_OR_WORLD)
  , m_temproryMwmPath(temproryMwmPath)
  , m_worldTmpFilename(base::JoinPath(m_temproryMwmPath, WORLD_FILE_NAME) + DATA_FILE_EXTENSION_TMP)
  , m_coastlineGeomFilename(coastlineGeomFilename)
  , m_popularPlacesFilename(popularPlacesFilename)
{
}

void WorldFinalProcessor::UseCityBoundaries(std::string const & filename)
{
  m_cityBoundariesTmpFilename = filename;
}

void WorldFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFinename = filename;
}

bool WorldFinalProcessor::Process()
{
  if ((!m_cityBoundariesTmpFilename.empty() || !m_citiesFinename.empty()) && !ProcessCities())
    return false;

  auto fbs = ReadAllDatRawFormat<MaxAccuracy>(m_worldTmpFilename);
  Sort(fbs);
  WorldGenerator generator(m_worldTmpFilename, m_coastlineGeomFilename, m_popularPlacesFilename);
  for (auto & fb : fbs)
    generator.Process(fb);

  generator.DoMerge();
  return true;
}

bool WorldFinalProcessor::ProcessCities()
{
  auto const affilation = SingleAffiliation(WORLD_FILE_NAME);
  auto cityBoundariesHelper = m_cityBoundariesTmpFilename.empty()
                              ? CityBoundariesHelper()
                              : CityBoundariesHelper(m_cityBoundariesTmpFilename);
  ProcessorCities processorCities(m_temproryMwmPath, affilation, cityBoundariesHelper);
  processorCities.SetPromoCatalog(m_citiesFinename);
  return processorCities.Process();
}

CoastlineFinalProcessor::CoastlineFinalProcessor(std::string const & filename)
  : FinalProcessorIntermediateMwmInteface(FinalProcessorPriority::WORLDCOASTS)
  , m_filename(filename)
{
}

void CoastlineFinalProcessor::SetCoastlineGeomFilename(std::string const & filename)
{
  m_coastlineGeomFilename = filename;
}

void CoastlineFinalProcessor::SetCoastlineRawGeomFilename(std::string const & filename)
{
  m_coastlineRawGeomFilename = filename;
}

bool CoastlineFinalProcessor::Process()
{
  auto fbs = ReadAllDatRawFormat<MaxAccuracy>(m_filename);
  Sort(fbs);
  for (auto && fb : fbs)
    m_generator.Process(std::move(fb));

  FeaturesAndRawGeometryCollector collector(m_coastlineGeomFilename, m_coastlineRawGeomFilename);
  // Check and stop if some coasts were not merged.
  if (!m_generator.Finish())
    return false;

  LOG(LINFO, ("Generating coastline polygons."));
  size_t totalFeatures = 0;
  size_t totalPoints = 0;
  size_t totalPolygons = 0;
  vector<FeatureBuilder> outputFbs;
  m_generator.GetFeatures(outputFbs);
  for (auto & fb : outputFbs)
  {
    collector.Collect(fb);
    ++totalFeatures;
    totalPoints += fb.GetPointsCount();
    totalPolygons += fb.GetPolygonsCount();
  }

  LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons,
              "total points:", totalPoints));
  return true;
}
}  // namespace generator
