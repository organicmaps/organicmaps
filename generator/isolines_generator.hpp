#pragma once

#include "generator/feature_builder.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace generator
{
class IsolineFeaturesGenerator
{
public:
  explicit IsolineFeaturesGenerator(std::string const & isolinesDir);

  using FeaturesCollectFn = std::function<void(feature::FeatureBuilder && fb)>;
  void GenerateIsolines(std::string const & countryName, FeaturesCollectFn const & fn) const;

private:
  uint32_t GetIsolineType(int altitude) const;

  std::string m_isolinesDir;
  std::unordered_map<int, uint32_t> m_altClassToType;
};
}  // namespace generator
