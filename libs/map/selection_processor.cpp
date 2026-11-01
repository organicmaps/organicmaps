#include "map/selection_processor.hpp"
#include "map/framework.hpp"

#include "editor/osm_editor.hpp"

#include "search/reverse_geocoder.hpp"

#include "storage/country_info_getter.hpp"

#include "descriptions/loader.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "coding/point_coding.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/triangle2d.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <limits>

SelectionProcessor::SelectionProcessor(Framework const & framework) : m_fw(framework) {}

bool SelectionProcessor::CanEditMapForPosition(m2::PointD const & position) const
{
  return m_fw.m_storage.IsAllowedToEditVersion(m_fw.m_infoGetter->GetRegionCountryId(position));
}

void SelectionProcessor::SetPlacePageLocation(place_page::Info & info) const
{
  if (info.GetCountryId().empty())
    info.SetCountryId(m_fw.m_infoGetter->GetRegionCountryId(info.GetMercator()));

  if (info.GetTopmostCountryIds().empty())
  {
    storage::CountriesVec countries;
    m_fw.m_storage.GetTopmostNodesFor(info.GetCountryId(), countries);
    info.SetTopmostCountryIds(std::move(countries));
  }
}

void SelectionProcessor::FillDescriptions(FeatureType & ft, place_page::Info & info) const
{
  if (!ft.GetID().m_mwmId.IsAlive())
    return;

  auto const & regionData = ft.GetID().m_mwmId.GetInfo()->GetRegionData();
  auto const deviceLang = StringUtf8Multilang::GetLangIndex(languages::GetCurrentMapLanguage());
  auto const langPriority = feature::GetDescriptionLangPriority(regionData, deviceLang);

  std::string wikiDescription = m_fw.m_descriptionsLoader->GetWikiDescription(ft.GetID(), langPriority);
  if (!wikiDescription.empty())
  {
    info.SetWikiDescription(std::move(wikiDescription));
    info.SetOpeningMode(m_fw.m_routingManager.IsRoutingActive() ? place_page::OpeningMode::Preview
                                                                : place_page::OpeningMode::PreviewPlus);
  }

  std::string_view const osmDescriptionValue = ft.GetMetadata(feature::Metadata::FMD_DESCRIPTION);
  if (osmDescriptionValue.empty())
    return;

  LangsBufferT langCodes;
  for (auto const & lang : languages::GetSystemPreferred())
  {
    auto const code = StringUtf8Multilang::GetLangIndex(languages::Normalize(lang));
    if (code != StringUtf8Multilang::kUnsupportedLanguageCode)
      langCodes.push_back(code);
  }
  langCodes.push_back(StringUtf8Multilang::kDefaultCode);
  langCodes.push_back(StringUtf8Multilang::kEnglishCode);

  auto const osmDescriptionMultilang = StringUtf8Multilang::FromBuffer(std::string(osmDescriptionValue));
  std::string_view osmDescription = osmDescriptionMultilang.GetBestString(langCodes);
  if (osmDescription.empty())
    osmDescription = osmDescriptionMultilang.GetFirstString();
  info.SetOSMDescription(std::string(osmDescription));
}

void SelectionProcessor::FillInfoFromFeatureType(FeatureType & ft, place_page::Info & info) const
{
  using namespace ftypes;

  auto const featureStatus = osm::Editor::Instance().GetFeatureStatus(ft.GetID());
  ASSERT_NOT_EQUAL(featureStatus, FeatureStatus::Deleted, ());
  info.SetFeatureStatus(featureStatus);

  if (IsAddressObjectChecker::Instance()(ft))
  {
    auto const & dataSource = m_fw.m_featuresFetcher.GetDataSource();
    search::ReverseGeocoder::Address addr;
    if (search::ReverseGeocoder(dataSource).GetFeatureAddress(ft, addr))
      info.SetAddress(addr.FormatAddress());
  }

  info.SetFromFeatureType(ft);

  FillDescriptions(ft, info);

  bool const isMapVersionEditable = CanEditMapForPosition(info.GetMercator());
  bool const canEditOrAdd = featureStatus != FeatureStatus::Obsolete && isMapVersionEditable;
  info.SetCanEditOrAdd(canEditOrAdd);

  auto const locType = IsLocalityChecker::Instance().GetType(info.GetTypes());
  if (locType == LocalityType::State || locType == LocalityType::Country)
  {
    storage::CountryId countryId = m_fw.m_infoGetter->GetRegionCountryId(info.GetMercator());
    storage::CountriesVec countries;
    m_fw.m_storage.GetTopmostNodesFor(countryId, countries, locType == LocalityType::State ? 1 : 0 /* level */);
    if (countries.size() == 1)
      countryId = countries.front();

    info.SetCountryId(countryId);
    info.SetTopmostCountryIds(std::move(countries));
  }
}

void SelectionProcessor::FillFeatureInfo(FeatureID const & fid, place_page::Info & info) const
{
  if (!fid.IsValid())
  {
    LOG(LERROR, ("FeatureID is invalid:", fid));
    return;
  }

  auto const & dataSource = m_fw.m_featuresFetcher.GetDataSource();
  FeaturesLoaderGuard const guard(dataSource, fid.m_mwmId);
  auto ft = guard.GetFeatureByIndex(fid.m_index);
  if (!ft)
  {
    LOG(LERROR, ("Feature can't be loaded:", fid));
    return;
  }

  FillInfoFromFeatureType(*ft, info);
}

void SelectionProcessor::FillPointInfo(place_page::Info & info, m2::PointD const & mercator,
                                       std::string const & customTitle /* = {} */,
                                       FeatureMatcher const & matcher /* = nullptr */) const
{
  auto const fid = GetFeatureAtPoint(mercator, matcher);
  if (fid.IsValid())
  {
    auto const & dataSource = m_fw.m_featuresFetcher.GetDataSource();
    dataSource.ReadFeature([&](FeatureType & ft) { FillInfoFromFeatureType(ft, info); }, fid);
    // This line overwrites mercator center from area feature which can be far away.
    info.SetMercator(mercator);
  }
  else
  {
    FillNotMatchedPlaceInfo(info, mercator, customTitle);
  }
}

void SelectionProcessor::FillNotMatchedPlaceInfo(place_page::Info & info, m2::PointD const & mercator,
                                                 std::string const & customTitle /* = {} */) const
{
  if (customTitle.empty())
    info.SetCustomNameWithCoordinates(mercator, m_fw.m_stringsBundle.GetString("core_placepage_unknown_place"));
  else
    info.SetCustomName(customTitle);
  info.SetCanEditOrAdd(CanEditMapForPosition(mercator));
  info.SetMercator(mercator);
}

FeatureID SelectionProcessor::TapFeatures::GetBest() const
{
  if (m_poi.IsValid())
    return m_poi;
  if (m_line.IsValid())
    return m_line;
  // Building has higher priority over non-building areas.
  if (m_building.IsValid())
    return m_building;
  return m_area;
}

namespace
{
// Minimum squared mercator distance from |pt| to line feature |ft|'s polyline at |scale|.
// Uses ParametrizedSegment::SquaredDistanceToPoint (no trig).
double LineMinSquaredMercator(FeatureType & ft, m2::PointD const & pt, int scale)
{
  double res = std::numeric_limits<double>::max();
  ft.ForEachSegment([&](m2::PointD const & p1, m2::PointD const & p2)
  {
    m2::ParametrizedSegment<m2::PointD> const seg(p1, p2);
    double const sq = seg.SquaredDistanceToPoint(pt);
    if (sq < res)
      res = sq;
  }, scale);
  return res;
}

// Squared mercator distance from |pt| to area feature |ft| (zero if |pt| is inside the polygon).
// Early-exits the first time a triangle contains the point. Uses per-edge squared distance,
// not earth distance, so no trig per segment.
double AreaMinSquaredMercator(FeatureType & ft, m2::PointD const & pt, int scale)
{
  double res = std::numeric_limits<double>::max();
  ft.ForEachTriangle([&](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    if (res == 0.0)
      return;

    if (m2::IsPointInsideTriangle(pt, p1, p2, p3))
    {
      res = 0.0;
      return;
    }

    auto updateEdge = [&](m2::PointD const & a, m2::PointD const & b)
    {
      double const sq = m2::ParametrizedSegment<m2::PointD>(a, b).SquaredDistanceToPoint(pt);
      if (sq < res)
        res = sq;
    };
    updateEdge(p1, p2);
    updateEdge(p2, p3);
    updateEdge(p3, p1);
  }, scale);
  return res;
}
}  // namespace

/// @todo Unify with indexer::ForEachFeatureAtPoint.
SelectionProcessor::TapFeatures SelectionProcessor::FindFeaturesInRect(
    m2::PointD const & mercator, m2::RectD const & rect, FeatureMatcher const & matcher /* = nullptr */) const
{
  auto const & isIsoline = ftypes::IsIsolineChecker::Instance();
  auto const & isCoastline = ftypes::IsCoastlineChecker::Instance();
  auto const & isBuilding = ftypes::IsBuildingChecker::Instance();

  constexpr int kScale = scales::GetUpperScale();

  TapFeatures result;

  // Square diagonal = 2 * sqrt(2) * radius => radius^2 = diagonal^2 / 8.
  double const thresholdMercSq = rect.LeftBottom().SquaredLength(rect.RightTop()) / 8.0;
  double closestPoint = thresholdMercSq;
  double closestLine = thresholdMercSq;
  double smallestBuilding = std::numeric_limits<double>::max();
  double smallestArea = std::numeric_limits<double>::max();

  m_fw.m_featuresFetcher.GetDataSource().ForEachInRect([&](FeatureType & ft)
  {
    if (matcher && !matcher(ft))
      return;

    m2::RectD const limitRect = ft.GetLimitRect(kScale);
    if (!rect.IsIntersect(limitRect))
      return;

    switch (ft.GetGeomType())
    {
    case feature::GeomType::Point:
    {
      double const sq = mercator.SquaredLength(ft.GetCenter());
      if (sq < closestPoint)
      {
        closestPoint = sq;
        result.m_poi = ft.GetID();
      }
      break;
    }
    case feature::GeomType::Line:
    {
      if (isIsoline(ft))
        return;

      double const sq = LineMinSquaredMercator(ft, mercator, kScale);
      if (sq < thresholdMercSq)
      {
        result.m_lineCandidates.emplace_back(sq, ft.GetID());
        if (sq < closestLine)
        {
          closestLine = sq;
          result.m_line = ft.GetID();
        }
      }
      break;
    }
    case feature::GeomType::Area:
    {
      feature::TypesHolder types(ft);
      if (isCoastline(types))
        return;

      // Consider areas with the selection point inside only.
      // kMwmPointAccuracy is more reasonable threshold (~1m on equator) than previous 1cm.
      if (AreaMinSquaredMercator(ft, mercator, kScale) > math::Pow2(kMwmPointAccuracy))
        return;

      // Choose the smallest (inner) feature. This is the most reasonable logic for area selection (ATM).
      double const area = limitRect.Area();
      if (isBuilding(types))
      {
        if (area < smallestBuilding)
        {
          smallestBuilding = area;
          result.m_building = ft.GetID();
        }
      }
      else if (area < smallestArea)
      {
        smallestArea = area;
        result.m_area = ft.GetID();
      }
      break;
    }
    case feature::GeomType::Undefined: ASSERT(false, ()); break;
    }
  }, rect, kScale);

  std::sort(result.m_lineCandidates.begin(), result.m_lineCandidates.end());
  return result;
}

FeatureID SelectionProcessor::GetFeatureAtPoint(m2::PointD const & mercator,
                                                FeatureMatcher const & matcher /* = nullptr */) const
{
  // ForEachFeatureAtPoint uses 1.1m rect and 3m line threshold at GetUpperScale.
  constexpr double kQueryRadiusM = 2;
  auto const searchRect = mercator::RectByCenterXYAndSizeInMeters(mercator, kQueryRadiusM);
  return FindFeaturesInRect(mercator, searchRect, matcher).GetBest();
}
