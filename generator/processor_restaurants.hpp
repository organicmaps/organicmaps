#include "generator/processor_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"

#include <vector>
#include <string>

namespace generator
{
class ProcessorRestaurants : public FeatureProcessorInterface
{
public:
  explicit ProcessorRestaurants(std::shared_ptr<FeatureProcessorQueue> const & queue);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & fb) override;
  void Flush() override;
  bool Finish() override;

  void Merge(FeatureProcessorInterface const & other) override;
  void MergeInto(ProcessorRestaurants & other) const override;

private:
  struct Stats
  {
    Stats operator+(Stats const & other) const
    {
      Stats s;
      s.m_restaurantsPoi = m_restaurantsPoi  + other.m_restaurantsPoi;
      s.m_restaurantsBuilding = m_restaurantsBuilding  + other.m_restaurantsBuilding;
      s.m_unexpectedFeatures = m_unexpectedFeatures  + other.m_unexpectedFeatures;
      return s;
    }

    // Number of features of any "food type".
    uint32_t m_restaurantsPoi = 0;
    uint32_t m_restaurantsBuilding = 0;
    uint32_t m_unexpectedFeatures = 0;
  };

  std::shared_ptr<AffilationsFeatureLayer<>> m_affilationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
  Stats m_stats;
};
}  // namespace generator
