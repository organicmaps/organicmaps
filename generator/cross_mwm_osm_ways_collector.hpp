#pragma once

#include "generator/affiliation.hpp"
#include "generator/collector_interface.hpp"
#include "generator/generate_info.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <set>

namespace generator
{
class CrossMwmOsmWaysCollector : public generator::CollectorInterface
{
public:
  explicit CrossMwmOsmWaysCollector(feature::GenerateInfo const & info);

  explicit CrossMwmOsmWaysCollector(std::string intermediateDir,
                                    std::shared_ptr<feature::CountriesFilesAffiliation> affiliation);

  struct Info
  {
    struct SegmentInfo
    {
      SegmentInfo() = default;
      SegmentInfo(uint32_t id, bool forwardIsEnter)
          : m_segmentId(id), m_forwardIsEnter(forwardIsEnter) {}

      uint32_t m_segmentId = 0;
      bool m_forwardIsEnter = false;
    };

    bool operator<(Info const & rhs) const;

    Info(uint64_t osmId, std::vector<SegmentInfo> crossMwmSegments)
        : m_osmId(osmId), m_crossMwmSegments(std::move(crossMwmSegments)) {}

    explicit Info(uint64_t osmId) : m_osmId(osmId) {}

    static void Dump(Info const & info, std::ofstream & output);
    static std::set<Info> LoadFromFileToSet(std::string const & path);

    uint64_t m_osmId;
    std::vector<SegmentInfo> m_crossMwmSegments;
  };

  // generator::CollectorInterface overrides:
  // @{
  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<cache::IntermediateDataReader> const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & element) override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(CrossMwmOsmWaysCollector & collector) const override;
  // @}

private:
  std::string m_intermediateDir;
  std::map<std::string, std::vector<Info>> m_mwmToCrossMwmOsmIds;
  std::shared_ptr<feature::CountriesFilesAffiliation> m_affiliation;
};
}  // namespace generator
