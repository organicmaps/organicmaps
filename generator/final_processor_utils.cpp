#include "generator/final_processor_utils.hpp"

#include "generator/promo_catalog_cities.hpp"

#include "indexer/feature_data.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <mutex>
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
  std::mutex mutex;
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path) {
    if (!m_affiliation.HasCountryByName(country))
      return;

    std::vector<FeatureBuilder> cities;
    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](auto  && fb, auto /* pos */) {
      if (PlaceHelper::IsPlace(fb))
        cities.emplace_back(std::move(fb));
      else
        writer.Write(std::move(fb));
    });

    std::lock_guard<std::mutex> lock(mutex);
    for (auto const & city : cities)
      m_citiesHelper.Process(city);

  }, m_threadsCount);

  auto fbsWithIds = m_citiesHelper.GetFeatures();
  if (!m_citiesFilename.empty())
    ProcessForPromoCatalog(fbsWithIds);

  std::vector<FeatureBuilder> fbs;
  fbs.reserve(fbsWithIds.size());
  base::Transform(fbsWithIds, std::back_inserter(fbs), base::RetrieveFirst());

  AppendToMwmTmp(fbs, m_affiliation, m_temporaryMwmPath, m_threadsCount);
}

void ProcessorCities::ProcessForPromoCatalog(std::vector<PlaceProcessor::PlaceWithIds> & fbsWithIds)
{
  auto const cities = promo::LoadCities(m_citiesFilename);
  for (auto & [feature, ids] : fbsWithIds)
  {
    for (auto const & id : ids)
    {
      if (cities.count(id) == 0)
        continue;

      auto static const kPromoType = classif().GetTypeByPath({"sponsored", "promo_catalog"});
      FeatureParams & params = feature.GetParams();
      params.AddType(kPromoType);
    }
  }
}

PlaceHelper::PlaceHelper()
  : m_table(std::make_shared<OsmIdToBoundariesTable>())
  , m_processor(m_table)
{
}

PlaceHelper::PlaceHelper(std::string const & filename)
  : PlaceHelper()
{
  feature::ForEachFeatureRawFormat<feature::serialization_policy::MaxAccuracy>(
      filename, [&](auto const & fb, auto const &) { m_processor.Add(fb); });
}

// static
bool PlaceHelper::IsPlace(feature::FeatureBuilder const & fb)
{
  auto const type = GetPlaceType(fb);
  return type != ftype::GetEmptyValue() && !fb.GetName().empty() && NeedProcessPlace(fb);
}

bool PlaceHelper::Process(feature::FeatureBuilder const & fb)
{
  m_processor.Add(fb);
  return true;
}

std::vector<PlaceProcessor::PlaceWithIds> PlaceHelper::GetFeatures()
{
  return m_processor.ProcessPlaces();
}

std::shared_ptr<OsmIdToBoundariesTable> PlaceHelper::GetTable() const { return m_table; }

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
}  // namespace generator
