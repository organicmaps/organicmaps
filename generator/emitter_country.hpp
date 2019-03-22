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
class CountryMapper;
class LayerBase;
// This class is the implementation of EmitterInterface for countries.
class EmitterCountry : public EmitterInterface
{
public:
  explicit EmitterCountry(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void Process(FeatureBuilder1 & feature) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  void WriteDump();

  std::shared_ptr<CityBoundaryProcessor> m_cityBoundaryProcessor;
  std::shared_ptr<CountryMapper> m_countryMapper;
  std::string m_skippedListFilename;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
