#include "guides.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"
#include "../../coding/internal/file_data.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/settings.hpp"

#include "../base/logging.hpp"
#include "../base/timer.hpp"

#include "../std/bind.hpp"
#include "../std/iostream.hpp"
#include "../std/target_os.hpp"

#include "../../3party/jansson/myjansson.hpp"


namespace
{
char const * GUIDE_UPDATE_TIME_KEY = "GuideUpdateTime";
char const * GUIDE_ADVERTISE_KEY = "GuideAdvertised:";
int const GUIDE_UPDATE_PERIOD = 60 * 60 * 24;
}

namespace guides
{

string GetStringImpl(json_t * j)
{
  char const * s = json_string_value(j);
  return (s ? s : "");
}

string GuideInfo::GetString(char const * key) const
{
  return GetStringImpl(json_object_get(m_obj.get(), key));
}

string GuideInfo::GetAdForLang(string const & lang, char const * key) const
{
  json_t * jKey = json_object_get(m_obj.get(), key);
  ASSERT(jKey, (key));

  string s = GetStringImpl(json_object_get(jKey, lang.c_str()));
  if (s.empty())
  {
    s = GetStringImpl(json_object_get(jKey, "en"));
    ASSERT(!s.empty(), ());
  }
  return s;
}

string GuideInfo::GetURL() const
{
  return GetString("url");
}

string GuideInfo::GetAppID() const
{
  return GetString("appId");
}

string GuideInfo::GetAdTitle(string const & lang) const
{
  return GetAdForLang(lang, "adTitles");
}

string GuideInfo::GetAdMessage(string const & lang) const
{
  return GetAdForLang(lang, "adMessages");
}

bool GuideInfo::IsValid() const
{
  return (!GetURL().empty() && !GetAppID().empty());
}

string DebugPrint(GuideInfo const & r)
{
  ostringstream ss;
  ss << "URL: " << r.GetURL() << "; ID: " << r.GetAppID();
  return ss.str();
}

bool GuidesManager::RestoreFromFile()
{
  try
  {
    ReaderPtr<Reader> r(GetPlatform().GetReader(GetDataFileName()));
    string data;
    r.ReadAsString(data);
    return ValidateAndParseGuidesData(data);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, (ex.Msg()));
    return false;
  }
}

void GuidesManager::UpdateGuidesData()
{
  if (m_httpRequest)
    return;

  double lastUpdateTime;
  bool const flag = Settings::Get(GUIDE_UPDATE_TIME_KEY, lastUpdateTime);

  double const currentTime = my::Timer::LocalTime();
  if (!flag || ((currentTime - lastUpdateTime) >= GUIDE_UPDATE_PERIOD))
  {
    Settings::Set(GUIDE_UPDATE_TIME_KEY, currentTime);

    downloader::HttpRequest::CallbackT onFinish = bind(&GuidesManager::OnFinish, this, _1);
    m_httpRequest.reset(downloader::HttpRequest::Get(GetGuidesDataUrl(), onFinish));
  }
}

bool GuidesManager::GetGuideInfo(string const & id, GuideInfo & appInfo) const
{
  MapT::const_iterator const it = m_file2Info.find(id);
  if (it != m_file2Info.end())
  {
    appInfo = it->second;
    return true;
  }
  return false;
}

string GuidesManager::GetGuidesDataUrl() const
{
#ifdef DEBUG
  return "http://application.server/rest/guides/debug/" + GetDataFileName();
#else
  return "http://application.server/rest/guides/v1/" + GetDataFileName();
#endif
}

string GuidesManager::GetDataFileName() const
{
  return OMIM_OS_NAME "-" GUIDES_DATA_FILE_SUFFIX;
}

string GuidesManager::GetDataFileFullPath() const
{
  return GetPlatform().WritableDir() + GetDataFileName();
}

void GuidesManager::OnFinish(downloader::HttpRequest & request)
{
  if (request.Status() == downloader::HttpRequest::ECompleted)
  {
    string const & data = request.Data();
    if (ValidateAndParseGuidesData(data) && !m_file2Info.empty())
    {
      // Save only if file was parsed successfully and info exists!

      string const path = GetDataFileFullPath();
      try
      {
        FileWriter writer(path);
        writer.Write(data.c_str(), data.size());
      }
      catch (Writer::Exception const & ex)
      {
        LOG(LWARNING, (ex.Msg()));

        // Delete file in case of error
        // (app will get the default one from bundle on start).
        (void)my::DeleteFileX(path);
      }
    }
    else
      LOG(LWARNING, ("Request data is invalid:", request.Data()));
  }
  else
    LOG(LWARNING, ("Request is failed to complete:", request.Status()));

  m_httpRequest.reset();
}

bool GuidesManager::ValidateAndParseGuidesData(string const & jsonData)
{
  typedef my::Json::Exception ParseException;

  try
  {
    my::Json root(jsonData.c_str());
    void * iter = json_object_iter(root.get());

    MapT temp;
    while (iter)
    {
      json_t * entry = json_object_iter_value(iter);
      if (entry)
      {
        GuideInfo info(entry);
        if (info.IsValid())
        {
          json_t * keys = json_object_get(entry, "keys");
          for (size_t i = 0; i < json_array_size(keys); ++i)
          {
            char const * key = json_string_value(json_array_get(keys, i));
            if (key)
              temp[key] = info;
          }
        }
      }

      iter = json_object_iter_next(root.get(), iter);
    }

    m_file2Info.swap(temp);
    return true;
  }
  catch (ParseException const & ex)
  {
    LOG(LWARNING, ("Failed to parse guides data:", ex.Msg()));
    return false;
  }
}

bool GuidesManager::WasAdvertised(string const & appID)
{
  bool flag = false;
  Settings::Get(GUIDE_ADVERTISE_KEY + appID, flag);
  return flag;
}

void GuidesManager::SetWasAdvertised(string const & appID)
{
  Settings::Set(GUIDE_ADVERTISE_KEY + appID, true);
}

}
