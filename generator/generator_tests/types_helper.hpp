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
template <size_t N>
inline void AddTypes(FeatureParams & params, base::StringIL (&arr)[N])
{
  Classificator const & c = classif();
  for (auto const & e : arr)
    params.AddType(c.GetTypeByPath(e));
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
}  // namespace tests
