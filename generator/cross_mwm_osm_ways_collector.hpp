#pragma once

#include "generator/affiliation.hpp"
#include "generator/collector_interface.hpp"
#include "generator/generate_info.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace generator
{
class CrossMwmOsmWaysCollector : public generator::CollectorInterface
{
public:
  struct CrossMwmInfo
  {
    struct SegmentInfo
    {
      SegmentInfo() = default;
      SegmentInfo(uint32_t id, bool forwardIsEnter) : m_segmentId(id), m_forwardIsEnter(forwardIsEnter) {}

      uint32_t m_segmentId = 0;
      bool m_forwardIsEnter = false;
    };

    explicit CrossMwmInfo(uint64_t osmId) : m_osmId(osmId) {}
    CrossMwmInfo(uint64_t osmId, std::vector<SegmentInfo> crossMwmSegments)
      : m_osmId(osmId)
      , m_crossMwmSegments(std::move(crossMwmSegments))
    {}

    bool operator<(CrossMwmInfo const & rhs) const;

    static void Dump(CrossMwmInfo const & info, std::ofstream & output);
    static std::set<CrossMwmInfo> LoadFromFileToSet(std::string const & path);

    uint64_t m_osmId;
    std::vector<SegmentInfo> m_crossMwmSegments;
  };

  CrossMwmOsmWaysCollector(std::string intermediateDir, AffiliationInterfacePtr affiliation);

  // generator::CollectorInterface overrides:
  // @{
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & element) override;

  IMPLEMENT_COLLECTOR_IFACE(CrossMwmOsmWaysCollector);
  void MergeInto(CrossMwmOsmWaysCollector & collector) const;
  // @}

protected:
  // generator::CollectorInterface overrides:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::string m_intermediateDir;
  std::map<std::string, std::vector<CrossMwmInfo>> m_mwmToCrossMwmOsmIds;
  AffiliationInterfacePtr m_affiliation;
};
}  // namespace generator
