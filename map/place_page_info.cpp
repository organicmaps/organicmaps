#include "place_page_info.hpp"

#include "indexer/osm_editor.hpp"

#include "platform/preferred_languages.hpp"

namespace place_page
{
char const * const Info::kSubtitleSeparator = " • ";
char const * const Info::kStarSymbol = "★";
char const * const Info::kMountainSymbol = "▲";
char const * const Info::kEmptyRatingSymbol = "-";
char const * const Info::kPricingSymbol = "$";

bool Info::IsFeature() const { return m_featureID.IsValid(); }
bool Info::IsBookmark() const { return m_bac != MakeEmptyBookmarkAndCategory(); }
bool Info::IsMyPosition() const { return m_isMyPosition; }
bool Info::HasApiUrl() const { return !m_apiUrl.empty(); }
bool Info::IsEditable() const { return m_isEditable && m_isDataEditable; }
bool Info::IsDataEditable() const { return m_isDataEditable; }
bool Info::HasWifi() const { return GetInternet() == osm::Internet::Wlan; }

bool Info::ShouldShowAddPlace() const
{
  return !IsSponsoredHotel() &&
      (!IsFeature() || (!IsPointType() && !IsBuilding())) &&
      m_isDataEditable;
}

bool Info::IsSponsoredHotel() const { return m_isSponsoredHotel; }

string Info::FormatNewBookmarkName() const
{
  string const title = GetTitle();
  if (title.empty())
    return GetLocalizedType();
  return title;
}

string Info::GetTitle() const
{
  if (!m_customName.empty())
    return m_customName;

  // Prefer names in native language over default ones.
  int8_t const langCode = StringUtf8Multilang::GetLangIndex(languages::GetCurrentNorm());
  if (langCode != StringUtf8Multilang::kUnsupportedLanguageCode)
  {
    string native;
    if (m_name.GetString(langCode, native))
      return native;
  }

  return GetDefaultName();
}

string Info::GetSubtitle() const
{
  if (!IsFeature())
  {
    if (IsBookmark())
      return m_bookmarkCategoryName;
    return {};
  }

  vector<string> values;

  // Bookmark category.
  if (IsBookmark())
    values.push_back(m_bookmarkCategoryName);

  // Type.
  values.push_back(GetLocalizedType());

  // Cuisines.
  for (string const & cuisine : GetLocalizedCuisines())
    values.push_back(cuisine);

  // Stars.
  string const stars = FormatStars();
  if (!stars.empty())
    values.push_back(stars);

  // Operator.
  string const op = GetOperator();
  if (!op.empty())
    values.push_back(op);

  // Elevation.
  string const eleStr = GetElevationFormatted();
  if (!eleStr.empty())
    values.push_back(kMountainSymbol + eleStr);
  if (HasWifi())
    values.push_back(m_localizedWifiString);

  return strings::JoinStrings(values, kSubtitleSeparator);
}

string Info::FormatStars() const
{
  string stars;
  for (int i = 0; i < GetStars(); ++i)
    stars.append(kStarSymbol);
  return stars;
}

string Info::GetCustomName() const { return m_customName; }
BookmarkAndCategory Info::GetBookmarkAndCategory() const { return m_bac; }
string Info::GetBookmarkCategoryName() const { return m_bookmarkCategoryName; }
string const & Info::GetApiUrl() const { return m_apiUrl; }

string const & Info::GetSponsoredBookingUrl() const { return m_sponsoredBookingUrl; }
string const & Info::GetSponsoredDescriptionUrl() const {return m_sponsoredDescriptionUrl; }

string Info::GetRatingFormatted() const
{
  if (!IsSponsoredHotel())
    return string();

  auto const r = GetMetadata().Get(feature::Metadata::FMD_RATING);
  char const * rating = r.empty() ? kEmptyRatingSymbol : r.c_str();
  int const size = snprintf(nullptr, 0, m_localizedRatingString.c_str(), rating);
  if (size < 0)
  {
    LOG(LERROR, ("Incorrect size for string:", m_localizedRatingString, ", rating:", rating));
    return string();
  }

  vector<char> buf(size + 1);
  snprintf(buf.data(), buf.size(), m_localizedRatingString.c_str(), rating);
  return string(buf.begin(), buf.end());
}

string Info::GetApproximatePricing() const
{
  if (!IsSponsoredHotel())
    return string();

  int pricing;
  strings::to_int(GetMetadata().Get(feature::Metadata::FMD_PRICE_RATE), pricing);
  string result;
  for (auto i = 0; i < pricing; i++)
    result.append(kPricingSymbol);

  return result;
}

void Info::SetMercator(m2::PointD const & mercator) { m_mercator = mercator; }
}  // namespace place_page
