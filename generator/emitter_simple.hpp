#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/polygonizer.hpp"
#include "generator/world_map_generator.hpp"

#include <memory>
#include <string>
#include <vector>

namespace generator
{
// EmitterSimple class is a simple emitter. It does not filter objects.
class EmitterSimple : public EmitterInterface
{
public:
  explicit EmitterSimple(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void GetNames(std::vector<std::string> & names) const override;
  void Process(FeatureBuilder1 & fb) override;
  bool Finish() override { return true; }

private:
  using SimpleGenerator = SimpleCountryMapGenerator<feature::Polygonizer<feature::FeaturesCollector>>;

  std::unique_ptr<SimpleGenerator> m_regionGenerator;
};

class EmitterPreserialize : public EmitterSimple
{
public:
  using EmitterSimple::EmitterSimple;

  // EmitterInterface overrides:
  void Process(FeatureBuilder1 & fb) override;
};
}  // namespace generator
