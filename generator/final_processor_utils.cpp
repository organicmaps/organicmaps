#include "generator/final_processor_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <mutex>
#include <tuple>

namespace generator
{
using namespace feature;

ProcessorCities::ProcessorCities(std::string const & collectorFilename,
                                 AffiliationInterface const & affiliation,
                                 size_t threadsCount)
  : m_processor(collectorFilename)
  , m_affiliation(affiliation)
  , m_threadsCount(threadsCount)
{
}

void ProcessorCities::Process(std::string const & mwmPath)
{
  std::mutex mutex;
  std::vector<FeatureBuilder> allCities;
  auto const & localityChecker = ftypes::IsLocalityChecker::Instance();

  ForEachMwmTmp(mwmPath, [&](auto const & country, auto const & path)
  {
    if (!m_affiliation.HasCountryByName(country))
      return;

    std::vector<FeatureBuilder> cities;
    FeatureBuilderWriter<serialization_policy::MaxAccuracy> writer(path, true /* mangleName */);
    ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(path, [&](FeatureBuilder && fb, uint64_t)
    {
      if (localityChecker.GetType(fb.GetTypes()) < ftypes::LocalityType::City)
        writer.Write(std::move(fb));
      else
        cities.emplace_back(std::move(fb));
    });

    std::lock_guard<std::mutex> lock(mutex);
    std::move(std::begin(cities), std::end(cities), std::back_inserter(allCities));
  }, m_threadsCount);

  /// @todo Do we need tmp vector and additional ordering here?
  Order(allCities);
  for (auto & city : allCities)
    m_processor.Add(std::move(city));

  AppendToMwmTmp(m_processor.ProcessPlaces(), m_affiliation, mwmPath, m_threadsCount);
}


bool Less(FeatureBuilder const & lhs, FeatureBuilder const & rhs)
{
  auto const lGeomType = static_cast<int8_t>(lhs.GetGeomType());
  auto const rGeomType = static_cast<int8_t>(rhs.GetGeomType());

  // may be empty IDs
  auto const lId = lhs.GetMostGenericOsmId();
  auto const rId = rhs.GetMostGenericOsmId();

  auto const lPointsCount = lhs.GetPointsCount();
  auto const rPointsCount = rhs.GetPointsCount();

  auto const lKeyPoint = lhs.GetKeyPoint();
  auto const rKeyPoint = rhs.GetKeyPoint();

  return std::tie(lGeomType, lId, lPointsCount, lKeyPoint) <
         std::tie(rGeomType, rId, rPointsCount, rKeyPoint);
}

void Order(std::vector<FeatureBuilder> & fbs) { std::sort(std::begin(fbs), std::end(fbs), Less); }

void OrderTextFileByLine(std::string const & filename)
{
  std::ifstream istream;
  istream.exceptions(std::fstream::badbit);
  istream.open(filename);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(istream, line))
    lines.emplace_back(std::move(line));

  std::sort(std::begin(lines), std::end(lines));
  istream.close();

  std::ofstream ostream;
  ostream.exceptions(std::fstream::failbit | std::fstream::badbit);
  ostream.open(filename);
  std::copy(std::begin(lines), std::end(lines), std::ostream_iterator<std::string>(ostream, "\n"));
}

std::vector<std::vector<std::string>> GetAffiliations(std::vector<FeatureBuilder> const & fbs,
                                                      AffiliationInterface const & affiliation,
                                                      size_t threadsCount)
{
  base::thread_pool::computational::ThreadPool pool(threadsCount);
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
