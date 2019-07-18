#include "generator/final_processor_intermediate_mwm.hpp"

#include "generator/affiliation.hpp"
#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_merger.hpp"
#include "generator/place_processor.hpp"
#include "generator/promo_catalog_cities.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"

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

using namespace base::thread_pool::computational;
using namespace feature;
using namespace serialization_policy;

namespace generator
{
namespace
{
template <typename ToDo>
void ForEachCountry(std::string const & temporaryMwmPath, ToDo && toDo)
{
  Platform::FilesList fileList;
  Platform::GetFilesByExt(temporaryMwmPath, DATA_FILE_EXTENSION_TMP, fileList);
  for (auto const & filename : fileList)
    toDo(filename);
}

std::vector<std::vector<std::string>> GetAffiliations(std::vector<FeatureBuilder> const & fbs,
                                                      AffiliationInterface const & affiliation,
                                                      size_t threadsCount)
{
  ThreadPool pool(threadsCount);
  std::vector<std::future<std::vector<std::string>>> futuresAffiliations;
  for (auto const & fb : fbs)
  {
    auto result = pool.Submit([&]() {
      return affiliation.GetAffiliations(fb);
    });
    futuresAffiliations.emplace_back(std::move(result));
  }

  std::vector<std::vector<std::string>> resultAffiliations;
  resultAffiliations.reserve(futuresAffiliations.size());
  for (auto & f : futuresAffiliations)
    resultAffiliations.emplace_back(f.get());

  return resultAffiliations;
}

// Writes |fbs| to countries tmp.mwm files that |fbs| belongs to according to |affiliations|.
template <class SerializationPolicy = MaxAccuracy>
void AppendToCountries(std::vector<FeatureBuilder> const & fbs,
                       std::vector<std::vector<std::string>> const & affiliations,
                       std::string const & temporaryMwmPath, size_t threadsCount)
{
  std::unordered_map<std::string, std::vector<size_t>> countryToFbsIndexes;
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    for (auto const & country : affiliations[i])
      countryToFbsIndexes[country].emplace_back(i);
  }

  ThreadPool pool(threadsCount);
  for (auto && p : countryToFbsIndexes)
  {
    pool.SubmitWork([&, country{std::move(p.first)}, indexes{std::move(p.second)}]() {
      auto const path = base::JoinPath(temporaryMwmPath, country + DATA_FILE_EXTENSION_TMP);
      FeatureBuilderWriter<SerializationPolicy> collector(path, FileWriter::Op::OP_APPEND);
      for (auto const index : indexes)
        collector.Write(fbs[index]);
    });
  }
}

void Sort(std::vector<FeatureBuilder> & fbs)
{
  std::sort(std::begin(fbs), std::end(fbs), [](auto const & l, auto const & r) {
    auto const lGeomType = static_cast<int8_t>(l.GetGeomType());
    auto const rGeomType = static_cast<int8_t>(r.GetGeomType());

    auto const lId = l.HasOsmIds() ? l.GetMostGenericOsmId() : base::GeoObjectId();
    auto const rId = r.HasOsmIds() ? r.GetMostGenericOsmId() : base::GeoObjectId();

    auto const lPointsCount = l.GetPointsCount();
    auto const rPointsCount = r.GetPointsCount();

    auto const lKeyPoint = l.GetKeyPoint();
    auto const rKeyPoint = r.GetKeyPoint();

    return std::tie(lGeomType, lId, lPointsCount, lKeyPoint) < std::tie(rGeomType, rId, rPointsCount, rKeyPoint);
  });
}

bool FilenameIsCountry(std::string filename, AffiliationInterface const & affiliation)
{
  strings::ReplaceLast(filename, DATA_FILE_EXTENSION_TMP, "");
  return affiliation.HasRegionByName(filename);
}

class PlaceHelper
{
public:
  PlaceHelper()
    : m_table(std::make_shared<OsmIdToBoundariesTable>())
    , m_processor(m_table)
  {
  }

  explicit PlaceHelper(std::string const & filename)
    : PlaceHelper()
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

    m_processor.TryUpdate(fb);
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
  PlaceProcessor m_processor;
};

class ProcessorCities
{
public:
  ProcessorCities(std::string const & temporaryMwmPath, AffiliationInterface const & affiliation,
                  PlaceHelper & citiesHelper, size_t threadsCount = 1)
    : m_temporaryMwmPath(temporaryMwmPath)
    , m_affiliation(affiliation)
    , m_citiesHelper(citiesHelper)
    , m_threadsCount(threadsCount)
  {
  }

  void SetPromoCatalog(std::string const & filename)
  {
    m_citiesFilename = filename;
  }

  bool Process()
  {
    std::vector<std::future<std::vector<FeatureBuilder>>> citiesResults;
    ThreadPool pool(m_threadsCount);
    ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
      auto cities = pool.Submit([&, filename]() {
        std::vector<FeatureBuilder> cities;
        if (!FilenameIsCountry(filename, m_affiliation))
          return cities;

        auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
        auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
        FeatureBuilderWriter<MaxAccuracy> writer(fullPath);
        for (size_t i = 0; i < fbs.size(); ++i)
        {
          if (PlaceHelper::IsPlace(fbs[i]))
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
        m_citiesHelper.Process(city);
    }

    auto fbs = m_citiesHelper.GetFeatures();
    if (!m_citiesFilename.empty())
      ProcessForPromoCatalog(fbs);

    auto const affiliations = GetAffiliations(fbs, m_affiliation, m_threadsCount);
    AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
    return true;
  }

private:
  void ProcessForPromoCatalog(std::vector<FeatureBuilder> & fbs)
  {
    auto const cities = promo::LoadCities(m_citiesFilename);
    for (auto & fb : fbs)
    {
      if (cities.count(fb.GetMostGenericOsmId()) == 0)
        continue;

      auto static const kPromoType = classif().GetTypeByPath({"sponsored", "promo_catalog"});
      FeatureParams & params = fb.GetParams();
      params.AddType(kPromoType);
    }
  }

  std::string m_citiesFilename;
  std::string m_temporaryMwmPath;
  AffiliationInterface const & m_affiliation;
  PlaceHelper & m_citiesHelper;
  size_t m_threadsCount;
};
}  // namespace

FinalProcessorIntermediateMwmInterface::FinalProcessorIntermediateMwmInterface(FinalProcessorPriority priority)
  : m_priority(priority)
{
}

bool FinalProcessorIntermediateMwmInterface::operator<(FinalProcessorIntermediateMwmInterface const & other) const
{
  return m_priority < other.m_priority;
}

bool FinalProcessorIntermediateMwmInterface::operator==(FinalProcessorIntermediateMwmInterface const & other) const
{
  return !(*this < other || other < *this);
}

bool FinalProcessorIntermediateMwmInterface::operator!=(FinalProcessorIntermediateMwmInterface const & other) const
{
  return !(*this == other);
}

CountryFinalProcessor::CountryFinalProcessor(std::string const & borderPath,
                                             std::string const & temporaryMwmPath,
                                             bool haveBordersForWholeWorld,
                                             size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_borderPath(borderPath)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
  , m_threadsCount(threadsCount)
{
}

void CountryFinalProcessor::SetBooking(std::string const & filename)
{
  m_hotelsFilename = filename;
}

void CountryFinalProcessor::SetCitiesAreas(std::string const & filename)
{
  m_citiesAreasTmpFilename = filename;
}

void CountryFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFilename = filename;
}

void CountryFinalProcessor::DumpCitiesBoundaries(std::string const & filename)
{
  m_citiesBoundariesFilename = filename;
}

void CountryFinalProcessor::SetCoastlines(std::string const & coastlineGeomFilename,
                                          std::string const & worldCoastsFilename)
{
  m_coastlineGeomFilename = coastlineGeomFilename;
  m_worldCoastsFilename = worldCoastsFilename;
}

bool CountryFinalProcessor::Process()
{
  if (!m_hotelsFilename.empty() && !ProcessBooking())
    return false;

  auto const haveCities = !m_citiesAreasTmpFilename.empty() || !m_citiesFilename.empty();
  if (haveCities && !ProcessCities())
    return false;

  if (!m_coastlineGeomFilename.empty() && !ProcessCoastline())
    return false;

  return Finish();
}

bool CountryFinalProcessor::ProcessBooking()
{
  BookingDataset dataset(m_hotelsFilename);
  auto const affiliation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  {
    ThreadPool pool(m_threadsCount);
    ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
      pool.SubmitWork([&, filename]() {
        std::vector<FeatureBuilder> cities;
        if (!FilenameIsCountry(filename, affiliation))
          return;

        auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
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
  auto const affiliations = GetAffiliations(fbs, affiliation, m_threadsCount);
  AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
  return true;
}

bool CountryFinalProcessor::ProcessCities()
{
  auto const affiliation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto citiesHelper = m_citiesAreasTmpFilename.empty()
                      ? PlaceHelper()
                      : PlaceHelper(m_citiesAreasTmpFilename);
  ProcessorCities processorCities(m_temporaryMwmPath, affiliation, citiesHelper, m_threadsCount);
  processorCities.SetPromoCatalog(m_citiesFilename);
  if (!processorCities.Process())
    return false;

  if (!m_citiesBoundariesFilename.empty())
  {
    auto const citiesTable = citiesHelper.GetTable();
    LOG(LINFO, ("Dumping cities boundaries to", m_citiesBoundariesFilename));
    if (!SerializeBoundariesTable(m_citiesBoundariesFilename, *citiesTable))
    {
      LOG(LINFO, ("Error serializing boundaries table to", m_citiesBoundariesFilename));
      return false;
    }
  }

  return true;
}

bool CountryFinalProcessor::ProcessCoastline()
{
  auto const affiliation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  auto fbs = ReadAllDatRawFormat(m_coastlineGeomFilename);
  auto const affiliations = GetAffiliations(fbs, affiliation, m_threadsCount);
  AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
  FeatureBuilderWriter<> collector(m_worldCoastsFilename);
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    fbs[i].AddName("default", strings::JoinStrings(affiliations[i], ";"));
    collector.Write(fbs[i]);
  }

  return true;
}

bool CountryFinalProcessor::Finish()
{
  auto const affiliation = CountriesFilesAffiliation(m_borderPath, m_haveBordersForWholeWorld);
  ThreadPool pool(m_threadsCount);
  ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
    pool.SubmitWork([&, filename]() {
      if (!FilenameIsCountry(filename, affiliation))
        return;

      auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
      auto fbs = ReadAllDatRawFormat<MaxAccuracy>(fullPath);
      Sort(fbs);
      FeatureBuilderWriter<> collector(fullPath);
      for (auto & fb : fbs)
        collector.Write(fb);
    });
  });

  return true;
}

WorldFinalProcessor::WorldFinalProcessor(std::string const & temporaryMwmPath,
                                         std::string const & coastlineGeomFilename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_worldTmpFilename(base::JoinPath(m_temporaryMwmPath, WORLD_FILE_NAME) + DATA_FILE_EXTENSION_TMP)
  , m_coastlineGeomFilename(coastlineGeomFilename)
{
}

void WorldFinalProcessor::SetPopularPlaces(std::string const & filename)
{
  m_popularPlacesFilename = filename;
}

void WorldFinalProcessor::SetCitiesAreas(std::string const & filename)
{
  m_citiesAreasTmpFilename = filename;
}

void WorldFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFilename = filename;
}

bool WorldFinalProcessor::Process()
{
  auto const haveCities = !m_citiesAreasTmpFilename.empty() || !m_citiesFilename.empty();
  if (haveCities && !ProcessCities())
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
  auto const affiliation = SingleAffiliation(WORLD_FILE_NAME);
  auto citiesHelper = m_citiesAreasTmpFilename.empty()
                      ? PlaceHelper()
                      : PlaceHelper(m_citiesAreasTmpFilename);
  ProcessorCities processorCities(m_temporaryMwmPath, affiliation, citiesHelper);
  processorCities.SetPromoCatalog(m_citiesFilename);
  return processorCities.Process();
}

CoastlineFinalProcessor::CoastlineFinalProcessor(std::string const & filename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::WorldCoasts)
  , m_filename(filename)
{
}

void CoastlineFinalProcessor::SetCoastlinesFilenames(std::string const & geomFilename,
                                                     std::string const & rawGeomFilename)
{
  m_coastlineGeomFilename = geomFilename;
  m_coastlineRawGeomFilename = rawGeomFilename;
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
  std::vector<FeatureBuilder> outputFbs;
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
