#pragma once

#include "generator/features_processing_helpers.hpp"
#include "generator/final_processor_intermediate_mwm.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/translator_collection.hpp"
#include "generator/translator_interface.hpp"

#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace generator
{
class RawGenerator
{
public:
  explicit RawGenerator(feature::GenerateInfo & genInfo, size_t threadsCount = 1,
                        size_t chankSize = 1024);

  void GenerateCountries(bool disableAds = true);
  void GenerateWorld(bool disableAds = true);
  void GenerateCoasts();
  void GenerateRegionFeatures(std::string const & filename);
  void GenerateStreetsFeatures(std::string const & filename);
  void GenerateGeoObjectsFeatures(std::string const & filename);
  void GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator);
  void GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator,
                      std::shared_ptr<FinalProcessorIntermediateMwmInterface> const & finalProcessor);
  bool Execute();
  std::vector<std::string> GetNames() const;
  std::shared_ptr<FeatureProcessorQueue> GetQueue();
  void ForceReloadCache();

private:
  using FinalProcessorPtr = std::shared_ptr<FinalProcessorIntermediateMwmInterface>;

  struct FinalProcessorPtrCmp
  {
    bool operator()(FinalProcessorPtr const & l, FinalProcessorPtr const & r)
    {
      return *l < *r;
    }
  };

  FinalProcessorPtr CreateCoslineFinalProcessor();
  FinalProcessorPtr CreateCountryFinalProcessor();
  FinalProcessorPtr CreateWorldFinalProcessor();
  bool GenerateFilteredFeatures();

  feature::GenerateInfo & m_genInfo;
  size_t m_threadsCount;
  size_t m_chankSize;
  std::shared_ptr<cache::IntermediateData> m_cache;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<TranslatorCollection> m_translators;
  std::priority_queue<FinalProcessorPtr, std::vector<FinalProcessorPtr>, FinalProcessorPtrCmp> m_finalProcessors;
  std::vector<std::string> m_names;
};
}  // namespace generator
