#include "towns_dumper.hpp"

#include "generator/osm_element.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/tree4d.hpp"

#include "base/logging.hpp"

#include <fstream>

namespace
{
uint64_t constexpr kTownsEqualityMeters = 500000;
}  // namespace

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

  LOG(LINFO, ("Capital's tree size =", resultTree.GetSize(), "; Town's vector size =", towns.size()));
  m_records.clear();

  while (!towns.empty())
  {
    auto const & top = towns.back();
    bool isUniq = true;
    resultTree.ForEachInRect(
        mercator::RectByCenterXYAndSizeInMeters(mercator::FromLatLon(top.point), kTownsEqualityMeters),
        [&top, &isUniq](Town const & candidate)
        {
          // The idea behind that is to collect all capitals and unique major cities in 500 km radius
          // for upgrading in World map visibility. See TOWNS_FILE usage.
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

void TownsDumper::CheckElement(OsmElement const & em)
{
  if (em.m_type != OsmElement::EntityType::Node)
    return;

  uint64_t population = 1;
  bool town = false;
  bool capital = false;

  // OSM goes to moving admin_level tag into boundary=administrative relation only.
  // So capital=yes should be enough for country capital.
  int admin_level = -1;

  for (auto const & tag : em.Tags())
  {
    auto const & key = tag.m_key;
    auto const & value = tag.m_value;
    if (key == "population")
    {
      if (!strings::to_uint64(value, population))
        continue;
    }
    else if (key == "admin_level")
    {
      if (!strings::to_int(value, admin_level))
        continue;
    }
    else if (key == "capital" && value == "yes")
    {
      capital = true;
    }
    else if (key == "place" && (value == "city" || value == "town"))
    {
      town = true;
    }
  }

  // Ignore regional capitals.
  if (capital && admin_level > 2)
    capital = false;

  if (town || capital)
    m_records.emplace_back(em.m_lat, em.m_lon, em.m_id, capital, population);
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
    stream << record.point.m_lat << ";" << record.point.m_lon << ";" << record.id << ";" << isCapital <<  std::endl;
  }
}
