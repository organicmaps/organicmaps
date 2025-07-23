#include "generator/cross_mwm_osm_ways_collector.hpp"

#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/osm_element.hpp"

#include "routing/routing_helpers.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <utility>

namespace generator
{
// CrossMwmOsmWaysCollector ------------------------------------------------------------------------

CrossMwmOsmWaysCollector::CrossMwmOsmWaysCollector(std::string intermediateDir, AffiliationInterfacePtr affiliation)
  : m_intermediateDir(std::move(intermediateDir))
  , m_affiliation(std::move(affiliation))
{}

std::shared_ptr<CollectorInterface> CrossMwmOsmWaysCollector::Clone(IDRInterfacePtr const &) const
{
  return std::make_shared<CrossMwmOsmWaysCollector>(m_intermediateDir, m_affiliation);
}

void CrossMwmOsmWaysCollector::CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & element)
{
  if (element.m_type != OsmElement::EntityType::Way)
    return;

  if (!routing::IsRoad(fb.GetTypes()))
    return;

  auto const & affiliations = m_affiliation->GetAffiliations(fb);
  if (affiliations.empty())
    return;

  auto const & featurePoints = fb.GetOuterGeometry();
  std::map<std::string, std::vector<bool>> featurePointsEntriesToMwm;
  for (auto const & mwmName : affiliations)
    featurePointsEntriesToMwm[mwmName] = std::vector<bool>(featurePoints.size(), false);

  std::vector<size_t> pointsAffiliationsNumber(featurePoints.size());
  for (size_t pointNumber = 0; pointNumber < featurePoints.size(); ++pointNumber)
  {
    auto const & point = featurePoints[pointNumber];
    auto const & pointAffiliations = m_affiliation->GetAffiliations(point);
    for (auto const & mwmName : pointAffiliations)
    {
      // Skip mwms which are not present in the map: these are GetAffiliations() false positives.
      auto it = featurePointsEntriesToMwm.find(mwmName);
      if (it == featurePointsEntriesToMwm.end())
        continue;
      it->second[pointNumber] = true;
    }

    // TODO (@gmoryes)
    //  Uncomment this check after: https://github.com/mapsme/omim/pull/10996
    //  It could happend when point places between mwm's borders and doesn't belong to any of them.
    // CHECK_GREATER(pointAffiliations.size(), 0, ());
    pointsAffiliationsNumber[pointNumber] = pointAffiliations.size();
  }

  for (auto const & item : featurePointsEntriesToMwm)
  {
    auto const & mwmName = item.first;
    auto const & entries = item.second;
    std::vector<CrossMwmInfo::SegmentInfo> crossMwmSegments;
    bool prevPointIn = entries[0];
    for (size_t i = 1; i < entries.size(); ++i)
    {
      bool curPointIn = entries[i];
      // We must be sure below that either both points belong to mwm either one of them belongs.
      if (!curPointIn && !prevPointIn)
        continue;
      // If pointsAffiliationsNumber[i] is more than 1, that means point lies on the mwms' borders
      // And belongs to both. So we consider such segment as cross mwm segment.
      // So the condition that segment certainly lies inside of mwm is:
      // both points inside and both points belong to only this mwm.
      if (prevPointIn && curPointIn && pointsAffiliationsNumber[i] == 1 && pointsAffiliationsNumber[i - 1] == 1)
        continue;

      bool forwardIsEnter;
      if (prevPointIn != curPointIn)
        forwardIsEnter = curPointIn;
      else if (pointsAffiliationsNumber[i - 1] != 1)
        forwardIsEnter = curPointIn;
      else if (pointsAffiliationsNumber[i] != 1)
        forwardIsEnter = !prevPointIn;
      else
        UNREACHABLE();
      prevPointIn = curPointIn;
      crossMwmSegments.emplace_back(i - 1 /* segmentId */, forwardIsEnter);
    }

    if (crossMwmSegments.empty())
      return;

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

void CrossMwmOsmWaysCollector::OrderCollectedData()
{
  auto const & crossMwmOsmWaysDir = base::JoinPath(m_intermediateDir, CROSS_MWM_OSM_WAYS_DIR);
  for (auto const & item : m_mwmToCrossMwmOsmIds)
    OrderTextFileByLine(base::JoinPath(crossMwmOsmWaysDir, item.first));
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
std::set<CrossMwmOsmWaysCollector::CrossMwmInfo> CrossMwmOsmWaysCollector::CrossMwmInfo::LoadFromFileToSet(
    std::string const & path)
{
  std::ifstream input(path);
  if (!input)
  {
    LOG(LWARNING, ("No info about cross mwm ways:", path));
    return {};
  }
  input.exceptions(std::fstream::badbit);

  std::set<CrossMwmInfo> result;
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
