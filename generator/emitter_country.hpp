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
class CountryMapper;
class LayerBase;
// This class is the implementation of EmitterInterface for countries.
class EmitterCountry : public EmitterInterface
{
public:
  explicit EmitterCountry(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void Process(feature::FeatureBuilder & feature) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  void WriteDump();

  std::shared_ptr<PlaceProcessor> m_placeProcessor;
  std::shared_ptr<CountryMapper> m_countryMapper;
  std::string m_skippedListFilename;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
