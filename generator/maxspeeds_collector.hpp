#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace generator
{

/// \brief Collects all maxspeed tags value and saves them to a csv file.
/// Every line describes maxspeed, maxspeed:forward and maxspeed:backward
/// tags of features. The format of the lines is described below.
class MaxspeedsCollector : public CollectorInterface
{
public:
  /// \param filePath path to csv file.
  explicit MaxspeedsCollector(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const &, OsmElement const & p) override;
  void Finish() override;

  IMPLEMENT_COLLECTOR_IFACE(MaxspeedsCollector);
  void MergeInto(MaxspeedsCollector & collector) const;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  // |m_stream| contains strings with maxspeed tags value for corresponding features:
  // OSM id,units (Metric/Imperial),maxspeed(forward),maxspeed:backward,maxspeed:conditional,string condition
  //
  // There are possible examples of strings contained in the list |m_stream|:
  // 2343313,Metric,60,,,
  // 3243245345,Metric,60,80,,
  // 13243214,Imperial,,,45,nov - mar
  // 31171117,Metric,100,80,80,nov - mar
  //
  // Note 1. Some speed constants for maxpseed(forward): kNoneMaxSpeed, kWalkMaxSpeed, kCommonMaxSpeedValue
  // Note 2. Saying OSM id means OSM id of features with OsmElement::EntityType::Way type without
  // any prefixes. It's done so to simplify the debugging process. This way it's very easy knowing
  // osm id to see the feature on the web.
  // Note 3. A string is saved in |m_stream| only if it's valid and parsed successfully
  // with ParseMaxspeedTag() function. That means all macro like RU:urban or GE:rural
  // are converted to an appropriate speed value and macro "none" and "walk" are converted
  // to |kNoneMaxSpeed| and |kWalkMaxSpeed|.
  std::ofstream m_stream;
};
}  // namespace generator
