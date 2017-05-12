#include "towns_dumper.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/tree4d.hpp"

#include "base/logging.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace
{
uint64_t constexpr kTownsEqualityMeters = 500000;
}  // namespace

TownsDumper::TownsDumper() {}
void TownsDumper::FilterTowns()
{
  LOG(LINFO, ("Preprocessing started. Have", m_records.size(), "towns."));
  m4::Tree<Town> resultTree;
  std::vector<Town> towns;
  towns.reserve(m_records.size());
  for (auto const & town : m_records)
  {
    if (town.capital)
      resultTree.Add(town);
    else
      towns.push_back(town);
  }
  sort(towns.begin(), towns.end());

  LOG(LINFO, ("Tree of capitals has size", resultTree.GetSize(), "towns has size:", towns.size()));
  m_records.clear();

  while (!towns.empty())
  {
    auto const & top = towns.back();
    bool isUniq = true;
    resultTree.ForEachInRect(
        MercatorBounds::RectByCenterXYAndSizeInMeters(MercatorBounds::FromLatLon(top.point),
                                                      kTownsEqualityMeters),
        [&top, &isUniq](Town const & candidate)
        {
          if (ms::DistanceOnEarth(top.point, candidate.point) < kTownsEqualityMeters)
            isUniq = false;
        });
    if (isUniq)
      resultTree.Add(top);
    towns.pop_back();
  }

  resultTree.ForEach([this](Town const & town)
                     {
                       m_records.push_back(town);
                     });
  LOG(LINFO, ("Preprocessing finished. Have", m_records.size(), "towns."));
}

void TownsDumper::Dump(std::string const & filePath)
{
  FilterTowns();
  ASSERT(!filePath.empty(), ());
  std::ofstream stream(filePath);
  stream.precision(9);
  for (auto const & record : m_records)
  {
    std::string const isCapital = record.capital ? "t" : "f";
    stream << record.point.lat << ";" << record.point.lon << ";" << record.id << ";" << isCapital <<  std::endl;
  }
}
