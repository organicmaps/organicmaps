#pragma once

#include "generator/emitter_interface.hpp"

#include <memory>
#include <string>
#include <vector>

class FeatureBuilder1;

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class CityBoundaryProcessor;
class WorldMapper;
class LayerBase;

// This class is implementation of EmitterInterface for the world.
class EmitterWorld : public EmitterInterface
{
public:
  explicit EmitterWorld(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void Process(FeatureBuilder1 & feature) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  std::shared_ptr<CityBoundaryProcessor> m_cityBoundaryProcessor;
  std::shared_ptr<WorldMapper> m_worldMapper;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
