#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "coding/url.hpp"

#include "indexer/editable_map_object.hpp"
#include "indexer/validate_and_format_contacts.hpp"

using namespace std;

namespace osm {

static auto const s_fbRegex = regex(R"(^@?[a-zA-Z\d.\-]{5,}$)");
static auto const s_instaRegex = regex(R"(^@?[A-Za-z0-9_][A-Za-z0-9_.]{0,28}[A-Za-z0-9_]$)");
static auto const s_twitterRegex = regex(R"(^@?[A-Za-z0-9_]{1,15}$)");
static auto const s_badVkRegex = regex(R"(^\d\d\d.+$)");
static auto const s_goodVkRegex = regex(R"(^[A-Za-z0-9_.]{5,32}$)");
static auto const s_lineRegex = regex(R"(^[a-z0-9-_.]{4,20}$)");

string ValidateAndFormat_facebook(string const & facebookPage)
{
  if (facebookPage.empty())
    return {};
  // Check that facebookPage contains valid username. See rules: https://www.facebook.com/help/105399436216001
  if (strings::EndsWith(facebookPage, ".com") || strings::EndsWith(facebookPage, ".net"))
    return {};
  if (regex_match(facebookPage, s_fbRegex))
  {
    if (facebookPage.front() == '@')
      return facebookPage.substr(1);
    return facebookPage;
  }
  if (!EditableMapObject::ValidateWebsite(facebookPage))
    return {};

  url::Url const url = url::Url::FromString(facebookPage);
  string const domain = strings::MakeLowerCase(url.GetWebDomain());
  // Check Facebook domain name.
  if (strings::EndsWith(domain, "facebook.com") || strings::EndsWith(domain, "fb.com")
      || strings::EndsWith(domain, "fb.me") || strings::EndsWith(domain, "facebook.de")
      || strings::EndsWith(domain, "facebook.fr"))
  {
    auto webPath = url.GetWebPath();
    // Strip last '/' symbol
    webPath.erase(webPath.find_last_not_of('/') + 1);
    return webPath;
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
  if (!EditableMapObject::ValidateWebsite(instagramPage))
    return {};

  url::Url const url = url::Url::FromString(instagramPage);
  string const domain = strings::MakeLowerCase(url.GetWebDomain());
  // Check Instagram domain name.
  if (domain == "instagram.com" || strings::EndsWith(domain, ".instagram.com"))
  {
    auto webPath = url.GetWebPath();
    // Strip last '/' symbol.
    webPath.erase(webPath.find_last_not_of('/') + 1);
    return webPath;
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
  if (!EditableMapObject::ValidateWebsite(twitterPage))
    return {};

  url::Url const url = url::Url::FromString(twitterPage);
  string const domain = strings::MakeLowerCase(url.GetWebDomain());
  // Check Twitter domain name.
  if (domain == "twitter.com" || strings::EndsWith(domain, ".twitter.com"))
  {
    auto webPath = url.GetWebPath();

    // Strip last '/' symbol and first '@' symbol
    webPath.erase(webPath.find_last_not_of('/') + 1);
    webPath.erase(0, webPath.find_first_not_of('@'));

    return webPath;
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
  if (!EditableMapObject::ValidateWebsite(vkPage))
    return {};

  url::Url const url = url::Url::FromString(vkPage);
  string const domain = strings::MakeLowerCase(url.GetWebDomain());
  // Check VK domain name.
  if (domain == "vk.com" || strings::EndsWith(domain, ".vk.com") ||
      domain == "vkontakte.ru" || strings::EndsWith(domain, ".vkontakte.ru"))
  {
    auto webPath = url.GetWebPath();
    // Strip last '/' symbol.
    webPath.erase(webPath.find_last_not_of('/') + 1);
    return webPath;
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

  if (!EditableMapObject::ValidateWebsite(linePage))
    return {};

  // URL schema documentation: https://developers.line.biz/en/docs/messaging-api/using-line-url-scheme/
  url::Url const url = url::Url::FromString(linePage);
  string const domain = strings::MakeLowerCase(url.GetWebDomain());
  // Check Line domain name.
  if (domain == "page.line.me")
  {
    // Parse https://page.line.me/{LINE ID}
    string lineId = url.GetWebPath();
    return stripAtSymbol(lineId);
  }
  else if (domain == "line.me" || strings::EndsWith(domain, ".line.me"))
  {
    auto webPath = url.GetWebPath();
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
      string lineId = {};
      url.ForEachParam([&lineId](url::Param const & param) {
        if (param.m_name == "id")
          lineId = param.m_value;
      });

      return lineId;
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

bool ValidateFacebookPage(string const & page)
{
  if (page.empty())
    return true;

  // Rules are defined here: https://www.facebook.com/help/105399436216001
  if (regex_match(page, s_fbRegex))
    return true;

  if (!EditableMapObject::ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetWebDomain());
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

  if (!EditableMapObject::ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetWebDomain());
  return domain == "instagram.com" || strings::EndsWith(domain, ".instagram.com");
}

bool ValidateTwitterPage(string const & page)
{
  if (page.empty())
    return true;

  if (!EditableMapObject::ValidateWebsite(page))
    return regex_match(page, s_twitterRegex); // Rules are defined here: https://stackoverflow.com/q/11361044

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetWebDomain());
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

  if (!EditableMapObject::ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetWebDomain());
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

  if (!EditableMapObject::ValidateWebsite(page))
    return false;

  string const domain = strings::MakeLowerCase(url::Url::FromString(page).GetWebDomain());
  // Check Line domain name.
  return (domain == "line.me" || strings::EndsWith(domain, ".line.me"));
}

} // namespace osm
