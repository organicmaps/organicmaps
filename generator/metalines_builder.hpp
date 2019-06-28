#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <cstdlib>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache
}  // namespace generator

namespace feature
{
// A string of connected ways.
class LineString
{
public:
  using Ways = std::vector<int32_t>;

  explicit LineString(OsmElement const & way);

  bool Add(LineString & line);
  void Reverse();

  Ways const & GetWays() const { return m_ways; }
  uint64_t GetStart() const { return m_start; }
  uint64_t GetEnd() const { return m_end; }

private:
  uint64_t m_start;
  uint64_t m_end;
  bool m_oneway;
  Ways m_ways;
};

class LineStringMerger
{
public:
  using LinePtr = std::shared_ptr<LineString>;
  using InputData = std::unordered_multimap<size_t, LinePtr>;
  using OutputData = std::map<size_t, std::vector<LinePtr>>;

  static OutputData Merge(InputData const & data);

private:
  using Buffer = std::unordered_map<uint64_t, LinePtr>;

  static bool TryMerge(LinePtr const & lineString, Buffer & buffer);
  static bool TryMergeOne(LinePtr const & lineString, Buffer & buffer);
  static OutputData OrderData(InputData const & data);
};

// Merges road segments with similar name and ref values into groups called metalines.
class MetalinesBuilder : public generator::CollectorInterface
{
public:
  explicit MetalinesBuilder(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<generator::cache::IntermediateDataReader> const & = {}) const override;

  /// Add a highway segment to the collection of metalines.
  void CollectFeature(FeatureBuilder const & feature, OsmElement const & element) override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(MetalinesBuilder & collector) const override;

private:
  std::unordered_multimap<size_t, std::shared_ptr<LineString>> m_data;
};

// Read an intermediate file from MetalinesBuilder and convert it to an mwm section.
bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath);
}
