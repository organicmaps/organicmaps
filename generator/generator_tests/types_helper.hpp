#pragma once

#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "base/stl_add.hpp"

#include <string>
#include <vector>


namespace tests
{

template <size_t N, size_t M>
inline void AddTypes(FeatureParams & params, char const * (&arr)[N][M])
{
  Classificator const & c = classif();

  for (size_t i = 0; i < N; ++i)
    params.AddType(c.GetTypeByPath(std::vector<std::string>(arr[i], arr[i] + M)));
}

inline void FillXmlElement(char const * arr[][2], size_t count, OsmElement * p)
{
  for (size_t i = 0; i < count; ++i)
    p->AddTag(arr[i][0], arr[i][1]);
}

template <size_t N>
inline uint32_t GetType(char const * (&arr)[N])
{
  std::vector<std::string> path(arr, arr + N);
  return classif().GetTypeByPath(path);
}

inline uint32_t GetType(my::StringIL const & lst)
{
  return classif().GetTypeByPath(lst);
}

} // namespace tests
