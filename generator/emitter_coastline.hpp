#pragma once

#include "generator/emitter_interface.hpp"

#include <memory>
#include <string>
#include <vector>

class FeatureBuilder1;
class CoastlineFeaturesGenerator;
namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class CityBoundaryProcessor;
class CountryMapper;
class LayerBase;
// This class is implementation of EmitterInterface for coastlines.
class EmitterCoastline : public EmitterInterface
{
public:
  explicit EmitterCoastline(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void Process(FeatureBuilder1 & feature) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  std::shared_ptr<CityBoundaryProcessor> m_cityBoundaryProcessor;
  std::shared_ptr<CoastlineFeaturesGenerator> m_generator;
  std::shared_ptr<LayerBase> m_processingChain;
  std::string m_coastlineGeomFilename;
  std::string m_coastlineRawGeomFilename;
};
}  // namespace generator
