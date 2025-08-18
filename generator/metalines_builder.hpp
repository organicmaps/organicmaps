#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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

  template <typename T>
  void Serialize(T & w)
  {
    WriteVarUint(w, m_start);
    WriteVarUint(w, m_end);
    WriteToSink(w, m_oneway);
    rw::WriteVectorOfPOD(w, m_ways);
  }

  template <typename T>
  static LineString Deserialize(T & r)
  {
    LineString ls;
    ls.m_start = ReadVarUint<uint64_t>(r);
    ls.m_end = ReadVarUint<uint64_t>(r);
    ReadPrimitiveFromSource(r, ls.m_oneway);
    rw::ReadVectorOfPOD(r, ls.m_ways);
    return ls;
  }

private:
  LineString() = default;

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
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;

  /// Add a highway segment to the collection of metalines.
  void CollectFeature(FeatureBuilder const & feature, OsmElement const & element) override;

  void Finish() override;

  IMPLEMENT_COLLECTOR_IFACE(MetalinesBuilder);
  void MergeInto(MetalinesBuilder & collector) const;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::unique_ptr<FileWriter> m_writer;
};

// Read an intermediate file from MetalinesBuilder and convert it to an mwm section.
bool WriteMetalinesSection(std::string const & mwmPath, std::string const & metalinesPath,
                           std::string const & osmIdsToFeatureIdsPath);
}  // namespace feature
