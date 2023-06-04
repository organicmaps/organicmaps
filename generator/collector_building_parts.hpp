#pragma once

#include "generator/collector_interface.hpp"
#include "generator/composite_id.hpp"

#include "coding/file_writer.hpp"

#include "base/geo_object_id.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

struct OsmElement;

class FileReader;

template <typename>
class ReaderSource;

namespace feature
{
class FeatureBuilder;
}  // namespace feature

namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

namespace generator
{
// BuildingPartsCollector collects ids of building parts from relations.
class BuildingPartsCollector : public CollectorInterface
{
public:
  struct BuildingParts
  {
    friend bool operator<(BuildingParts const & lhs, BuildingParts const & rhs)
    {
      return std::tie(lhs.m_id, lhs.m_buildingParts) < std::tie(rhs.m_id, rhs.m_buildingParts);
    }

    static void Write(FileWriter & writer, BuildingParts const & pb);
    static BuildingParts Read(ReaderSource<FileReader> & src);

    CompositeId m_id;
    std::vector<base::GeoObjectId> m_buildingParts;
  };

  explicit BuildingPartsCollector(std::string const & filename,
                                  std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const &) override;

  void Finish() override;

  void Merge(CollectorInterface const & collector) override;
  void MergeInto(BuildingPartsCollector & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  base::GeoObjectId FindTopRelation(base::GeoObjectId elId);
  std::vector<base::GeoObjectId> FindAllBuildingParts(base::GeoObjectId const & id);

  std::shared_ptr<cache::IntermediateDataReaderInterface> m_cache;
  std::unique_ptr<FileWriter> m_writer;
};

class BuildingToBuildingPartsMap
{
public:
  explicit BuildingToBuildingPartsMap(std::string const & filename);

  bool HasBuildingPart(base::GeoObjectId const & id);
  std::vector<base::GeoObjectId> const & GetBuildingPartsByOutlineId(CompositeId const & id);

private:
  std::vector<base::GeoObjectId> m_buildingParts;
  std::vector<std::pair<CompositeId, std::vector<base::GeoObjectId>>> m_outlineToBuildingPart;
};
}  // namespace generator
