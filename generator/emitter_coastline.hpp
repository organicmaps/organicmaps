#pragma once

#include "generator/emitter_interface.hpp"

#include <memory>
#include <string>
#include <vector>

class CoastlineFeaturesGenerator;
namespace feature
{
struct GenerateInfo;
class FeatureBuilder;
}  // namespace feature

namespace generator
{
class PlaceProcessor;
class CountryMapper;
class LayerBase;
// This class is implementation of EmitterInterface for coastlines.
class EmitterCoastline : public EmitterInterface
{
public:
  explicit EmitterCoastline(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void Process(feature::FeatureBuilder & feature) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  std::shared_ptr<PlaceProcessor> m_placeProcessor;
  std::shared_ptr<CoastlineFeaturesGenerator> m_generator;
  std::shared_ptr<LayerBase> m_processingChain;
  std::string m_coastlineGeomFilename;
  std::string m_coastlineRawGeomFilename;
};
}  // namespace generator
