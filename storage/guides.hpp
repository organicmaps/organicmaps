#pragma once

#include "../platform/http_request.hpp"

#include "../std/string.hpp"
#include "../std/map.hpp"
#include "../std/set.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/noncopyable.hpp"

#include "../3party/jansson/jansson_handle.hpp"

namespace guides
{

class GuideInfo
{
  my::JsonHandle m_obj;
  string m_name;

  string GetString(char const * key) const;
  string GetAdForLang(string const & lang, char const * key) const;

public:
  GuideInfo(json_struct_t * obj = 0, char const * name = 0);

  string GetName() const;
  string GetURL() const;
  string GetAppID() const;
  string GetAdTitle(string const & lang) const;
  string GetAdMessage(string const & lang) const;

  bool operator == (GuideInfo const & r) const { return (m_obj.get() == r.m_obj.get()); }

  bool IsValid() const;
};

string DebugPrint(GuideInfo const & r);

class GuidesManager : private noncopyable
{
/// @name Guides managment
//@{
public:
  void UpdateGuidesData();
  bool RestoreFromFile();
  typedef map<string, GuideInfo> MapT;
  /// @param[in] guidesInfo filled with correct info about guides on success
  /// @return -1 if failed to parse or json version number otherwise. 0 means version is absent in json.
  static int ParseGuidesData(string const & jsonData, MapT & guidesInfo);
  void RestoreFromParsedData(int version, MapT & guidesInfo);
  static string GetDataFileFullPath();

  /// @param[in] id MWM file name without extension as a key.
  bool GetGuideInfo(string const & countryFile, GuideInfo & appInfo) const;
  bool GetGuideInfoByAppId(string const & id, GuideInfo & appInfo) const;
  void GetGuidesIds(set<string> & s);

  /// @param[in] appID Guide app package id.
  //@{
  bool WasAdvertised(string const & appID);
  void SetWasAdvertised(string const & appID);
  //@}

private:
  void   OnFinish(downloader::HttpRequest & request);
  static string GetGuidesDataUrl();
  static string GetDataFileName();

  /// Map from mwm file name (key) to guide info.
  MapT m_file2Info;

  /// Loaded Guides json version
  int m_version;

  unique_ptr<downloader::HttpRequest> m_httpRequest;
//@}
};

}
