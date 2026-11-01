#pragma once

#include "generator/affiliation.hpp"
#include "generator/collector_interface.hpp"

#include "indexer/cycling_infrastructure.hpp"

#include "coding/file_writer.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace generator
{
inline constexpr char kCyclingJunctionsFilename[] = "cycling_junctions";

struct CyclingWay
{
  struct Node
  {
    uint64_t m_id;
    m2::PointD m_point;
    bool operator==(Node const &) const = default;
  };
  uint64_t m_id = 0;
  std::string m_layer, m_level, m_sides;
  bool m_reversed = false;
  std::vector<Node> m_nodes;
  bool operator==(CyclingWay const &) const = default;
};

// Only shared node IDs authorize a join. Coordinates identify the approached
// side, not connectivity or permission to cross the road.
std::map<uint64_t, cycling::Sides> FindCyclingJunctions(std::vector<CyclingWay> const & roads,
                                                        std::vector<CyclingWay> const & connectors,
                                                        std::function<bool(uint64_t)> const & blocked);

class CyclingJunctionCollector : public CollectorInterface
{
public:
  CyclingJunctionCollector(std::string const & filename, AffiliationInterfacePtr affiliation);
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;
  void Collect(OsmElement const &) override;
  void CollectFeature(feature::FeatureBuilder const &, OsmElement const &) override;
  void Finish() override;
  IMPLEMENT_COLLECTOR_IFACE(CyclingJunctionCollector);
  void MergeInto(CyclingJunctionCollector &) const;

protected:
  void Save() override;

private:
  AffiliationInterfacePtr m_affiliation;
  std::unique_ptr<FileWriter> m_writer;
};

void ApplyCountryCyclingJunctions(std::string const & filename, std::string const & country,
                                  std::string const & rawFeaturesPath);
}  // namespace generator
