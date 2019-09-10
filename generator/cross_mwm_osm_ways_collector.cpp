#include "generator/cross_mwm_osm_ways_collector.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "routing/routing_helpers.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <utility>

namespace generator
{
// CrossMwmOsmWaysCollector ------------------------------------------------------------------------

CrossMwmOsmWaysCollector::CrossMwmOsmWaysCollector(std::string intermediateDir,
                                                   std::string const & targetDir,
                                                   bool haveBordersForWholeWorld)
  : m_intermediateDir(std::move(intermediateDir))
{
  m_affiliation =
      std::make_shared<feature::CountriesFilesAffiliation>(targetDir, haveBordersForWholeWorld);
}

CrossMwmOsmWaysCollector::CrossMwmOsmWaysCollector(
    std::string intermediateDir, std::shared_ptr<feature::CountriesFilesAffiliation> affiliation)
  : m_intermediateDir(std::move(intermediateDir)), m_affiliation(std::move(affiliation))
{
}

std::shared_ptr<CollectorInterface>
CrossMwmOsmWaysCollector::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CrossMwmOsmWaysCollector>(m_intermediateDir, m_affiliation);
}

void CrossMwmOsmWaysCollector::CollectFeature(feature::FeatureBuilder const & fb,
                                              OsmElement const & element)
{
  if (element.m_type != OsmElement::EntityType::Way)
    return;

  if (!routing::IsRoad(fb.GetTypes()))
    return;

  auto const & affiliations = m_affiliation->GetAffiliations(fb);
  if (affiliations.size() == 1)
    return;

  auto const & featurePoints = fb.GetOuterGeometry();

  std::map<std::string, std::vector<bool>> featurePointsEntriesToMwm;
  for (auto const & mwmName : affiliations)
    featurePointsEntriesToMwm[mwmName] = std::vector<bool>(featurePoints.size(), false);

  for (size_t pointNumber = 0; pointNumber < featurePoints.size(); ++pointNumber)
  {
    auto const & point = featurePoints[pointNumber];
    auto const & pointAffiliations = m_affiliation->GetAffiliations(point);
    for (auto const & mwmName : pointAffiliations)
      featurePointsEntriesToMwm[mwmName][pointNumber] = true;
  }

  for (auto const & mwmName : affiliations)
  {
    auto const & entries = featurePointsEntriesToMwm[mwmName];
    std::vector<CrossMwmInfo::SegmentInfo> crossMwmSegments;
    bool prevPointIn = entries[0];
    for (size_t i = 1; i < entries.size(); ++i)
    {
      bool curPointIn = entries[i];
      if (prevPointIn == curPointIn)
        continue;

      prevPointIn = curPointIn;
      crossMwmSegments.emplace_back(i - 1, curPointIn);
    }

    CHECK(!crossMwmSegments.empty(),
         ("Can not find cross mwm segments, mwmName:", mwmName, "fb:", fb));

    m_mwmToCrossMwmOsmIds[mwmName].emplace_back(element.m_id, std::move(crossMwmSegments));
  }
}

void CrossMwmOsmWaysCollector::Save()
{
  auto const & crossMwmOsmWaysDir = base::JoinPath(m_intermediateDir, CROSS_MWM_OSM_WAYS_DIR);
  CHECK(Platform::MkDirChecked(crossMwmOsmWaysDir), (crossMwmOsmWaysDir));

  for (auto const & item : m_mwmToCrossMwmOsmIds)
  {
    auto const & mwmName = item.first;
    auto const & waysInfo = item.second;

    auto const & pathToCrossMwmOsmIds = base::JoinPath(crossMwmOsmWaysDir, mwmName);
    std::ofstream output;
    output.exceptions(std::fstream::failbit | std::fstream::badbit);
    output.open(pathToCrossMwmOsmIds);
    for (auto const & wayInfo : waysInfo)
      CrossMwmInfo::Dump(wayInfo, output);
  }
}

void CrossMwmOsmWaysCollector::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CrossMwmOsmWaysCollector::MergeInto(CrossMwmOsmWaysCollector & collector) const
{
  for (auto const & item : m_mwmToCrossMwmOsmIds)
  {
    auto const & mwmName = item.first;
    auto const & osmIds = item.second;

    auto & otherOsmIds = collector.m_mwmToCrossMwmOsmIds[mwmName];
    otherOsmIds.insert(otherOsmIds.end(), osmIds.cbegin(), osmIds.cend());
  }
}

// CrossMwmOsmWaysCollector::Info ------------------------------------------------------------------

bool CrossMwmOsmWaysCollector::CrossMwmInfo::operator<(CrossMwmInfo const & rhs) const
{
  return m_osmId < rhs.m_osmId;
}

// static
void CrossMwmOsmWaysCollector::CrossMwmInfo::Dump(CrossMwmInfo const & info, std::ofstream & output)
{
  output << base::MakeOsmWay(info.m_osmId) << " " << info.m_crossMwmSegments.size() << " ";
  for (auto const & segmentInfo : info.m_crossMwmSegments)
    output << segmentInfo.m_segmentId << " " << segmentInfo.m_forwardIsEnter << " ";

  output << std::endl;
}

// static
std::set<CrossMwmOsmWaysCollector::CrossMwmInfo>
CrossMwmOsmWaysCollector::CrossMwmInfo::LoadFromFileToSet(std::string const & path)
{
  std::set<CrossMwmInfo> result;
  std::ifstream input;
  input.exceptions(std::fstream::failbit | std::fstream::badbit);
  input.open(path);
  input.exceptions(std::fstream::badbit);

  uint64_t osmId;
  size_t segmentsNumber;
  while (input >> osmId >> segmentsNumber)
  {
    std::vector<SegmentInfo> segments;

    CHECK_NOT_EQUAL(segmentsNumber, 0, ());
    segments.resize(segmentsNumber);
    for (size_t j = 0; j < segmentsNumber; ++j)
      input >> segments[j].m_segmentId >> segments[j].m_forwardIsEnter;

    result.emplace(osmId, std::move(segments));
  }

  return result;
}
}  // namespace generator
