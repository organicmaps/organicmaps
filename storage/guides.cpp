#include "guides.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/settings.hpp"

#include "../base/logging.hpp"
#include "../base/timer.hpp"

#include "../std/bind.hpp"
#include "../std/iostream.hpp"
#include "../std/target_os.hpp"

#include "../../3party/jansson/myjansson.hpp"


#define GUIDE_UPDATE_TIME_KEY "guideUpdateTime"
#define GUIDE_UPDATE_PERIOD 60 * 60 * 24

using namespace guides;

bool GuidesManager::RestoreFromFile()
{
  try
  {
    ReaderPtr<Reader> r(GetPlatform().GetReader(GetDataFileName()));
    string data;
    r.ReadAsString(data);
    return ValidateAndParseGuidesData(data);
  }
  catch (exception e)
  {
    LOG(LWARNING, ("Exception while restring from file", e.what()));
    return false;
  }
}

void GuidesManager::UpdateGuidesData()
{
  if (m_httpRequest)
    return;
  double lastUpdateTime;
  bool flag = Settings::Get(GUIDE_UPDATE_TIME_KEY, lastUpdateTime);
  double const currentTime = my::Timer::LocalTime();
  if (!flag || ((currentTime - lastUpdateTime) >= GUIDE_UPDATE_PERIOD))
  {
    Settings::Set(GUIDE_UPDATE_TIME_KEY, currentTime);
    downloader::HttpRequest::CallbackT onFinish = bind(&GuidesManager::OnFinish, this, _1);
    m_httpRequest.reset(downloader::HttpRequest::Get(GetGuidesDataUrl(), onFinish));
  }
}

bool GuidesManager::GetGuideInfo(string const & countryId, GuideInfo & appInfo) const
{
  map<string, GuideInfo>::const_iterator const it = m_countryToInfoMapping.find(countryId);

  if (it != m_countryToInfoMapping.end())
  {
    appInfo = it->second;
    return true;
  }
  return false;
}

string GuidesManager::GetGuidesDataUrl() const
{
  return "http://application.server/rest/guides/v1/" + GetDataFileName();
}

string GuidesManager::GetDataFileName() const
{
  return OMIM_OS_NAME "-" GUIDES_DATA_FILE_SUFFIX;
}

void GuidesManager::OnFinish(downloader::HttpRequest & request)
{
  if (request.Status() == downloader::HttpRequest::ECompleted)
  {
    string const & data = request.Data();
    if (ValidateAndParseGuidesData(data))
      SaveToFile();
    else
      LOG(LWARNING, ("Request data is invalid ", request.Data()));
  }
  else
    LOG(LWARNING, ("Request is failed to complete", request.Status()));
  m_httpRequest.reset();
}

bool GuidesManager::ValidateAndParseGuidesData(string const & jsonData)
{
  try
  {
    my::Json root(jsonData.c_str());
    void * iter  = json_object_iter(root.get());

    map<string, GuideInfo> temp;
    while (iter)
    {
      char const * key = json_object_iter_key(iter);
      json_t * value = json_object_get(root.get(), key);

      GuideInfo info;
      info.m_appId = json_string_value(json_object_get(value, "appId"));
      info.m_appName = json_string_value(json_object_get(value, "name"));
      info.m_appUrl = json_string_value(json_object_get(value, "url"));
      temp[key] = info;

      iter = json_object_iter_next(root.get(), iter);
    }
    LOG(LDEBUG, ("Read guides data:", temp.size()));
    m_countryToInfoMapping.swap(temp);
    return true;
  }
  catch (my::Json::Exception const &)
  {
    LOG(LDEBUG, ("Failded to read guides data."));
    return false;
  }
}

void GuidesManager::SaveToFile() const
{
  string const path = GetPlatform().WritableDir() + GetDataFileName();
  FileWriter writer(path);

  string const openJson  = "{";
  string const closeJson = "}";
  writer.Write(openJson.data(), openJson.size());

  if (!m_countryToInfoMapping.empty())
  {
    bool isFirst = true;
    map<string, GuideInfo>::const_iterator it;
    for (it = m_countryToInfoMapping.begin(); it != m_countryToInfoMapping.end(); ++it)
    {
      ostringstream node;
      node << (isFirst ? "" : " ,");
      isFirst = false;

      GuideInfo info = it->second;
      node << "\"" + it->first + "\": {"
           << "\"name\": \""  << info.m_appName << "\", "
           << "\"url\": \""   << info.m_appUrl  << "\", "
           << "\"appId\": \"" << info.m_appId   << "\"}";

      string const nodeStr = node.str();
      writer.Write(nodeStr.data(), nodeStr.size());
    }
  }

  writer.Write(closeJson.data(), closeJson.size());
}
