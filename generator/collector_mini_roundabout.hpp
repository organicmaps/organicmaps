#pragma once

#include "generator/collector_interface.hpp"
#include "generator/mini_roundabout_info.hpp"
#include "generator/osm_element.hpp"

#include "coding/file_writer.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

class MiniRoundaboutProcessor
{
public:
  explicit MiniRoundaboutProcessor(std::string const & filename);
  ~MiniRoundaboutProcessor();

  template <typename Fn>
  void ForEachMiniRoundabout(Fn && toDo)
  {
    for (auto & p : m_miniRoundabouts)
    {
      if (m_miniRoundaboutsExceptions.find(p.first) == m_miniRoundaboutsExceptions.end())
        toDo(p.second);
    }
  }

  void ProcessNode(OsmElement const & element);
  void ProcessWay(OsmElement const & element);
  void ProcessRestriction(uint64_t osmId);

  void FillMiniRoundaboutsInWays();

  void Finish();
  void Merge(MiniRoundaboutProcessor const & MiniRoundaboutProcessor);

private:
  std::string m_waysFilename;
  std::unique_ptr<FileWriter> m_waysWriter;
  std::unordered_map<uint64_t, MiniRoundaboutInfo> m_miniRoundabouts;
  std::set<uint64_t> m_miniRoundaboutsExceptions;
};

class MiniRoundaboutCollector : public generator::CollectorInterface
{
public:
  explicit MiniRoundaboutCollector(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<generator::cache::IntermediateDataReaderInterface> const & = {})
      const override;

  void Collect(OsmElement const & element) override;
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Finish() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(MiniRoundaboutCollector & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  MiniRoundaboutProcessor m_processor;
};
}  // namespace generator
