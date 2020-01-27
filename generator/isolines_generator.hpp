#pragma once

#include "generator/feature_builder.hpp"

#include <string>

namespace generator
{
class IsolineFeaturesGenerator
{
public:
  explicit IsolineFeaturesGenerator(std::string const & isolinesDir);

  void GenerateIsolines(std::string const & countryName,
                        std::vector<feature::FeatureBuilder> & fbs) const;

private:
  std::string m_isolinesDir;
};
}  // namespace generator
