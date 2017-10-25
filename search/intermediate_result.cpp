#include "intermediate_result.hpp"
#include "geometry_utils.hpp"
#include "reverse_geocoder.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/ftypes_sponsored.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"

#include "platform/measurement_utils.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "3party/opening_hours/opening_hours.hpp"

namespace search
{
namespace
{
class SkipRegionInfo
{
  static size_t const m_count = 2;
  uint32_t m_types[m_count];

public:
  SkipRegionInfo()
  {
    char const * arr[][2] = {
      {"place", "continent"},
      {"place", "country"}
    };
    static_assert(m_count == ARRAY_SIZE(arr), "");

    Classificator const & c = classif();
    for (size_t i = 0; i < m_count; ++i)
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

char const * const kEmptyRatingSymbol = "-";
char const * const kPricingSymbol = "$";

void ProcessMetadata(FeatureType const & ft, Result::Metadata & meta)
{
  if (meta.m_isInitialized)
    return;

  feature::Metadata const & src = ft.GetMetadata();

  meta.m_cuisine = src.Get(feature::Metadata::FMD_CUISINE);

  string const openHours = src.Get(feature::Metadata::FMD_OPEN_HOURS);
  if (!openHours.empty())
  {
    osmoh::OpeningHours const oh(openHours);
    // TODO: We should check closed/open time for specific feature's timezone.
    time_t const now = time(nullptr);
    if (oh.IsValid() && !oh.IsUnknown(now))
      meta.m_isOpenNow = oh.IsOpen(now) ? osm::Yes : osm::No;
    // In else case value us osm::Unknown, it's set in preview's constructor.
  }

  if (strings::to_int(src.Get(feature::Metadata::FMD_STARS), meta.m_stars))
    meta.m_stars = my::clamp(meta.m_stars, 0, 5);
  else
    meta.m_stars = 0;

  bool const isSponsoredHotel = ftypes::IsBookingChecker::Instance()(ft);
  meta.m_isSponsoredHotel = isSponsoredHotel;
  meta.m_isHotel = ftypes::IsHotelChecker::Instance()(ft);

  if (isSponsoredHotel)
  {
    auto const r = src.Get(feature::Metadata::FMD_RATING);
    char const * const rating = r.empty() ? kEmptyRatingSymbol : r.c_str();
    meta.m_hotelRating = rating;

    int pricing;
    if (!strings::to_int(src.Get(feature::Metadata::FMD_PRICE_RATE), pricing))
      pricing = 0;
    string pricingStr;
    CHECK_GREATER_OR_EQUAL(pricing, 0, ("Pricing must be positive!"));
    for (auto i = 0; i < pricing; i++)
      pricingStr.append(kPricingSymbol);

    meta.m_hotelApproximatePricing = pricingStr;
  }

  meta.m_isInitialized = true;
}

PreRankerResult::PreRankerResult(FeatureID const & fID, PreRankingInfo const & info)
  : m_id(fID), m_info(info)
{
  ASSERT(m_id.IsValid(), ());
}

// static
bool PreRankerResult::LessRank(PreRankerResult const & r1, PreRankerResult const & r2)
{
  if (r1.m_info.m_rank != r2.m_info.m_rank)
    return r1.m_info.m_rank > r2.m_info.m_rank;
  return r1.m_info.m_distanceToPivot < r2.m_info.m_distanceToPivot;
}

// static
bool PreRankerResult::LessDistance(PreRankerResult const & r1, PreRankerResult const & r2)
{
  if (r1.m_info.m_distanceToPivot != r2.m_info.m_distanceToPivot)
    return r1.m_info.m_distanceToPivot < r2.m_info.m_distanceToPivot;
  return r1.m_info.m_rank > r2.m_info.m_rank;
}

RankerResult::RankerResult(FeatureType const & f, m2::PointD const & center,
                           m2::PointD const & pivot, string const & displayName,
                           string const & fileName)
  : m_id(f.GetID())
  , m_types(f)
  , m_str(displayName)
  , m_resultType(ftypes::IsBuildingChecker::Instance()(m_types) ? TYPE_BUILDING : TYPE_FEATURE)
  , m_geomType(f.GetFeatureType())
{
  ASSERT(m_id.IsValid(), ());
  ASSERT(!m_types.Empty(), ());

  m_types.SortBySpec();

  m_region.SetParams(fileName, center);
  m_distance = PointDistance(center, pivot);

  ProcessMetadata(f, m_metadata);
}

RankerResult::RankerResult(double lat, double lon)
  : m_str("(" + measurement_utils::FormatLatLon(lat, lon) + ")"), m_resultType(TYPE_LATLON)
{
  m_region.SetParams(string(), MercatorBounds::FromLatLon(lat, lon));
}

string RankerResult::GetRegionName(storage::CountryInfoGetter const & infoGetter,
                                   uint32_t fType) const
{
  static SkipRegionInfo const checker;
  if (checker.IsSkip(fType))
    return string();

  storage::CountryInfo info;
  m_region.GetRegion(infoGetter, info);
  return info.m_name;
}

bool RankerResult::IsEqualCommon(RankerResult const & r) const
{
  return m_geomType == r.m_geomType && GetBestType() == r.GetBestType() && m_str == r.m_str;
}

bool RankerResult::IsStreet() const
{
  return m_geomType == feature::GEOM_LINE && ftypes::IsStreetChecker::Instance()(m_types);
}

string RankerResult::DebugPrint() const
{
  stringstream ss;
  ss << "IntermediateResult [ "
     << "Name: " << m_str
     << "; Type: " << GetBestType()
     << "; " << search::DebugPrint(m_info)
     << "; Linear model rank: " << m_info.GetLinearModelRank()
     << " ]";
  return ss.str();
}

uint32_t RankerResult::GetBestType(set<uint32_t> const * pPrefferedTypes) const
{
  if (pPrefferedTypes)
  {
    for (uint32_t type : m_types)
    {
      if (pPrefferedTypes->count(type) > 0)
        return type;
    }
  }

  // Do type truncate (2-level is enough for search results) only for
  // non-preffered types (types from categories leave original).
  uint32_t type = m_types.GetBestType();
  ftype::TruncValue(type, 2);
  return type;
}

void RankerResult::RegionInfo::GetRegion(storage::CountryInfoGetter const & infoGetter,
                                         storage::CountryInfo & info) const
{
  if (!m_file.empty())
    infoGetter.GetRegionInfo(m_file, info);
  else
    infoGetter.GetRegionInfo(m_point, info);
}
}  // namespace search
