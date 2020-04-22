#include "generator/final_processor_utils.hpp"

#include "generator/promo_catalog_cities.hpp"

#include "indexer/feature_data.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

using namespace base::thread_pool::computational;
using namespace feature;

namespace generator
{
ProcessorCities::ProcessorCities(std::string const & temporaryMwmPath,
                                 AffiliationInterface const & affiliation,
                                 PlaceHelper & citiesHelper, size_t threadsCount)
  : m_temporaryMwmPath(temporaryMwmPath)
  , m_affiliation(affiliation)
  , m_citiesHelper(citiesHelper)
  , m_threadsCount(threadsCount)
{
}

void ProcessorCities::SetPromoCatalog(std::string const & filename) { m_citiesFilename = filename; }

void ProcessorCities::Process()
{
  std::vector<std::future<std::vector<FeatureBuilder>>> citiesResults;
  ThreadPool pool(m_threadsCount);
  ForEachCountry(m_temporaryMwmPath, [&](auto const & filename) {
    auto cities = pool.Submit([&, filename]() {
      std::vector<FeatureBuilder> cities;
      if (!FilenameIsCountry(filename, m_affiliation))
        return cities;

      auto const fullPath = base::JoinPath(m_temporaryMwmPath, filename);
      auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(fullPath);
      FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(fullPath);
      for (auto && fb : fbs)
      {
        if (PlaceHelper::IsPlace(fb))
          cities.emplace_back(std::move(fb));
        else
          writer.Write(std::move(fb));
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
  auto fbsWithIds = m_citiesHelper.GetFeatures();
  if (!m_citiesFilename.empty())
    ProcessForPromoCatalog(fbsWithIds);

  std::vector<FeatureBuilder> fbs;
  fbs.reserve(fbsWithIds.size());
  base::Transform(fbsWithIds, std::back_inserter(fbs), base::RetrieveFirst());
  auto const affiliations = GetAffiliations(fbs, m_affiliation, m_threadsCount);
  AppendToCountries(fbs, affiliations, m_temporaryMwmPath, m_threadsCount);
}

void ProcessorCities::ProcessForPromoCatalog(std::vector<PlaceProcessor::PlaceWithIds> & fbs)
{
  auto const cities = promo::LoadCities(m_citiesFilename);
  for (auto & fbWithIds : fbs)
  {
    for (auto const & id : fbWithIds.second)
    {
      if (cities.count(id) == 0)
        continue;

      auto static const kPromoType = classif().GetTypeByPath({"sponsored", "promo_catalog"});
      FeatureParams & params = fbWithIds.first.GetParams();
      params.AddType(kPromoType);
    }
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

    return std::tie(lGeomType, lId, lPointsCount, lKeyPoint) <
           std::tie(rGeomType, rId, rPointsCount, rKeyPoint);
  });
}

std::vector<std::vector<std::string>> GetAffiliations(std::vector<FeatureBuilder> const & fbs,
                                                      AffiliationInterface const & affiliation,
                                                      size_t threadsCount)
{
  ThreadPool pool(threadsCount);
  std::vector<std::future<std::vector<std::string>>> futuresAffiliations;
  for (auto const & fb : fbs)
  {
    auto result = pool.Submit([&]() { return affiliation.GetAffiliations(fb); });
    futuresAffiliations.emplace_back(std::move(result));
  }

  std::vector<std::vector<std::string>> resultAffiliations;
  resultAffiliations.reserve(futuresAffiliations.size());
  for (auto & f : futuresAffiliations)
    resultAffiliations.emplace_back(f.get());

  return resultAffiliations;
}

std::string GetCountryNameFromTmpMwmPath(std::string filename)
{
  strings::ReplaceLast(filename, DATA_FILE_EXTENSION_TMP, "");
  return filename;
}

bool FilenameIsCountry(std::string const & filename, AffiliationInterface const & affiliation)
{
  return affiliation.HasCountryByName(GetCountryNameFromTmpMwmPath(filename));
}
}  // namespace generator
