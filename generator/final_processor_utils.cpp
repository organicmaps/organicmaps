#include "generator/final_processor_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <tuple>

namespace generator
{
using namespace feature;

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

  return std::tie(lGeomType, lId, lPointsCount, lKeyPoint) < std::tie(rGeomType, rId, rPointsCount, rKeyPoint);
}

void Order(std::vector<FeatureBuilder> & fbs)
{
  std::sort(std::begin(fbs), std::end(fbs), Less);
}

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
                                                      AffiliationInterface const & affiliation, size_t threadsCount)
{
  base::ComputationalThreadPool pool(threadsCount);
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
