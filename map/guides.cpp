#include "guides.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"
#include "../../platform/platform.hpp"

#include "../base/logging.hpp"
#include "../std/bind.hpp"

#include "../../3party/jansson/myjansson.hpp"

using namespace guides;

void GuidesManager::RestoreFromFile()
{
  string data;
  ReaderPtr<Reader> r(GetPlatform().GetReader(GUIDES_DATA_FILE));
  r.ReadAsString(data);

  ValidateAndParseGuidesData(data);
}

void GuidesManager::UpdateGuidesData()
{
  downloader::HttpRequest::CallbackT onFinish = bind(&GuidesManager::OnFinish, this, _1);
  m_httpRequest.reset(downloader::HttpRequest::Get(GetGuidesDataUrl(), onFinish));
}

bool GuidesManager::GetGuideInfo(string const & countryId, GuideInfo & appInfo)
{
  map<string, GuideInfo>::iterator const it = m_countryToUrl.find(countryId);

  if (it != m_countryToUrl.end())
  {
    appInfo = it->second;
    return true;
  }
  return false;
}

string GuidesManager::GetGuidesDataUrl()
{
  /// @todo add platform parametr
  return "http://third.server/guides.json";
}

void GuidesManager::OnFinish(downloader::HttpRequest & request)
{
  if (request.Status() == downloader::HttpRequest::ECompleted)
  {
    string const & data = request.Data();
    if(ValidateAndParseGuidesData(data))
      SaveToFile(data);
    else
      LOG(LWARNING, ("Request data is invalid ", request.Data()));
  }
  else
    LOG(LWARNING, ("Request is failed to complete", request.Status()));
}

bool GuidesManager::ValidateAndParseGuidesData(string const & jsonData)
{
  try
  {
    my::Json root(jsonData.c_str());
    void * iter  = json_object_iter(root.get());
    while (iter)
    {
      char const * key = json_object_iter_key(iter);
      json_t * value = json_object_get(root.get(), key);

      GuideInfo info;
      info.m_appId = json_string_value(json_object_get(value, "appId"));
      info.m_appName = json_string_value(json_object_get(value, "name"));
      info.m_appUrl = json_string_value(json_object_get(value, "url"));
      m_countryToUrl[key] = info;

      iter = json_object_iter_next(root.get(), iter);
    }
    return true;
  }
  catch (my::Json::Exception const &)
  {
    return false;
  }
}

void GuidesManager::SaveToFile(string const & jsonData)
{
  string const path = GetPlatform().WritableDir() + GUIDES_DATA_FILE;
  FileWriter writer(path);
  writer.Write(jsonData.data(), jsonData.size());
}
