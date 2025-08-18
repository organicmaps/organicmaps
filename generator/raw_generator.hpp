#pragma once

#include "generator/affiliation.hpp"
#include "generator/composite_id.hpp"
#include "generator/features_processing_helpers.hpp"
#include "generator/final_processor_interface.hpp"
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
  explicit RawGenerator(feature::GenerateInfo & genInfo, size_t threadsCount = 1, size_t chunkSize = 1024);

  void GenerateCountries(bool isTests = false);
  void GenerateWorld(bool cutBordersByWater = true);
  void GenerateCoasts();
  void GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator);
  void GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator,
                      std::shared_ptr<FinalProcessorIntermediateMwmInterface> const & finalProcessor);
  bool Execute();
  std::vector<std::string> const & GetNames() const { return m_names; }
  std::shared_ptr<FeatureProcessorQueue> GetQueue() { return m_queue; }
  void ForceReloadCache();

private:
  using FinalProcessorPtr = std::shared_ptr<FinalProcessorIntermediateMwmInterface>;

  struct FinalProcessorPtrCmp
  {
    bool operator()(FinalProcessorPtr const & l, FinalProcessorPtr const & r) { return *l < *r; }
  };

  FinalProcessorPtr CreateCoslineFinalProcessor();
  FinalProcessorPtr CreateCountryFinalProcessor(AffiliationInterfacePtr const & affiliations, bool needMixNodes);
  FinalProcessorPtr CreateWorldFinalProcessor(bool cutBordersByWater);
  FinalProcessorPtr CreatePlacesFinalProcessor(AffiliationInterfacePtr const & affiliations);

  bool GenerateFilteredFeatures();

  feature::GenerateInfo & m_genInfo;
  size_t m_threadsCount;
  size_t m_chunkSize;
  cache::IntermediateDataObjectsCache m_intermediateDataObjectsCache;
  std::shared_ptr<cache::IntermediateData> m_cache;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<TranslatorCollection> m_translators;
  std::priority_queue<FinalProcessorPtr, std::vector<FinalProcessorPtr>, FinalProcessorPtrCmp> m_finalProcessors;
  std::vector<std::string> m_names;
  // std::unordered_set<CompositeId> m_hierarchyNodesSet;
};
}  // namespace generator
