#pragma once

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

class TownsDumper
{
public:
  TownsDumper();

  template <typename TElement>
  void CheckElement(TElement const & em)
  {
    if (em.type != TElement::EntityType::Node)
      return;
    uint64_t population = 1;
    bool town = false;
    bool capital = false;
    for (auto const & tag : em.Tags())
    {
      string key(tag.key), value(tag.value);
      if (key == "population")
      {
        try
        {
          population = stoul(value);
        }
        catch (std::invalid_argument const &)
        {
          continue;
        }
      }
      else if (key == "capital")
      {
        capital = true;
      }
      else if (key == "place" && (value == "city" || value == "town"))
      {
        town = true;
      }
    }
    if (town || capital)
      m_records.emplace_back(em.lat, em.lon, em.id, capital, population);
  }

  void Dump(string filePath);

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

  vector<Town> m_records;
};
