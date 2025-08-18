#include "generator/final_processor_complex.hpp"

#include "generator/final_processor_utils.hpp"
#include "generator/utils.hpp"

#include "base/file_name_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include <iterator>
#include <mutex>

using namespace feature;

namespace generator
{
ComplexFinalProcessor::ComplexFinalProcessor(std::string const & mwmTmpPath, std::string const & outFilename,
                                             size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::Complex)
  , m_mwmTmpPath(mwmTmpPath)
  , m_outFilename(outFilename)
  , m_threadsCount(threadsCount)
{}

void ComplexFinalProcessor::SetGetMainTypeFunction(hierarchy::GetMainTypeFn const & getMainType)
{
  m_getMainType = getMainType;
}

void ComplexFinalProcessor::SetFilter(std::shared_ptr<FilterInterface> const & filter)
{
  m_filter = filter;
}

void ComplexFinalProcessor::SetGetNameFunction(hierarchy::GetNameFn const & getName)
{
  m_getName = getName;
}

void ComplexFinalProcessor::SetPrintFunction(hierarchy::PrintFn const & printFunction)
{
  m_printFunction = printFunction;
}

void ComplexFinalProcessor::UseCentersEnricher(std::string const & mwmPath, std::string const & osm2ftPath)
{
  m_useCentersEnricher = true;
  m_mwmPath = mwmPath;
  m_osm2ftPath = osm2ftPath;
}

std::unique_ptr<hierarchy::HierarchyEntryEnricher> ComplexFinalProcessor::CreateEnricher(
    std::string const & countryName) const
{
  return std::make_unique<hierarchy::HierarchyEntryEnricher>(
      base::JoinPath(m_osm2ftPath, countryName + DATA_FILE_EXTENSION OSM2FEATURE_FILE_EXTENSION),
      base::JoinPath(m_mwmPath, countryName + DATA_FILE_EXTENSION));
}

void ComplexFinalProcessor::UseBuildingPartsInfo(std::string const & filename)
{
  m_buildingPartsFilename = filename;
}

void ComplexFinalProcessor::Process()
{
  if (!m_buildingPartsFilename.empty())
    m_buildingToParts = std::make_unique<BuildingToBuildingPartsMap>(m_buildingPartsFilename);

  std::mutex mutex;
  std::vector<HierarchyEntry> allLines;
  ForEachMwmTmp(m_mwmTmpPath, [&](auto const & name, auto const & path)
  {
    // https://wiki.openstreetmap.org/wiki/Simple_3D_buildings
    // An object with tag 'building:part' is a part of a relation with outline 'building' or
    // is contained in an object with tag 'building'. We will split data and work with
    // these cases separately. First of all let's remove objects with tag building:part is
    // contained in relations. We will add them back after data processing.
    std::unordered_map<base::GeoObjectId, FeatureBuilder> relationBuildingParts;

    auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(path);

    if (m_buildingToParts)
      relationBuildingParts = RemoveRelationBuildingParts(fbs);

    // This case is second. We build a hierarchy using the geometry of objects and their nesting.
    auto trees = hierarchy::BuildHierarchy(std::move(fbs), m_getMainType, m_filter);

    // We remove tree roots with tag 'building:part'.
    base::EraseIf(trees, [](auto const & node)
    {
      static auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
      return buildingPartChecker(node->GetData().GetTypes());
    });

    // We want to transform
    // building
    //        |_building-part
    //                       |_building-part
    // to
    // building
    //        |_building-part
    //        |_building-part
    hierarchy::FlattenBuildingParts(trees);
    // In the end we add objects, which were saved by the collector.
    if (m_buildingToParts)
    {
      hierarchy::AddChildrenTo(trees, [&](auto const & compositeId)
      {
        auto const & ids = m_buildingToParts->GetBuildingPartsByOutlineId(compositeId);
        std::vector<hierarchy::HierarchyPlace> places;
        places.reserve(ids.size());
        for (auto const & id : ids)
        {
          if (relationBuildingParts.count(id) == 0)
            continue;

          places.emplace_back(hierarchy::HierarchyPlace(relationBuildingParts[id]));
        }
        return places;
      });
    }

    // We create and save hierarchy lines.
    hierarchy::HierarchyLinesBuilder hierarchyBuilder(std::move(trees));
    if (m_useCentersEnricher)
      hierarchyBuilder.SetHierarchyEntryEnricher(CreateEnricher(name));

    hierarchyBuilder.SetCountry(name);
    hierarchyBuilder.SetGetMainTypeFunction(m_getMainType);
    hierarchyBuilder.SetGetNameFunction(m_getName);

    auto lines = hierarchyBuilder.GetHierarchyLines();

    std::lock_guard<std::mutex> lock(mutex);
    allLines.insert(std::cend(allLines), std::make_move_iterator(std::begin(lines)),
                    std::make_move_iterator(std::end(lines)));
  }, m_threadsCount);

  WriteLines(allLines);
}

std::unordered_map<base::GeoObjectId, FeatureBuilder> ComplexFinalProcessor::RemoveRelationBuildingParts(
    std::vector<FeatureBuilder> & fbs)
{
  CHECK(m_buildingToParts, ());

  auto it = std::partition(std::begin(fbs), std::end(fbs), [&](auto const & fb)
  { return !m_buildingToParts->HasBuildingPart(fb.GetMostGenericOsmId()); });

  std::unordered_map<base::GeoObjectId, FeatureBuilder> buildingParts;
  buildingParts.reserve(static_cast<size_t>(std::distance(it, std::end(fbs))));

  std::transform(it, std::end(fbs), std::inserter(buildingParts, std::begin(buildingParts)), [](auto && fb)
  {
    auto const id = fb.GetMostGenericOsmId();
    return std::make_pair(id, std::move(fb));
  });

  fbs.resize(static_cast<size_t>(std::distance(std::begin(fbs), it)));
  return buildingParts;
}

void ComplexFinalProcessor::WriteLines(std::vector<HierarchyEntry> const & lines)
{
  auto stream = OfstreamWithExceptions(m_outFilename);
  for (auto const & line : lines)
    stream << m_printFunction(line) << '\n';
}
}  // namespace generator
