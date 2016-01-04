#pragma once

#include "geometry/latlon.hpp"

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
    for (auto const & tag : em.Tags())
    {
      string key(tag.key), value(tag.value);
      if (key == "capital" || (key == "place" && (value == "city" || value == "town")))
      {
       m_records.emplace_back(em.lat, em.lon, em.id);
       return;
      }
    }
  }

void Dump(string filePath);

private:

struct Town{
  ms::LatLon point;
  uint64_t id;

  Town(double lat, double lon, uint64_t id) : point(lat, lon), id(id) {}
};

vector<Town> m_records;
};
