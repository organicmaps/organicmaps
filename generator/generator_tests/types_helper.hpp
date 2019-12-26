#pragma once

#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "base/stl_helpers.hpp"

#include <string>
#include <utility>
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

inline void FillXmlElement(std::vector<OsmElement::Tag> const & tags, OsmElement * p)
{
  for (auto const & t : tags)
    p->AddTag(t);
}

inline uint32_t GetType(std::vector<std::string> const & path)
{
  return classif().GetTypeByPath(path);
}

inline uint32_t GetType(base::StringIL const & lst)
{
  return classif().GetTypeByPath(lst);
}
} // namespace tests
