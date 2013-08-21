#pragma once

#include "../std/string.hpp"
#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"

#include "../platform/http_request.hpp"


namespace guides {

struct GuideInfo
{
  GuideInfo()
    : m_appName(""), m_appUrl(""), m_appId("")
  {}

  GuideInfo(string const & appName, string const & appUrl, string const & appId)
    : m_appName(appName), m_appUrl(appUrl), m_appId(appId)
  {}

  string m_appName;
  string m_appUrl;
  string m_appId;

  bool operator == (GuideInfo const & other) const
  {
    return (m_appName == other.m_appName
            && m_appId == other.m_appId
            && m_appUrl == other.m_appUrl);
  }
};

class GuidesManager
{
/// @name Guides managment
//@{
public:
  void UpdateGuidesData();
  void RestoreFromFile();
  bool GetGuideInfo(string const & countryId, GuideInfo & appInfo);
  bool ValidateAndParseGuidesData(string const & jsonData);

private:
  void   SaveToFile(string const & jsonData);
  void   OnFinish(downloader::HttpRequest & request);
  string GetGuidesDataUrl();

  map<string, GuideInfo> m_countryToUrl;
  scoped_ptr<downloader::HttpRequest> m_httpRequest;
//@}
};

}
