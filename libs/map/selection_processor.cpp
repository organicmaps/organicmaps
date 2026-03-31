#include "map/selection_processor.hpp"
#include "map/framework.hpp"

#include "editor/osm_editor.hpp"

#include "search/reverse_geocoder.hpp"

#include "storage/country_info_getter.hpp"

#include "descriptions/loader.hpp"

#include "coding/point_coding.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/mercator.hpp"

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
  ASSERT_NOT_EQUAL(featureStatus, FeatureStatus::Deleted, ("Deleted features cannot be selected from UI."));
  info.SetFeatureStatus(featureStatus);

  if (IsAddressObjectChecker::Instance()(ft))
  {
    auto const & dataSource = m_fw.m_featuresFetcher.GetDataSource();
    search::ReverseGeocoder const coder(dataSource);
    search::ReverseGeocoder::Address addr;
    coder.GetNearbyAddress(feature::GetCenter(ft), 0.5 /* maxDistanceM */, addr, true /* placeAsStreet */);
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
                                       FeatureMatcher && matcher /* = nullptr */) const
{
  auto const fid = GetFeatureAtPoint(mercator, std::move(matcher));
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
  if (m_bestLine.IsValid())
    return m_bestLine;
  // Building has higher priority over non-building areas.
  if (m_building.IsValid())
    return m_building;
  return m_bestArea;
}

SelectionProcessor::TapFeatures SelectionProcessor::FindFeaturesInRect(
    m2::PointD const & mercator, m2::RectD const & searchRect, double lineDistThresholdM,
    FeatureMatcher const & matcher /* = nullptr */) const
{
  auto const & isIsoline = ftypes::IsIsolineChecker::Instance();
  auto const & isCoastline = ftypes::IsCoastlineChecker::Instance();
  auto const & isBuilding = ftypes::IsBuildingChecker::Instance();

  constexpr int kScale = scales::GetUpperScale();

  TapFeatures result;
  double closestLineDist = lineDistThresholdM;
  double closestAreaDist = std::numeric_limits<double>::max();
  bool matched = false;

  auto const & dataSource = m_fw.m_featuresFetcher.GetDataSource();
  dataSource.ForEachInRect([&](FeatureType & ft)
  {
    if (matched)
      return;

    bool geometryHit = false;

    switch (ft.GetGeomType())
    {
    case feature::GeomType::Point:
      if (searchRect.IsPointInside(ft.GetCenter()))
      {
        geometryHit = true;
        result.m_poi = ft.GetID();
      }
      break;
    case feature::GeomType::Line:
    {
      if (isIsoline(ft))
        return;
      double const dist = feature::GetMinDistanceMeters(ft, mercator);
      if (dist < lineDistThresholdM)
      {
        geometryHit = true;
        result.m_lineCandidates.emplace_back(dist, ft.GetID());
        if (dist < closestLineDist)
        {
          closestLineDist = dist;
          result.m_bestLine = ft.GetID();
        }
      }
      break;
    }
    case feature::GeomType::Area:
    {
      feature::TypesHolder types(ft);
      if (isCoastline(types))
        return;

      // Inflate limit rect to tolerate small coordinate errors (e.g. from editor).
      auto limitRect = ft.GetLimitRect(kScale);
      limitRect.Inflate(kMwmPointAccuracy, kMwmPointAccuracy);
      if (!limitRect.IsPointInside(mercator))
        return;
      if (feature::GetMinDistanceMeters(ft, mercator) > 0.01)
        return;

      geometryHit = true;

      if (isBuilding(types))
      {
        if (!result.m_building.IsValid())
          result.m_building = ft.GetID();
      }
      else
      {
        double const dist = mercator::DistanceOnEarth(mercator, feature::GetCenter(ft));
        if (dist < closestAreaDist)
        {
          closestAreaDist = dist;
          result.m_bestArea = ft.GetID();
        }
      }
      break;
    }
    case feature::GeomType::Undefined: ASSERT(false, ("case feature::GeomType::Undefined")); break;
    }

    // Matcher runs after geometry hit-testing to avoid matching features
    // from neighboring spatial-index cells that don't actually contain the point.
    if (geometryHit && matcher && matcher(ft))
    {
      result.m_poi = ft.GetID();
      matched = true;
    }
  }, searchRect, kScale);

  if (!matched)
    std::sort(result.m_lineCandidates.begin(), result.m_lineCandidates.end());

  return result;
}

FeatureID SelectionProcessor::GetFeatureAtPoint(m2::PointD const & mercator,
                                                FeatureMatcher && matcher /* = nullptr */) const
{
  // ForEachFeatureAtPoint uses 1.1m rect and 3m line threshold at GetUpperScale.
  constexpr double kQueryRectWidthM = 1.1;
  constexpr double kLineDistThresholdM = 3.0;
  auto const searchRect = mercator::RectByCenterXYAndSizeInMeters(mercator, kQueryRectWidthM);
  return FindFeaturesInRect(mercator, searchRect, kLineDistThresholdM, std::move(matcher)).GetBest();
}
