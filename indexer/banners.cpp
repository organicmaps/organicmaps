#include "indexer/banners.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "platform/platform.hpp"

#include "coding/reader_streambuf.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

namespace
{
time_t constexpr kEternity = 0;

// Convert ISO date to unix time.
bool StringToTimestamp(string const & s, time_t & result)
{
  istringstream is(s);
  tm time;
  is >> get_time(&time, "%Y-%m-%d");
  CHECK(!is.fail(), ("Wrong date format:", s, "(expecting YYYY-MM-DD)"));

  time.tm_sec = time.tm_min = time.tm_hour = 0;

  time_t timestamp = mktime(&time);
  if (timestamp < 0)
    return false;

  result = timestamp;
  return true;
}
}  // namespace

namespace banner
{
string Banner::GetProperty(string const & name) const
{
  if (name == "lang")
  {
    return "ru";  // TODO(@zverik): this variable, {mwmlang}, {country} etc.
  }
  else
  {
    auto const property = m_properties.find(name);
    if (property != m_properties.end())
      return property->second;
  }
  return {};
}

Banner::Banner(string const & id)
  : m_id(id), m_messageBase("banner_" + id), m_activeAfter(time(nullptr)), m_activeBefore(kEternity)
{
}

bool Banner::IsActive() const
{
  if (IsEmpty())
    return false;
  time_t const now = time(nullptr);
  return now >= m_activeAfter && (m_activeBefore == kEternity || now < m_activeBefore);
}

void Banner::SetProperty(string const & name, string const & value)
{
  if (name == "messages")
  {
    m_messageBase = value;
  }
  else if (name == "icon")
  {
    m_iconName = value;
  }
  else if (name == "url")
  {
    CHECK(strings::StartsWith(value, "http://") || strings::StartsWith(value, "https://"),
          ("URL without a protocol for banner", m_id));
    m_defaultUrl = value;
  }
  else if (name == "start")
  {
    if (!StringToTimestamp(value, m_activeAfter))
      LOG(LERROR, ("Wrong start date", value, "for banner", m_id));
  }
  else if (name == "end")
  {
    if (!StringToTimestamp(value, m_activeBefore))
      LOG(LERROR, ("Wrong end date", value, "for banner", m_id));
    else
      m_activeBefore += 24 * 60 * 60;  // Add a day so we don't miss one
  }
  else
  {
    m_properties.emplace(make_pair(name, value));
  }
}

string Banner::GetFormattedUrl(string const & url) const
{
  string baseUrl = url.empty() ? m_defaultUrl : url;
  auto start = baseUrl.find('{');
  while (start != string::npos)
  {
    auto end = baseUrl.find('}', start + 1);
    if (end == string::npos)
      break;
    string value = GetProperty(baseUrl.substr(start + 1, end - start - 1));
    if (!value.empty())
    {
      baseUrl.replace(start, end - start + 1, value);
      end -= end - start + 1 - value.length();
    }
    start = baseUrl.find('{', end + 1);
  }
  return baseUrl;
}

void BannerSet::ReadBanners(istream & s)
{
  m_banners.clear();

  Banner banner;
  string type;
  int lineNumber = 1;
  for (string line; getline(s, line); ++lineNumber)
  {
    strings::Trim(line);
    if (line.empty() || line.front() == '#')
      continue;

    auto const equals = line.find('=');
    if (equals == string::npos)
    {
      // Section header, should be in square brackets.
      CHECK(line.front() == '[' && line.back() == ']', ("Unknown syntax at line", lineNumber));
      strings::Trim(line, " \t[]");
      CHECK(!line.empty(), ("Empty banner ID at line", lineNumber));
      if (!banner.IsEmpty())
        Add(banner, type);
      banner = Banner(line);
      type = "sponsored-banner-" + line;
    }
    else
    {
      // Variable definition, must be inside a section.
      CHECK(!banner.IsEmpty(), ("Variable definition outside a section at line", lineNumber));
      string name = line.substr(0, equals);
      string value = line.substr(equals + 1);
      strings::Trim(name);
      CHECK(!name.empty(), ("Empty variable name at line", lineNumber));
      strings::Trim(value);
      if (name == "type")
        type = value;
      else
        banner.SetProperty(name, value);
    }
  }
  if (!banner.IsEmpty())
    Add(banner, type);
}

void BannerSet::Add(Banner const & banner, string const & type)
{
  vector<string> v;
  strings::Tokenize(type, "-", MakeBackInsertFunctor(v));
  uint32_t const ctype = classif().GetTypeByPathSafe(v);
  if (ctype == 0)
  {
    LOG(LWARNING, ("Missing type", type, "for a banner"));
  }
  else
  {
    CHECK(m_banners.find(ctype) == m_banners.end(), ("Duplicate banner type", type));
    m_banners.emplace(make_pair(ctype, banner));
  }
}

bool BannerSet::HasBannerForType(uint32_t type) const
{
  return m_banners.find(type) != m_banners.end();
}

Banner const & BannerSet::GetBannerForType(uint32_t type) const
{
  auto const result = m_banners.find(type);
  CHECK(result != m_banners.end(), ("GetBannerForType() for absent banner"));
  return result->second;
}

bool BannerSet::HasBannerForFeature(FeatureType const & ft) const
{
  bool result = false;
  ft.ForEachType([this, &result](uint32_t type)
  {
    if (!result && HasBannerForType(type))
      result = true;
  });
  return result;
}

Banner const & BannerSet::GetBannerForFeature(FeatureType const & ft) const
{
  vector<uint32_t> types;
  ft.ForEachType([this, &types](uint32_t type)
  {
    if (types.empty() && HasBannerForType(type))
      types.push_back(type);
  });
  CHECK(!types.empty(), ("No banners for the feature", ft));
  return GetBannerForType(types.front());
}

void BannerSet::LoadBanners()
{
  try
  {
    auto reader = GetPlatform().GetReader(BANNERS_FILE);
    ReaderStreamBuf buffer(move(reader));
    istream s(&buffer);
    ReadBanners(s);
  }
  catch (FileAbsentException const &)
  {
    LOG(LWARNING, ("No", BANNERS_FILE, "found"));
    return;
  }
}
}  // namespace banner
