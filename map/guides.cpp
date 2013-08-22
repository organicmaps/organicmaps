#include "guides.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"
#include "../../platform/platform.hpp"

#include "../base/logging.hpp"
#include "../std/bind.hpp"
#include "../std/iostream.hpp"
#include "../std/target_os.hpp"

#include "../../3party/jansson/myjansson.hpp"



using namespace guides;

bool GuidesManager::RestoreFromFile()
{
  string const fileName = GetDataFileName();
  Platform & p = GetPlatform();

  // At least file in resources must exist
  if (p.IsFileExistsByFullPath(p.ResourcesDir() + fileName))
  {
    string data;
    ReaderPtr<Reader> r(p.GetReader(fileName));
    r.ReadAsString(data);

    return ValidateAndParseGuidesData(data);
  }
  return false;
}

void GuidesManager::UpdateGuidesData()
{
  downloader::HttpRequest::CallbackT onFinish = bind(&GuidesManager::OnFinish, this, _1);
  m_httpRequest.reset(downloader::HttpRequest::Get(GetGuidesDataUrl(), onFinish));
}

bool GuidesManager::GetGuideInfo(string const & countryId, GuideInfo & appInfo) const
{
  map<string, GuideInfo>::const_iterator const it = m_countryToUrl.find(countryId);

  if (it != m_countryToUrl.end())
  {
    appInfo = it->second;
    return true;
  }
  return false;
}

string GuidesManager::GetGuidesDataUrl() const
{
  /// @todo add platform parametr
  return "http://third.server/guides.json";
}

string GuidesManager::GetDataFileName() const
{
  return OMIM_OS_NAME "-" GUIDES_DATA_FILE;
}

void GuidesManager::OnFinish(downloader::HttpRequest & request)
{
  if (request.Status() == downloader::HttpRequest::ECompleted)
  {
    string const & data = request.Data();
    if(ValidateAndParseGuidesData(data))
      SaveToFile();
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

void GuidesManager::SaveToFile() const
{
  string const path = GetPlatform().WritableDir() + GetDataFileName();
  FileWriter writer(path);

  string const openJson  = "{";
  string const closeJson = "}";
  writer.Write(openJson.data(), openJson.size());

  if (!m_countryToUrl.empty())
  {
    bool isFirst = true;
    map<string, GuideInfo>::const_iterator it;
    for (it = m_countryToUrl.begin(); it != m_countryToUrl.end(); ++it)
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
