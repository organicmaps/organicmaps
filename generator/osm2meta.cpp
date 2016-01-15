#include "generator/osm2meta.hpp"

#include "coding/url_encode.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/regex.hpp"
#include "std/cctype.hpp"

string MetadataTagProcessorImpl::ValidateAndFormat_maxspeed(string const & v) const
{
  if (!ftypes::IsSpeedCamChecker::Instance()(m_params.m_Types))
    return string();

  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_stars(string const & v) const
{
  if (v.empty())
    return string();

  // We are accepting stars from 1 to 7.
  if (v[0] <= '0' || v[0] > '7')
    return string();

  // Ignore numbers large then 9.
  if (v.size() > 1 && ::isdigit(v[1]))
    return string();

  return string(1, v[0]);
}

string MetadataTagProcessorImpl::ValidateAndFormat_operator(string const & v) const
{
  auto const & isATM = ftypes::IsATMChecker::Instance();
  auto const & isFuelStation = ftypes::IsFuelStationChecker::Instance();

  if (!(isATM(m_params.m_Types) || isFuelStation(m_params.m_Types)))
    return string();

  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_url(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_phone(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_opening_hours(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_ele(string const & v) const
{
  auto const & isPeak = ftypes::IsPeakChecker::Instance();
  if (!isPeak(m_params.m_Types))
    return string();
  double val = 0;
  if(!strings::to_double(v, val) || val == 0)
    return string();
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_turn_lanes(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_turn_lanes_forward(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_turn_lanes_backward(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_email(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_postcode(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_flats(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_height(string const & v) const
{
  double val = 0;
  string corrected(v, 0, v.find(" "));
  if(!strings::to_double(corrected, val) || val == 0)
    return string();
  ostringstream ss;
  ss << fixed << setprecision(1) << val;
  return ss.str();
}

string MetadataTagProcessorImpl::ValidateAndFormat_building_levels(string const & v) const
{
  double constexpr kMetersPerLevel = 3;
  double val = 0;
  if(!strings::to_double(v, val) || val == 0)
    return string();
  ostringstream ss;
  ss << fixed << setprecision(1) << (val * kMetersPerLevel);
  return ss.str();
}

string MetadataTagProcessorImpl::ValidateAndFormat_denomination(string const & v) const
{
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_cuisine(string v) const
{
  strings::MakeLowerCaseInplace(v);
  v = regex_replace(v, regex("[;,]\\s*"), ";");
  v = regex_replace(v, regex("\\s+"), "_");
  strings::Trim(v, ";_");
  return v;
}

string MetadataTagProcessorImpl::ValidateAndFormat_wikipedia(string v) const
{
  strings::Trim(v);
  // Normalize by converting full URL to "lang:title" if necessary
  // (some OSMers do not follow standard for wikipedia tag).
  static string const base = ".wikipedia.org/wiki/";
  auto const baseIndex = v.find(base);
  if (baseIndex != string::npos)
  {
    auto const baseSize = base.size();
    // Do not allow urls without article name after /wiki/.
    if (v.size() > baseIndex + baseSize)
    {
      auto const slashIndex = v.rfind('/', baseIndex);
      if (slashIndex != string::npos && slashIndex + 1 != baseIndex)
      {
        // Normalize article title according to OSM standards.
        string title = UrlDecode(v.substr(baseIndex + baseSize));
        replace(title.begin(), title.end(), '_', ' ');
        return v.substr(slashIndex + 1, baseIndex - slashIndex - 1) + ":" + title;
      }
    }
    LOG(LDEBUG, ("Invalid Wikipedia tag value:", v));
    return string();
  }
  // Standard case: "lang:Article Name With Spaces".
  // Language and article are at least 2 chars each.
  auto const colonIndex = v.find(':');
  if (colonIndex == string::npos || colonIndex < 2 || colonIndex + 2 > v.size())
  {
    LOG(LDEBUG, ("Invalid Wikipedia tag value:", v));
    return string();
  }
  // Check if it's not a random/invalid link.
  if (v.find("//") != string::npos || v.find(".org") != string::npos)
  {
    LOG(LDEBUG, ("Invalid Wikipedia tag value:", v));
    return string();
  }
  // Normalize to OSM standards.
  string normalized(v);
  replace(normalized.begin() + colonIndex, normalized.end(), '_', ' ');
  return normalized;
}
