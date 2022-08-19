#include "indexer/validate_and_format_contacts.hpp"

#include "coding/url.hpp"

#include "base/string_utils.hpp"

namespace osm
{
using namespace std;

static auto const s_instaRegex = regex(R"(^@?[A-Za-z0-9_][A-Za-z0-9_.]{0,28}[A-Za-z0-9_]$)");
static auto const s_twitterRegex = regex(R"(^@?[A-Za-z0-9_]{1,15}$)");
static auto const s_badVkRegex = regex(R"(^\d\d\d.+$)");
static auto const s_goodVkRegex = regex(R"(^[A-Za-z0-9_.]{5,32}$)");
static auto const s_lineRegex = regex(R"(^[a-z0-9-_.]{4,20}$)");

char const * const kWebsiteProtocols[] = {"http://", "https://"};
size_t const kWebsiteProtocolDefaultIndex = 0;

size_t GetProtocolNameLength(string const & website)
{
  for (auto const & protocol : kWebsiteProtocols)
  {
    if (strings::StartsWith(website, protocol))
      return strlen(protocol);
  }
  return 0;
}

bool IsProtocolSpecified(string const & website)
{
  return GetProtocolNameLength(website) > 0;
}

// TODO: Current implementation looks only for restricted symbols from ASCII block ignoring
//       unicode. Need to find all restricted *Unicode* symbols
//       from https://www.facebook.com/pages/create page and verify those symbols
//       using MakeUniString or utf8cpp.
bool containsInvalidFBSymbol(string const & facebookPage, size_t startIndex = 0)
{
  auto const size = facebookPage.size();
  for (auto i=startIndex; i<size; ++i)
  {
    const char ch = facebookPage[i];
    // Forbid all ASCII symbols except '-', '.', and '_'
    if ((ch >= ' ' && ch <= ',') ||
        ch == '/' ||
        (ch >= ':' && ch <= '@') ||
        (ch >= '[' && ch <= '^') ||
        ch == '`' ||
        (ch >= '{' && ch <= '~'))
      return true;
  }
  return false;
}

std::string ValidateAndFormat_website(std::string const & v)
{
  if (!v.empty() && !IsProtocolSpecified(v))
    return kWebsiteProtocols[kWebsiteProtocolDefaultIndex] + v;
  return v;
}

string ValidateAndFormat_facebook(string const & facebookPage)
{
  if (facebookPage.empty())
    return {};

  if (facebookPage.front() == '@')
  {
    // Validate facebookPage as username or page name.
    if (facebookPage.length() >= 6 && !containsInvalidFBSymbol(facebookPage, 1))
      return facebookPage.substr(1);
    else
      return {}; // Invalid symbol in Facebook username of page name.
  }
  else
  {
    if (facebookPage.length() >= 5 && !containsInvalidFBSymbol(facebookPage))
      return facebookPage;
  }

  // facebookPage is not a valid username it must be an URL.
  if (!ValidateWebsite(facebookPage))
    return {};

  url::Url const url = url::Url::FromString(facebookPage);
  string const domain = strings::MakeLowerCase(url.GetHost());
  // Check Facebook domain name.
  if (strings::EndsWith(domain, "facebook.com") || strings::EndsWith(domain, "fb.com")
      || strings::EndsWith(domain, "fb.me") || strings::EndsWith(domain, "facebook.de")
      || strings::EndsWith(domain, "facebook.fr"))
  {
    auto webPath = url.GetPath();
    // Strip last '/' symbol
    webPath.erase(webPath.find_last_not_of('/') + 1);
    return (webPath.find('/') == string::npos) ? webPath : ("facebook.com/" + webPath);
  }

  return {};
}

string ValidateAndFormat_instagram(string const & instagramPage)
{
  if (instagramPage.empty())
    return {};
  // Check that instagramPage contains valid username.
  // Rules are defined here: https://blog.jstassen.com/2016/03/code-regex-for-instagram-username-and-hashtags/
  if (regex_match(instagramPage, s_instaRegex))
  {
    if (instagramPage.front() == '@')
      return instagramPage.substr(1);
    return instagramPage;
  }
  if (!ValidateWebsite(instagramPage))
    return {};

  url::Url const url = url::Url::FromString(instagramPage);
  string const domain = strings::MakeLowerCase(url.GetHost());
  // Check Instagram domain name.
  if (domain == "instagram.com" || strings::EndsWith(domain, ".instagram.com"))
  {
    auto webPath = url.GetPath();
    // Strip last '/' symbol.
    webPath.erase(webPath.find_last_not_of('/') + 1);
    return (webPath.find('/') == string::npos) ? webPath : ("instagram.com/" + webPath);
  }

  return {};
}

string ValidateAndFormat_twitter(string const & twitterPage)
{
  if (twitterPage.empty())
    return {};
  // Check that twitterPage contains valid username.
  // Rules took here: https://stackoverflow.com/q/11361044
  if (regex_match(twitterPage, s_twitterRegex))
  {
    if (twitterPage.front() == '@')
      return twitterPage.substr(1);
    return twitterPage;
  }
  if (!ValidateWebsite(twitterPage))
    return {};

  url::Url const url = url::Url::FromString(twitterPage);
  string const domain = strings::MakeLowerCase(url.GetHost());
  // Check Twitter domain name.
  if (domain == "twitter.com" || strings::EndsWith(domain, ".twitter.com"))
  {
    auto webPath = url.GetPath();

    // Strip last '/' symbol and first '@' symbol
    webPath.erase(webPath.find_last_not_of('/') + 1);
    webPath.erase(0, webPath.find_first_not_of('@'));

    return (webPath.find('/') == string::npos) ? webPath : ("twitter.com/" + webPath);
  }

  return {};
}

string ValidateAndFormat_vk(string const & vkPage)
{
  if (vkPage.empty())
    return {};
  {
    // Check that vkPage contains valid page name. Rules are defined here: https://vk.com/faq18038
    // The page name must be between 5 and 32 characters.
    // Invalid format could be in cases:
    // - begins with three or more numbers (one or two numbers are allowed).
    // - begins and ends with "_".
    // - contains a period with less than four symbols after it starting with a letter.

    string vkPageClean = vkPage;
    if (vkPageClean.front() == '@')
      vkPageClean = vkPageClean.substr(1);

    if ((vkPageClean.front() == '_' && vkPageClean.back() == '_') || regex_match(vkPageClean, s_badVkRegex))
      return {};
    if (regex_match(vkPageClean, s_goodVkRegex))
      return vkPageClean;
  }
  if (!ValidateWebsite(vkPage))
    return {};

  url::Url const url = url::Url::FromString(vkPage);
  string const domain = strings::MakeLowerCase(url.GetHost());
  // Check VK domain name.
  if (domain == "vk.com" || strings::EndsWith(domain, ".vk.com") ||
      domain == "vkontakte.ru" || strings::EndsWith(domain, ".vkontakte.ru"))
  {
    auto webPath = url.GetPath();
    // Strip last '/' symbol.
    webPath.erase(webPath.find_last_not_of('/') + 1);
    return (webPath.find('/') == string::npos) ? webPath : ("vk.com/" + webPath);
  }

  return {};
}

// Strip '%40' and `@` chars from Line ID start.
string stripAtSymbol(string const & lineId)
{
  if (lineId.empty())
    return lineId;
  if (lineId.front() == '@')
    return lineId.substr(1);
  if (strings::StartsWith(lineId, "%40"))
    return lineId.substr(3);
  return lineId;
}

string ValidateAndFormat_contactLine(string const & linePage)
{
  if (linePage.empty())
    return {};

  {
    // Check that linePage contains valid page name.
    // Rules are defined here: https://help.line.me/line/?contentId=10009904
    // The page name must be between 4 and 20 characters. Should contains alphanumeric characters
    // and symbols '.', '-', and '_'

    string linePageClean = stripAtSymbol(linePage);

    if (regex_match(linePageClean, s_lineRegex))
      return linePageClean;
  }

  if (!ValidateWebsite(linePage))
    return {};

  // URL schema documentation: https://developers.line.biz/en/docs/messaging-api/using-line-url-scheme/
  url::Url const url = url::Url::FromString(linePage);
  string const domain = strings::MakeLowerCase(url.GetHost());
  // Check Line domain name.
  if (domain == "page.line.me")
  {
    // Parse https://page.line.me/{LINE ID}
    string lineId = url.GetPath();
    return stripAtSymbol(lineId);
  }
  else if (domain == "line.me" || strings::EndsWith(domain, ".line.me"))
  {
    auto webPath = url.GetPath();
    if (strings::StartsWith(webPath, "R/ti/p/"))
    {
      // Parse https://line.me/R/ti/p/{LINE ID}
      string lineId = webPath.substr(7, webPath.length());
      return stripAtSymbol(lineId);
    }
    else if (strings::StartsWith(webPath, "ti/p/"))
    {
      // Parse https://line.me/ti/p/{LINE ID}
      string lineId = webPath.substr(5, webPath.length());
      return stripAtSymbol(lineId);
    }
    else if (strings::StartsWith(webPath, "R/home/public/main") || strings::StartsWith(webPath, "R/home/public/profile"))
    {
      // Parse https://line.me/R/home/public/main?id={LINE ID without @}
      // and https://line.me/R/home/public/profile?id={LINE ID without @}
      std::string const * id = url.GetParamValue("id");
      return (id? *id : std::string());
    }
    else
    {
      if (strings::StartsWith(linePage, "http://"))
        return linePage.substr(7);
      if (strings::StartsWith(linePage, "https://"))
        return linePage.substr(8);
    }
  }

  return {};
}

bool ValidateWebsite(string const & site)
{
  if (site.empty())
    return true;

  auto const startPos = GetProtocolNameLength(site);

  if (startPos >= site.size())
    return false;

  // Site should contain at least one dot but not at the begining/end.
  if ('.' == site[startPos] || '.' == site.back())
    return false;

  if (string::npos == site.find("."))
    return false;

  if (string::npos != site.find(".."))
    return false;

  return true;
}

bool ValidateFacebookPage(string const & page)
{
  if (page.empty())
    return true;

  // Check if 'page' contains valid Facebook username or page name.
  // * length >= 5
  // * no forbidden symbols in the string
  // * optional '@' at the start
  if (page.front() == '@')
    return page.length() >= 6 && !containsInvalidFBSymbol(page, 1);
  else if (page.length() >= 5 && !containsInvalidFBSymbol(page))
    return true;

  if (!ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetHost());
  return (strings::StartsWith(domain, "facebook.") || strings::StartsWith(domain, "fb.") ||
          domain.find(".facebook.") != string::npos || domain.find(".fb.") != string::npos);
}

bool ValidateInstagramPage(string const & page)
{
  if (page.empty())
    return true;

  // Rules are defined here: https://blog.jstassen.com/2016/03/code-regex-for-instagram-username-and-hashtags/
  if (regex_match(page, s_instaRegex))
    return true;

  if (!ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetHost());
  return domain == "instagram.com" || strings::EndsWith(domain, ".instagram.com");
}

bool ValidateTwitterPage(string const & page)
{
  if (page.empty())
    return true;

  if (!ValidateWebsite(page))
    return regex_match(page, s_twitterRegex); // Rules are defined here: https://stackoverflow.com/q/11361044

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetHost());
  return domain == "twitter.com" || strings::EndsWith(domain, ".twitter.com");
}

bool ValidateVkPage(string const & page)
{
  if (page.empty())
    return true;

  {
    // Check that page contains valid username. Rules took here: https://vk.com/faq18038
    // The page name must be between 5 and 32 characters.
    // Invalid format could be in cases:
    // - begins with three or more numbers (one or two numbers are allowed).
    // - begins and ends with "_".
    // - contains a period with less than four symbols after it starting with a letter.

    if (page.size() < 5)
      return false;

    string vkLogin = page;
    if (vkLogin.front() == '@')
      vkLogin = vkLogin.substr(1);

    if ((vkLogin.front() == '_' && vkLogin.back() == '_') || regex_match(vkLogin, s_badVkRegex))
      return false;
    if (regex_match(vkLogin, s_goodVkRegex))
      return true;
  }

  if (!ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetHost());
  return domain == "vk.com" || strings::EndsWith(domain, ".vk.com")
         || domain == "vkontakte.ru" || strings::EndsWith(domain, ".vkontakte.ru");
}

bool ValidateLinePage(string const & page)
{
  if (page.empty())
    return true;

  {
    // Check that linePage contains valid page name.
    // Rules are defined here: https://help.line.me/line/?contentId=10009904
    // The page name must be between 4 and 20 characters. Should contains alphanumeric characters
    // and symbols '.', '-', and '_'

    if (regex_match(stripAtSymbol(page), s_lineRegex))
      return true;
  }

  if (!ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetHost());
  // Check Line domain name.
  return (domain == "line.me" || strings::EndsWith(domain, ".line.me"));
}

} // namespace osm
