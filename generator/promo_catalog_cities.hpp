#pragma once

#include "coding/file_reader.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <string>
#include <unordered_set>

#include "3party/jansson/myjansson.hpp"

namespace promo
{
using Cities = std::unordered_set<base::GeoObjectId>;

inline Cities LoadCities(std::string const & filename)
{
  std::string src;
  try
  {
    FileReader reader(filename);
    reader.ReadAsString(src);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, (filename, e.Msg()));
    return {};
  }

  Cities result;
  try
  {
    base::Json root(src.c_str());
    auto const dataArray = json_object_get(root.get(), "data");

    auto const size = json_array_size(dataArray);

    result.reserve(size);
    for (size_t i = 0; i < size; ++i)
    {
      int64_t id = 0;
      auto const obj = json_array_get(dataArray, i);
      FromJSONObject(obj, "osmid", id);

      result.emplace(static_cast<uint64_t>(id));
    }
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    result.clear();
  }

  return result;
}
}  // namespace promo
