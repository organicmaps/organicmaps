#pragma once

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace drule
{

class ICityRankTable
{
public:
  virtual ~ICityRankTable() = default;
  virtual bool GetCityRank(uint32_t zoomLevel, uint32_t population, double & rank) const = 0;
};

// Parses string and returns city-rank table which corresponds to string
// String format similar to mapcss format:
// z[ZOOMLEVEL_FROM]-[ZOOMLEVEL_TP]
// {
//    [POPULATION_FROMn]-[POPULATION_TOn]:RANKn;
// }
//
// z[ZOOMLEVEL_FROM]-[ZOOMLEVEL_TO] - specifies zoom range for which population rula is applicable.
// ZOOMLEVEL_FROM and ZOOMLEVEL_TO are non negative integers, ZOOMLEVEL_TO must be not less than
// ZOOMLEVEL_FROM.
// ZOOMLEVEL_FROM may be omitted, then ZOOMLEVEL_FROM is 0
// ZOOMLEVEL_TO may be omitted, then ZOOMLEVEL_TO is UPPER_SCALE
// if ZOOMLEVEL_FROM equals to ZOOMLEVEL_TO then z can be rewritten as z[ZOOMLEVEL]
//
// [POPULATION_FROMn]-[POPULATION_TOn]:RANKn; - specifies population range and city rank.
// POPULATION_FROMn and POPULATION_TOn are non negative integers, POPULATION_TOn must be not less than
// POPULATION_FROMn.
// Similarly, POPULATION_FROMn can be ommited then, POPULATION_FROMn is 0
// POPULATION_TOn also can be ommited then POPULATION_FROMn is MAX_UINT
// RANKn is float value. if RANKn is less than 0 then population scope is not drawn,
// if RANKn is 0 then city will be drawn as it specified in mapcss
// if RANKn is more than 0 then font size will be (1.0 + RANKn) * fontSizeInMapcss
//
// Example is:
// z-3 { -10000:-1; 10000-:0; }
// z4 { -10000:0; 10000-:0.5; }
// z5-9 { -10000:0; 10000-100000:0.5;100000-:0.75; }
// z10- { -10000:0.5; 10000-100000:0.75;100000-:1; }

// Returns city-rank-table if str matches to format, otherwise returns nullptr
unique_ptr<ICityRankTable> GetCityRankTableFromString(string & str);

// Returns city-rank-table which returns constant rank for any zoom and population.
unique_ptr<ICityRankTable> GetConstRankCityRankTable(double rank = 0.0);

}  // namespace drule
