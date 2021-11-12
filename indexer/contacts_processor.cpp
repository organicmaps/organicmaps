#include "indexer/contacts_processor.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "coding/url.hpp"

#include "indexer/editable_map_object.hpp"

using namespace std;

namespace osm {

string ValidateAndFormat_facebook(string const & facebookPage)
{
  if (facebookPage.empty())
    return {};
  // Check that facebookPage contains valid username. See rules: https://www.facebook.com/help/105399436216001
  if (strings::EndsWith(facebookPage, ".com") || strings::EndsWith(facebookPage, ".net"))
    return {};
  if (regex_match(facebookPage, regex(R"(^@?[a-zA-Z\d.\-]{5,}$)")))
  {
    if (facebookPage.front() == '@')
      return facebookPage.substr(1);
    else
      return facebookPage;
  }
  if (EditableMapObject::ValidateWebsite(facebookPage))
  {
    string facebookPageUrl = facebookPage;
    // Check if HTTP protocol is present
    if (!strings::StartsWith(facebookPage, "http://") && !strings::StartsWith(facebookPage, "https://"))
      facebookPageUrl = "https://" + facebookPage;

    const url::Url url = url::Url(facebookPageUrl);
    const string & domain = strings::MakeLowerCase(url.GetWebDomain());
    // Check Facebook domain name.
    if (strings::EndsWith(domain, "facebook.com") || strings::EndsWith(domain, "fb.com")
        || strings::EndsWith(domain, "fb.me") || strings::EndsWith(domain, "facebook.de")
        || strings::EndsWith(domain, "facebook.fr"))
    {
      auto webPath = url.GetWebPath();
      // Strip last '/' symbol
      if (webPath.back() == '/')
        return webPath.substr(0, webPath.length()-1);
      else
        return webPath;
    }
  }

  return {};
}

string ValidateAndFormat_instagram(string const & instagramPage)
{
  if (instagramPage.empty())
    return {};
  // Check that instagramPage contains valid username.
  // Rules are defined here: https://blog.jstassen.com/2016/03/code-regex-for-instagram-username-and-hashtags/
  if (regex_match(instagramPage, regex(R"(^@?[A-Za-z0-9_][A-Za-z0-9_.]{0,28}[A-Za-z0-9_]$)")))
  {
    if (instagramPage.front() == '@')
      return instagramPage.substr(1);
    else
      return instagramPage;
  }
  if (EditableMapObject::ValidateWebsite(instagramPage))
  {
    string instagramPageUrl = instagramPage;
    // Check if HTTP protocol is present.
    if (!strings::StartsWith(instagramPage, "http://") && !strings::StartsWith(instagramPage, "https://"))
      instagramPageUrl = "https://" + instagramPage;

    const url::Url url = url::Url(instagramPageUrl);
    const string & domain = strings::MakeLowerCase(url.GetWebDomain());
    // Check Instagram domain name.
    if (domain == "instagram.com" || strings::EndsWith(domain, ".instagram.com"))
    {
      auto webPath = url.GetWebPath();
      // Strip last '/' symbol.
      if (webPath.back() == '/')
        return webPath.substr(0, webPath.length()-1);
      else
        return webPath;
    }
  }

  return {};
}

string ValidateAndFormat_twitter(string const & twitterPage)
{
  if (twitterPage.empty())
    return {};
  // Check that twitterPage contains valid username.
  // Rules took here: https://stackoverflow.com/q/11361044
  if (regex_match(twitterPage, regex(R"(^@?[A-Za-z0-9_]{1,15}$)")))
  {
    if (twitterPage.front() == '@')
      return twitterPage.substr(1);
    else
      return twitterPage;
  }
  if (EditableMapObject::ValidateWebsite(twitterPage))
  {
    string twitterPageUrl = twitterPage;
    // Check if HTTP protocol is present.
    if (!strings::StartsWith(twitterPage, "http://") && !strings::StartsWith(twitterPage, "https://"))
      twitterPageUrl = "https://" + twitterPage;

    const url::Url url = url::Url(twitterPageUrl);
    const string & domain = strings::MakeLowerCase(url.GetWebDomain());
    // Check Twitter domain name.
    if (domain == "twitter.com" || strings::EndsWith(domain, ".twitter.com"))
    {
      auto webPath = url.GetWebPath();
      // Strip last '/' symbol.
      if (webPath.back() == '/')
        webPath = webPath.substr(0, webPath.length()-1);

      // Strip first '@' symbol
      if (webPath.front() == '@')
        webPath = webPath.substr(1);

      return webPath;
    }
  }

  return {};
}

string ValidateAndFormat_vk(string const & vkPage)
{
  if (vkPage.empty())
    return {};
  {
    /* Check that vkPage contains valid page name. Rules are defined here: https://vk.com/faq18038
     * The page name must be between 5 and 32 characters.
       Invalid format could be in cases:
     * - begins with three or more numbers (one or two numbers are allowed).
     * - begins and ends with "_".
     * - contains a period with less than four symbols after it starting with a letter.
     */

    string vkPageClean = vkPage;
    if (vkPageClean.front() == '@')
      vkPageClean = vkPageClean.substr(1);

    if (vkPageClean.front() == '_' && vkPageClean.back() == '_') return {};
    if (regex_match(vkPageClean, regex(R"(^\d\d\d.+$)"))) return {};
    if (regex_match(vkPageClean, regex(R"(^[A-Za-z0-9_.]{5,32}$)"))) return vkPageClean;
  }

  if (EditableMapObject::ValidateWebsite(vkPage))
  {
    string vkPageUrl = vkPage;
    // Check if HTTP protocol is present.
    if (!strings::StartsWith(vkPage, "http://") && !strings::StartsWith(vkPage, "https://"))
      vkPageUrl = "https://" + vkPage;

    const url::Url url = url::Url(vkPageUrl);
    const string & domain = strings::MakeLowerCase(url.GetWebDomain());
    // Check VK domain name.
    if (domain == "vk.com" || strings::EndsWith(domain, ".vk.com") ||
        domain == "vkontakte.ru" || strings::EndsWith(domain, ".vkontakte.ru"))
    {
      auto webPath = url.GetWebPath();
      // Strip last '/' symbol.
      if (webPath.back() == '/')
        return webPath.substr(0, webPath.length()-1);
      else
        return webPath;
    }
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

    if (regex_match(linePageClean, regex(R"(^[a-z0-9-_.]{4,20}$)")))
      return linePageClean;
  }

  if (EditableMapObject::ValidateWebsite(linePage))
  {
    // URL schema documentation: https://developers.line.biz/en/docs/messaging-api/using-line-url-scheme/

    string linePageUrl = linePage;
    // Check if HTTP protocol is present.
    if (!strings::StartsWith(linePage, "http://") && !strings::StartsWith(linePage, "https://"))
      linePageUrl = "https://" + linePage;

    url::Url const url = url::Url(linePageUrl);
    string const & domain = strings::MakeLowerCase(url.GetWebDomain());
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
  }

  return {};
}

}