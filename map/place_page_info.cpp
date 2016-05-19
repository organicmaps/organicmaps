#include "place_page_info.hpp"

#include "indexer/osm_editor.hpp"

#include "platform/preferred_languages.hpp"

namespace place_page
{
char const * Info::kSubtitleSeparator = " • ";
char const * Info::kStarSymbol = "★";
char const * Info::kMountainSymbol = "▲";
char const * Info::kEmptyRatingSymbol = "-";
char const * Info::kPricingSymbol = "$";

bool Info::IsFeature() const { return m_featureID.IsValid(); }
bool Info::IsBookmark() const { return m_bac != MakeEmptyBookmarkAndCategory(); }
bool Info::IsMyPosition() const { return m_isMyPosition; }
bool Info::HasApiUrl() const { return !m_apiUrl.empty(); }
bool Info::IsEditable() const { return m_isEditable; }
bool Info::HasWifi() const { return GetInternet() == osm::Internet::Wlan; }
bool Info::ShouldShowAddPlace() const { return !IsFeature() || (!IsPointType() && !IsBuilding()); }

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

string Info::GetRatingFormatted() const
{
  if (!IsBookingObject())
    return "";

  auto const r = GetMetadata().Get(feature::Metadata::FMD_RATING);
  char const * rating = r.empty() ? kEmptyRatingSymbol : r.c_str();
  int const sz = snprintf(nullptr, 0, m_localizedRatingString.c_str(), rating);

  vector<char> buf(sz + 1);
  snprintf(&buf[0], buf.size(), m_localizedRatingString.c_str(), rating);
  return string(buf.begin(), buf.end());
}

string Info::GetApproximatelyPricing() const
{
  if (!IsBookingObject())
    return "";
  uint64_t pricing;
  strings::to_uint64(GetMetadata().Get(feature::Metadata::FMD_PRICE_RATE), pricing);
  string result;
  for (auto i = 0; i < pricing; i++)
    result.append("$");

  return result;
}

void Info::SetMercator(m2::PointD const & mercator) { m_mercator = mercator; }
}  // namespace place_page
