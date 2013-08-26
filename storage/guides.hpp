#pragma once

#include "../platform/http_request.hpp"

#include "../std/string.hpp"
#include "../std/map.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/noncopyable.hpp"

#include "../../3party/jansson/jansson_handle.hpp"


namespace guides
{

class GuideInfo
{
  my::JsonHandle m_obj;

  string GetString(char const * key) const;
  string GetAdForLang(string const & lang, char const * key) const;

public:
  GuideInfo(json_struct_t * obj = 0) : m_obj(obj) {}

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

  /// @param[in] id MWM file name without extension as a key.
  bool GetGuideInfo(string const & id, GuideInfo & appInfo) const;

  /// @param[in] appID Guide app package id.
  //@{
  bool WasAdvertised(string const & appID);
  void SetWasAdvertised(string const & appID);
  //@}

  bool ValidateAndParseGuidesData(string const & jsonData);

  /// Public visibility for unit tests only!
  string GetDataFileFullPath() const;

private:
  void   OnFinish(downloader::HttpRequest & request);
  string GetGuidesDataUrl() const;
  string GetDataFileName() const;

  /// Map from mwm file name (key) to guide info.
  typedef map<string, GuideInfo> MapT;
  MapT m_file2Info;

  scoped_ptr<downloader::HttpRequest> m_httpRequest;
//@}
};

}
