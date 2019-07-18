#pragma once

#include "generator/emitter_interface.hpp"

#include <memory>
#include <string>
#include <vector>

namespace feature
{
class FeatureBuilder;
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class PlaceProcessor;
class WorldMapper;
class LayerBase;

// This class is implementation of EmitterInterface for the world.
class EmitterWorld : public EmitterInterface
{
public:
  explicit EmitterWorld(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void Process(feature::FeatureBuilder & feature) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  std::shared_ptr<PlaceProcessor> m_placeProcessor;
  std::shared_ptr<WorldMapper> m_worldMapper;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
