#pragma once

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "generator/osm_element.hpp"

#include "base/string_utils.hpp"

#include <limits>
#include <string>
#include <vector>

class TownsDumper
{
public:
  TownsDumper();

  template <typename TElement>
  void CheckElement(TElement const & em, std::vector<OsmElement::Tag> const & tags)
  {
    if (em.type != TElement::EntityType::Node)
      return;
    uint64_t population = 1;
    bool town = false;
    bool capital = false;
    int admin_level = std::numeric_limits<int>::max();
    for (auto const & tag : tags)
    {
      std::string key(tag.key), value(tag.value);
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
      m_records.emplace_back(em.lat, em.lon, em.id, capital, population);
  }

  void Dump(std::string const & filePath);

private:
  void FilterTowns();

  struct Town
  {
    ms::LatLon point;
    uint64_t id;
    bool capital;
    uint64_t population;

    Town(double lat, double lon, uint64_t id, bool isCapital, uint64_t population)
      : point(lat, lon), id(id), capital(isCapital), population(population)
    {
    }

    bool operator<(Town const & rhs) const { return population < rhs.population; }
    m2::RectD GetLimitRect() const
    {
      return m2::RectD(MercatorBounds::FromLatLon(point), MercatorBounds::FromLatLon(point));
    }
  };

  std::vector<Town> m_records;
};
