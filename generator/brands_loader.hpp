#pragma once

#include "base/geo_object_id.hpp"

#include <string>
#include <unordered_map>

namespace generator
{
// Loads brand GeoObjectId to brand key map from json.
// |brandsFilename| file example:
//
//------------------------------------------------------
//   {
//    "nodes": {
//      "2132500347": 13,
//      "5321137826": 12
//    },
//    "ways: {
//      "440527172": 13,
//      "149816366": 12
//    },
//    "relations": {
//      "6018309": 13,
//      "6228042": 12
//    }
//   }
//------------------------------------------------------
//
// |translationsFilename| example:
//
//------------------------------------------------------
//    {
//      "12": {
//        "en": ["Subway"],
//        "ru": ["Сабвей", "Сабвэй"]
//      },
//      "13": {
//        "en": ["McDonalds", "Mc Donalds"],
//        "ru": ["Макдоналдс", "Мак Дональдс", "Мак Дональд'с"]
//      }
//    }
//------------------------------------------------------
//
// Which means OSM node 2132500347, OSM way 440527172 and OSM relation 6018309 belong to
// brand with id 13.
// OSM node 5321137826, OSM way 149816366 and OSM relation 6228042 belong to brand with id 12.
// Brand with id 13 has names in English and Russian. The most popular English name is "McDonalds",
// the second most popluar one is "Mc Donalds". The most popular Russian name is "Макдоналдс", then
// "Мак Дональдс", then "Мак Дональд'с".
// Brand with id 12 has names in English and Russian. Name in English is "Subway". The most popular Russian
// name is "Сабвей", then "Сабвэй".

bool LoadBrands(std::string const & brandsFilename, std::string const & translationsFilename,
                std::unordered_map<base::GeoObjectId, std::string> & brands);
}  // namespace generator
