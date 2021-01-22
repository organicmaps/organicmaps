#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <strstream>
#include <memory>
#include <string>

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

/// \brief Collects all maxspeed tags value and saves them to a csv file.
/// Every line describes maxspeed, maxspeed:forward and maxspeed:backward
/// tags of features. The format of the lines is described below.
class MaxspeedsCollector : public CollectorInterface
{
public:
  /// \param filePath path to csv file.
  explicit MaxspeedsCollector(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const &, OsmElement const & p) override;
  void Finish() override;

  void Merge(CollectorInterface const & collector) override;
  void MergeInto(MaxspeedsCollector & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  // |m_data| contains strings with maxspeed tags value for corresponding features in one of the
  // following formats
  // 1. osm id,units kmh or mph,maxspeed value
  // 2. osm id,units kmh or mph,maxspeed:forward value
  // 3. osm id,units kmh or mph,maxspeed:forward value,maxspeed:backward value
  // There are possible examples of strings contained in the list |m_data|:
  // 2343313,Metric,60
  // 13243214,Imperial,60
  // 3243245345,Metric,60,80
  // 134243,Imperial,30,50
  // 45432423,Metric,60,65534
  // 53445423,Metric,60,65533
  //
  // Note 1. 65534 means kNoneMaxSpeed and 65533 means kWalkMaxSpeed. They are constants for
  // maxspeed tag value "none" and "walk" correspondingly.
  // Note 2. Saying osm id means osm id of features with OsmElement::EntityType::Way type without
  // any prefixes. It's done so to simplify the debugging process. This way it's very easy knowing
  // osm id to see the feature on the web.
  // Note 3. A string is saved in |m_data| only if it's valid and parsed successfully
  // with ParseMaxspeedTag() function. That means all macro like RU:urban or GE:rural
  // are converted to an appropriate speed value and macro "none" and "walk" are converted
  // to |kNoneMaxSpeed| and |kWalkMaxSpeed|.
  std::ofstream m_stream;
};
}  // namespace generator
