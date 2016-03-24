#include "place_page_info.hpp"

#include "indexer/osm_editor.hpp"

#include "platform/preferred_languages.hpp"

namespace place_page
{
char const * Info::kSubtitleSeparator = " • ";
char const * Info::kStarSymbol = "★";
char const * Info::kMountainSymbol = "▲";

bool Info::IsFeature() const { return m_featureID.IsValid(); }
bool Info::IsBookmark() const { return m_bac != MakeEmptyBookmarkAndCategory(); }
bool Info::IsMyPosition() const { return m_isMyPosition; }
bool Info::HasApiUrl() const { return !m_apiUrl.empty(); }
bool Info::IsEditable() const { return m_isEditable; }
bool Info::HasWifi() const { return GetInternet() == osm::Internet::Wlan; }

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
    return {};

  vector<string> values;

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
string const & Info::GetApiUrl() const { return m_apiUrl; }
void Info::SetMercator(m2::PointD const & mercator) { m_mercator = mercator; }
}  // namespace place_page
