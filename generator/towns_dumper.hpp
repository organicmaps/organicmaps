#pragma once

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include <string>
#include <vector>

struct OsmElement;
class TownsDumper
{
public:
  void CheckElement(OsmElement const & em);
  void Dump(std::string const & filePath);

  static double GetDistanceThreshold();

private:
  void FilterTowns();

  struct Town
  {
    ms::LatLon point;
    uint64_t id;
    bool capital;
    uint64_t population;

    Town(double lat, double lon, uint64_t id, bool isCapital, uint64_t population)
      : point(lat, lon)
      , id(id)
      , capital(isCapital)
      , population(population)
    {}

    bool operator<(Town const & rhs) const { return population < rhs.population; }
    m2::RectD GetLimitRect() const
    {
      auto const mercPt = mercator::FromLatLon(point);
      return m2::RectD(mercPt, mercPt);
    }
  };

  std::vector<Town> m_records;
};
