#pragma once
#include "generator/collector_interface.hpp"

#include <unordered_map>

namespace generator
{

class AddressesHolder
{
public:
  struct AddressInfo
  {
    std::string m_house, m_street, m_postcode;
    std::string m_house2;  // end point house number for the 2nd stage

    std::string FormatRange() const;
  };

private:
  std::unordered_map<uint64_t, AddressInfo> m_addresses;

public:
  void Add(feature::FeatureBuilder const & fb);
  bool Update(feature::FeatureBuilder & fb) const;

  void MergeInto(AddressesHolder & holder) const;
  void Deserialize(std::string const & filePath);

  AddressInfo const * Get(uint64_t id) const
  {
    auto const i = m_addresses.find(id);
    return (i != m_addresses.end()) ? &i->second : nullptr;
  }
};

class AddressesCollector : public CollectorInterface
{
  AddressesHolder m_nodeAddresses;

  struct WayInfo
  {
    // OSM Node IDs here.
    uint64_t m_beg, m_end;
  };
  // OSM Way ID is a key here.
  std::unordered_map<uint64_t, WayInfo> m_interpolWays;

public:
  explicit AddressesCollector(std::string const & filename);

  /// @todo We can simplify this (and some others) Collector's logic if we will have strict
  /// processing order (like in o5m): Nodes, Ways, Relations.
  /// And make mutable CollectFeature(FeatureBuilder & fb).

  /// @name CollectorInterface overrides:
  /// @{
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const override;
  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & element) override;
  void Save() override;
  /// @}

  IMPLEMENT_COLLECTOR_IFACE(AddressesCollector);
  void MergeInto(AddressesCollector & collector) const;
};

}  // namespace generator
