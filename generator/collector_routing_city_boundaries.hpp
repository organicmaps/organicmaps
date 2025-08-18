#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_maker.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <memory>

namespace generator
{
class PlaceBoundariesHolder
{
public:
  struct Locality
  {
    Locality() = default;
    Locality(std::string const & placeType, OsmElement const & elem);

    // Used in cpp module only.
    template <class Sink>
    void Serialize(Sink & sink) const;
    template <class Source>
    void Deserialize(Source & src);

    /// @param[in]  placeName Original Node place's name if available to match.
    /// @return Is this boundary better than rhs.
    bool IsBetterBoundary(Locality const & rhs, std::string const & placeName = {}) const;
    bool IsPoint() const { return m_boundary.empty(); }
    bool IsHonestCity() const { return m_place >= ftypes::LocalityType::City; }
    double GetApproxArea() const;

    ftypes::LocalityType GetPlace() const;
    uint64_t GetPopulation() const;

    void AssignNodeParams(Locality const & node);

    bool TestValid() const;

    bool IsInBoundary(m2::PointD const & pt) const;
    void RecalcBoundaryRect();

    // Valid if m_boundary is empty or m_adminLevel > 0.
    m2::PointD m_center{0, 0};

    /// @todo Do not take into account holes, store only outer geometry.
    using PointSeq = feature::FeatureBuilder::PointSeq;
    std::vector<PointSeq> m_boundary;
    m2::RectD m_boundaryRect;

    uint8_t m_adminLevel = 0;

    std::string m_name;

    friend std::string DebugPrint(Locality const & l);

  private:
    uint64_t m_population = 0;
    uint64_t m_populationFromNode = 0;

    ftypes::LocalityType m_placeFromNode = ftypes::LocalityType::None;
    ftypes::LocalityType m_place = ftypes::LocalityType::None;
  };

  void Serialize(std::string const & fileName) const;
  void Deserialize(std::string const & fileName);

  using IDType = base::GeoObjectId;
  void Add(IDType id, Locality && loc, IDType nodeID);

  /// @note Mutable function!
  template <class FnT>
  void ForEachLocality(FnT && fn)
  {
    for (auto & loc : m_data)
      fn(loc);
  }

  m2::RectD GetBoundaryRect(IDType id) const
  {
    auto const it = m_id2index.find(id);
    if (it != m_id2index.end())
      return m_data[it->second].m_boundaryRect;
    return {};
  }

  int GetIndex(IDType id) const;

  Locality const * GetBestBoundary(std::vector<IDType> const & ids, m2::PointD const & center) const;

private:
  // Value is an index in m_data vector.
  std::unordered_map<IDType, uint64_t> m_id2index;
  std::vector<Locality> m_data;
};

class PlaceBoundariesBuilder
{
public:
  using Locality = PlaceBoundariesHolder::Locality;
  using IDType = PlaceBoundariesHolder::IDType;

  void Add(Locality && loc, IDType id, std::vector<uint64_t> const & nodes);
  void MergeInto(PlaceBoundariesBuilder & dest) const;
  void Save(std::string const & fileName);

private:
  std::unordered_map<IDType, Locality> m_id2loc;
  using IDsSetT = std::unordered_set<IDType>;
  std::unordered_map<IDType, IDsSetT> m_node2rel;
  std::unordered_map<std::string, IDsSetT> m_name2rel;
};

class RoutingCityBoundariesCollector : public CollectorInterface
{
public:
  RoutingCityBoundariesCollector(std::string const & filename, IDRInterfacePtr const & cache);

  /// @name CollectorInterface overrides:
  /// @{
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const override;
  void Collect(OsmElement const & elem) override;
  void Save() override;
  /// @}

  IMPLEMENT_COLLECTOR_IFACE(RoutingCityBoundariesCollector);
  void MergeInto(RoutingCityBoundariesCollector & collector) const;

  using Locality = PlaceBoundariesBuilder::Locality;

private:
  PlaceBoundariesBuilder m_builder;

  std::shared_ptr<cache::IntermediateDataReaderInterface> m_cache;
  FeatureMakerSimple m_featureMakerSimple;
};
}  // namespace generator
