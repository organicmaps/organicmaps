#pragma once

#include "generator/collector_building_parts.hpp"
#include "generator/feature_builder.hpp"
#include "generator/filter_interface.hpp"
#include "generator/final_processor_interface.hpp"
#include "generator/hierarchy.hpp"
#include "generator/hierarchy_entry.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace generator
{
// Class ComplexFinalProcessor generates hierarchies for each previously filtered mwm.tmp file.
// Warning: If the border separates the complex, then a situation is possible in which two logically
// identical complexes are generated, but with different representations.
class ComplexFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  ComplexFinalProcessor(std::string const & mwmTmpPath, std::string const & outFilename, size_t threadsCount);

  void SetGetMainTypeFunction(hierarchy::GetMainTypeFn const & getMainType);
  void SetFilter(std::shared_ptr<FilterInterface> const & filter);
  void SetGetNameFunction(hierarchy::GetNameFn const & getName);
  void SetPrintFunction(hierarchy::PrintFn const & printFunction);

  void UseCentersEnricher(std::string const & mwmPath, std::string const & osm2ftPath);
  void UseBuildingPartsInfo(std::string const & filename);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  std::unique_ptr<hierarchy::HierarchyEntryEnricher> CreateEnricher(std::string const & countryName) const;
  void WriteLines(std::vector<HierarchyEntry> const & lines);
  std::unordered_map<base::GeoObjectId, feature::FeatureBuilder> RemoveRelationBuildingParts(
      std::vector<feature::FeatureBuilder> & fbs);

  hierarchy::GetMainTypeFn m_getMainType;
  hierarchy::PrintFn m_printFunction;
  hierarchy::GetNameFn m_getName;
  std::shared_ptr<FilterInterface> m_filter;
  std::unique_ptr<BuildingToBuildingPartsMap> m_buildingToParts;
  bool m_useCentersEnricher = false;
  std::string m_mwmTmpPath;
  std::string m_outFilename;
  std::string m_mwmPath;
  std::string m_osm2ftPath;
  std::string m_buildingPartsFilename;
  size_t m_threadsCount;
};
}  // namespace generator
