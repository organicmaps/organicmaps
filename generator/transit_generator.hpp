#pragma once

#include "geometry/point2d.hpp"

#include "3party/jansson/myjansson.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
namespace transit
{
class DeserializerFromJson
{
public:
  DeserializerFromJson(json_struct_t * node) : m_node(node) {}

  void operator()(uint8_t & d, char const * name = nullptr) { GetField(d, name); }
  void operator()(uint16_t & d, char const * name = nullptr) { GetField(d, name); }
  void operator()(uint32_t & d, char const * name = nullptr) { GetField(d, name); }
  void operator()(uint64_t & d, char const * name = nullptr) { GetField(d, name); }
  void operator()(double & d, char const * name = nullptr) { GetField(d, name); }
  void operator()(std::string & s, char const * name = nullptr) { GetField(s, name); }
  void operator()(m2::PointD & p, char const * name = nullptr);

  template <typename R>
  void operator()(R & r, char const * name = nullptr)
  {
    r.Visit(*this);
  }

  template <typename T>
  void operator()(std::vector<T> & vs, char const * name = nullptr)
  {
    auto * arr = my::GetJSONOptionalField(m_node, name);
    if (arr == nullptr)
      return;

    if (!json_is_array(arr))
      MYTHROW(my::Json::Exception, ("The field", name, "must contain a json array."));
    size_t const sz = json_array_size(arr);
    vs.resize(sz);
    for (size_t i = 0; i < sz; ++i)
    {
      DeserializerFromJson arrayItem(json_array_get(arr, i));
      arrayItem(vs[i]);
    }
  }

private:
  template <typename T>
  void GetField(T & t, char const * name = nullptr)
  {
    if (name == nullptr)
    {
      // |name| is not set in case of array items
      FromJSON(m_node, t);
    }
    else
    {
      json_struct_t * field = my::GetJSONOptionalField(m_node, name);
      if (field == nullptr)
        return; // No field |name| at |m_node|.
      FromJSON(field, t);
    }
  }

  json_struct_t * m_node;
};

/// \brief Builds transit section at mwm.
/// \param mwmPath relative or full path to built mwm. The name of mwm without extension is considered
/// as country id.
/// \param transitDir a path to directory with json files with transit graphs.
void BuildTransit(std::string const & mwmPath, std::string const & transitDir);
}  // namespace transit
}  // namespace routing
