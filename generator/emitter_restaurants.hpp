#include "generator/emitter_interface.hpp"
#include "generator/feature_builder.hpp"

#include <vector>
#include <string>

namespace generator
{
class EmitterRestaurants : public EmitterInterface
{
public:
  EmitterRestaurants(std::vector<FeatureBuilder1> & features);

  // EmitterInterface overrides:
  void operator()(FeatureBuilder1 & fb) override;
  void GetNames(std::vector<std::string> & names) const override;
  bool Finish() override;

private:
  struct Stats
  {
    // Number of features of any "food type".
    uint32_t m_restaurantsPoi = 0;
    uint32_t m_restaurantsBuilding = 0;
    uint32_t m_unexpectedFeatures = 0;
  };

  std::vector<FeatureBuilder1> & m_features;
  Stats m_stats;
};
}  // namespace generator
