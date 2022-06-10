#include "search/intermediate_result.hpp"

#include "search/geometry_utils.hpp"
#include "search/reverse_geocoder.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/cuisines.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "platform/measurement_utils.hpp"

#include "geometry/angles.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "3party/opening_hours/opening_hours.hpp"

namespace search
{
using namespace std;

namespace
{
class SkipRegionInfo
{
  static size_t const kCount = 2;
  uint32_t m_types[kCount];

public:
  SkipRegionInfo()
  {
    char const * arr[][2] = {
      {"place", "continent"},
      {"place", "country"}
    };
    static_assert(kCount == ARRAY_SIZE(arr), "");

    Classificator const & c = classif();
    for (size_t i = 0; i < kCount; ++i)
      m_types[i] = c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2));
  }

  bool IsSkip(uint32_t type) const
  {
    for (uint32_t t : m_types)
    {
      if (t == type)
        return true;
    }
    return false;
  }
};
}  // namespace

// PreRankerResult ---------------------------------------------------------------------------------
PreRankerResult::PreRankerResult(FeatureID const & id, PreRankingInfo const & info,
                                 vector<ResultTracer::Branch> const & provenance)
: m_id(id), m_info(info)
, m_isRelaxed(base::IsExist(provenance, ResultTracer::Branch::Relaxed))
#ifdef SEARCH_USE_PROVENANCE
, m_provenance(provenance)
#endif
{
  ASSERT(m_id.IsValid(), ());

  m_matchedTokensNumber = 0;
  for (auto const & r : m_info.m_tokenRanges)
    m_matchedTokensNumber += r.Size();
}

// static
bool PreRankerResult::LessRankAndPopularity(PreRankerResult const & lhs, PreRankerResult const & rhs)
{
  if (lhs.m_info.m_rank != rhs.m_info.m_rank)
    return lhs.m_info.m_rank > rhs.m_info.m_rank;
  if (lhs.m_info.m_popularity != rhs.m_info.m_popularity)
    return lhs.m_info.m_popularity > rhs.m_info.m_popularity;
  return lhs.m_info.m_distanceToPivot < rhs.m_info.m_distanceToPivot;
}

// static
bool PreRankerResult::LessDistance(PreRankerResult const & lhs, PreRankerResult const & rhs)
{
  if (lhs.m_info.m_distanceToPivot != rhs.m_info.m_distanceToPivot)
    return lhs.m_info.m_distanceToPivot < rhs.m_info.m_distanceToPivot;
  return lhs.m_info.m_rank > rhs.m_info.m_rank;
}

// static
bool PreRankerResult::LessByExactMatch(PreRankerResult const & lhs, PreRankerResult const & rhs)
{
  auto const lhsScore = lhs.m_info.m_exactMatch && lhs.m_info.m_allTokensUsed;
  auto const rhsScore = rhs.m_info.m_exactMatch && rhs.m_info.m_allTokensUsed;
  if (lhsScore != rhsScore)
    return lhsScore;

  if (lhs.GetInnermostTokensNumber() != rhs.GetInnermostTokensNumber())
    return lhs.GetInnermostTokensNumber() > rhs.GetInnermostTokensNumber();

  if (lhs.GetMatchedTokensNumber() != rhs.GetMatchedTokensNumber())
    return lhs.GetMatchedTokensNumber() > rhs.GetMatchedTokensNumber();

  return LessDistance(lhs, rhs);
}

bool PreRankerResult::CategoriesComparator::operator()(PreRankerResult const & lhs,
                                                       PreRankerResult const & rhs) const
{
  if (m_positionIsInsideViewport)
    return lhs.GetDistance() < rhs.GetDistance();

  if (m_detailedScale)
  {
    bool const lhsInside = m_viewport.IsPointInside(lhs.GetInfo().m_center);
    bool const rhsInside = m_viewport.IsPointInside(rhs.GetInfo().m_center);
    if (lhsInside && !rhsInside)
      return true;
    if (rhsInside && !lhsInside)
      return false;
  }
  return lhs.GetPopularity() > rhs.GetPopularity();
}

// RankerResult ------------------------------------------------------------------------------------
RankerResult::RankerResult(FeatureType & f, m2::PointD const & center, m2::PointD const & pivot,
                           string displayName, string const & fileName)
  : m_id(f.GetID())
  , m_types(f)
  , m_str(std::move(displayName))
  , m_resultType(ftypes::IsBuildingChecker::Instance()(m_types) ? Type::Building : Type::Feature)
  , m_geomType(f.GetGeomType())
{
  ASSERT(m_id.IsValid(), ());
  ASSERT(!m_types.Empty(), ());

  m_types.SortBySpec();

  m_region.SetParams(fileName, center);
  m_distance = PointDistance(center, pivot);

  FillDetails(f, m_details);
}

RankerResult::RankerResult(FeatureType & ft, m2::PointD const & pivot, std::string const & fileName)
  : RankerResult(ft, feature::GetCenter(ft, FeatureType::WORST_GEOMETRY),
                 pivot, std::string(ft.GetReadableName()), fileName)
{
}

RankerResult::RankerResult(double lat, double lon)
  : m_str("(" + measurement_utils::FormatLatLon(lat, lon) + ")"), m_resultType(Type::LatLon)
{
  m_region.SetParams(string(), mercator::FromLatLon(lat, lon));
}

RankerResult::RankerResult(m2::PointD const & coord, string_view postcode)
  : m_str(postcode), m_resultType(Type::Postcode)
{
  m_region.SetParams(string(), coord);
}

bool RankerResult::GetCountryId(storage::CountryInfoGetter const & infoGetter, uint32_t ftype,
                                storage::CountryId & countryId) const
{
  static SkipRegionInfo const checker;
  if (checker.IsSkip(ftype))
    return false;
  return m_region.GetCountryId(infoGetter, countryId);
}

bool RankerResult::IsEqualCommon(RankerResult const & r) const
{
  if ((m_geomType != r.m_geomType) || (m_str != r.m_str))
    return false;

  auto const bestType = GetBestType();
  auto const rBestType = r.GetBestType();
  if (bestType == rBestType)
    return true;

  auto const & checker = ftypes::IsWayChecker::Instance();
  return checker(bestType) && checker(rBestType);
}

bool RankerResult::IsStreet() const { return ftypes::IsStreetOrSquareChecker::Instance()(m_types); }

uint32_t RankerResult::GetBestType(vector<uint32_t> const & preferredTypes) const
{
  ASSERT(is_sorted(preferredTypes.begin(), preferredTypes.end()), ());
  if (!preferredTypes.empty())
  {
    for (uint32_t type : m_types)
    {
      if (binary_search(preferredTypes.begin(), preferredTypes.end(), type))
        return type;
    }
  }

  return m_types.GetBestType();
}

// RankerResult::RegionInfo ------------------------------------------------------------------------
bool RankerResult::RegionInfo::GetCountryId(storage::CountryInfoGetter const & infoGetter,
                                            storage::CountryId & countryId) const
{
  if (!m_countryId.empty())
  {
    countryId = m_countryId;
    return true;
  }

  auto const id = infoGetter.GetRegionCountryId(m_point);
  if (id != storage::kInvalidCountryId)
  {
    countryId = id;
    return true;
  }

  return false;
}

// Functions ---------------------------------------------------------------------------------------
void FillDetails(FeatureType & ft, Result::Details & details)
{
  if (details.m_isInitialized)
    return;

  details.m_airportIata = ft.GetMetadata(feature::Metadata::FMD_AIRPORT_IATA);
  details.m_brand = ft.GetMetadata(feature::Metadata::FMD_BRAND);

  /// @todo Avoid temporary string when OpeningHours (boost::spirit) will allow string_view.
  std::string const openHours(ft.GetMetadata(feature::Metadata::FMD_OPEN_HOURS));
  if (!openHours.empty())
  {
    osmoh::OpeningHours const oh((std::string(openHours)));
    /// @todo We should check closed/open time for specific feature's timezone.
    time_t const now = time(nullptr);
    if (oh.IsValid() && !oh.IsUnknown(now))
    {
      details.m_isOpenNow = oh.IsOpen(now) ? osm::Yes : osm::No;
      // In else case value is osm::Unknown, it's set in preview's constructor.
      details.m_minutesUntilOpen = (oh.GetNextTimeOpen(now) - now) / 60;
      details.m_minutesUntilClosed = (oh.GetNextTimeClosed(now) - now) / 60;
    }
  }

  if (strings::to_uint(ft.GetMetadata(feature::Metadata::FMD_STARS), details.m_stars))
    details.m_stars = std::min(details.m_stars, uint8_t(5));
  else
    details.m_stars = 0;

  string const kFieldsSeparator = " • ";
  auto const cuisines = feature::GetLocalizedCuisines(feature::TypesHolder(ft));
  details.m_cuisine = strings::JoinStrings(cuisines, kFieldsSeparator);

  auto const roadShields = feature::GetRoadShieldsNames(ft.GetRoadNumber());
  details.m_roadShields = strings::JoinStrings(roadShields, kFieldsSeparator);

  details.m_isInitialized = true;
}

string DebugPrint(RankerResult const & r)
{
  stringstream ss;
  ss << "RankerResult ["
     << "Name: " << r.GetName()
     << "; Type: " << classif().GetReadableObjectName(r.GetBestType());

#ifdef SEARCH_USE_PROVENANCE
    if (!r.m_provenance.empty())
      ss << "; Provenance: " << ::DebugPrint(r.m_provenance);
#endif

     ss << "; " << DebugPrint(r.GetRankingInfo())
     << "; Linear model rank: " << r.GetLinearModelRank()
     << "]";
  return ss.str();
}
}  // namespace search
