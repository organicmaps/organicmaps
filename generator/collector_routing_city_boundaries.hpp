#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_maker.hpp"
#include "generator/intermediate_data.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "coding/reader.hpp"

#include <cstdint>
#include <memory>
#include <tuple>

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

class RoutingCityBoundariesWriter;

class RoutingCityBoundariesCollector : public CollectorInterface
{
public:
  struct LocalityData
  {
    LocalityData() = default;
    LocalityData(uint64_t population, ftypes::LocalityType place, m2::PointD const & position)
      : m_population(population), m_place(place), m_position(position)
    {
    }

    friend bool operator<(LocalityData const & lhs, LocalityData const & rhs)
    {
      return std::tie(lhs.m_population, lhs.m_place, lhs.m_position) <
             std::tie(rhs.m_population, rhs.m_place, rhs.m_position);
    }

    static void Serialize(FileWriter & writer, LocalityData const & localityData);
    static LocalityData Deserialize(ReaderSource<FileReader> & reader);

    uint64_t m_population = 0;
    ftypes::LocalityType m_place = ftypes::LocalityType::None;
    m2::PointD m_position = m2::PointD::Zero();
  };

  RoutingCityBoundariesCollector(
      std::string const & filename, std::string const & dumpFilename,
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache = {}) const override;

  void Collect(OsmElement const & osmElement) override;
  void Finish() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(RoutingCityBoundariesCollector & collector) const override;

  static bool FilterOsmElement(OsmElement const & osmElement);
  void Process(feature::FeatureBuilder & feature, OsmElement const & osmElement);

protected:
  // CollectorInterface overrides:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::unique_ptr<RoutingCityBoundariesWriter> m_writer;
  std::shared_ptr<cache::IntermediateDataReaderInterface> m_cache;
  FeatureMakerSimple m_featureMakerSimple;
  std::string m_dumpFilename;
};

class RoutingCityBoundariesWriter
{
public:
  using LocalityData = RoutingCityBoundariesCollector::LocalityData;

  static std::string GetNodeToLocalityDataFilename(std::string const & filename);
  static std::string GetNodeToBoundariesFilename(std::string const & filename);
  static std::string GetBoundariesFilename(std::string const & filename);

  explicit RoutingCityBoundariesWriter(std::string const & filename);
  ~RoutingCityBoundariesWriter();

  void Process(uint64_t nodeOsmId, LocalityData const & localityData);
  void Process(uint64_t nodeOsmId, feature::FeatureBuilder const & feature);
  void Process(feature::FeatureBuilder const & feature);

  void Reset();
  void MergeInto(RoutingCityBoundariesWriter & writer);
  void Save(std::string const & finalFileName, std::string const & dumpFilename);
  void OrderCollectedData(std::string const & finalFileName, std::string const & dumpFilename);

private:
  using MinAccuracy = feature::serialization_policy::MinSize;
  using FeatureWriter = feature::FeatureBuilderWriter<MinAccuracy>;

  std::string m_nodeOsmIdToLocalityDataFilename;
  std::string m_nodeOsmIdToBoundariesFilename;
  std::string m_finalBoundariesGeometryFilename;

  uint64_t m_nodeOsmIdToLocalityDataCount = 0;
  uint64_t m_nodeOsmIdToBoundariesCount = 0;

  std::unique_ptr<FileWriter> m_nodeOsmIdToLocalityDataWriter;
  std::unique_ptr<FileWriter> m_nodeOsmIdToBoundariesWriter;
  std::unique_ptr<FileWriter> m_finalBoundariesGeometryWriter;
};
}  // namespace generator
