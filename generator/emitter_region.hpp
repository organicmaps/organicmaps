#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/polygonizer.hpp"
#include "generator/world_map_generator.hpp"

#include <memory>
#include <string>

namespace generator
{
class EmitterRegion : public EmitterInterface
{
  using RegionGenerator = CountryMapGenerator<feature::Polygonizer<feature::FeaturesCollector>>;

public:
  explicit EmitterRegion(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void GetNames(std::vector<std::string> & names) const override;
  void operator()(FeatureBuilder1 & fb) override;
  bool Finish() override { return true; }

private:
  std::unique_ptr<RegionGenerator> m_regionGenerator;
};
}  // namespace generator
