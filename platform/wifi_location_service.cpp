#include "location.hpp"
#include "wifi_info.hpp"
#include "download_manager.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../3party/jansson/myjansson.hpp"

#define MWM_GEOLOCATION_SERVER "http://geolocation.server/"

namespace location
{
  class WiFiLocationService : public LocationService
  {
    WiFiInfo m_wifiInfo;

    void OnHttpPostFinished(HttpFinishedParams const & result)
    {
      // here we should receive json reply with coordinates and accuracy
      try
      {
        my::Json root(result.m_data.c_str());
        if (json_is_object(root))
        {
          json_t * location = json_object_get(root, "location");
          if (json_is_object(location))
          {
            json_t * lat = json_object_get(location, "latitude");
            json_t * lon = json_object_get(location, "longitude");
            json_t * acc = json_object_get(location, "accuracy");
            if (json_is_real(lat) && json_is_real(lon) && json_is_real(acc))
            {
              GpsInfo info;
              info.m_latitude = json_real_value(lat);
              info.m_longitude = json_real_value(lon);
              info.m_horizontalAccuracy = json_real_value(acc);
              // @TODO introduce flags to mark valid values
              info.m_status = ERoughMode;
              NotifyGpsObserver(info);
            }
          }
        }
      }
      catch (my::Json::Exception const & e)
      {
        LOG(LWARNING, ("Invalid reply from location server:", e.what(), result.m_data));
      }
      LOG(LWARNING, ("Invalid reply from location server:", result.m_data));
    }

    void OnWifiScanCompleted(vector<WiFiInfo::AccessPoint> const & accessPoints)
    {
      string jsonRequest("{\"version\":\"1.1.0\"");
      if (accessPoints.size())
        jsonRequest += ",\"wifi_towers\":[";

      for (size_t i = 0; i < accessPoints.size(); ++i)
      {
        jsonRequest += "{\"mac_address\":\"";
        jsonRequest += accessPoints[i].m_bssid;
        jsonRequest += "\",\"ssid\":\"";
        jsonRequest += accessPoints[i].m_ssid;
        jsonRequest += "\",\"signal_strength\":";
        jsonRequest += accessPoints[i].m_signalStrength;
        jsonRequest += "},";
      }
      if (accessPoints.size())
        jsonRequest[jsonRequest.size() - 1] = ']';
      jsonRequest += "}";

      HttpStartParams params;
      params.m_finish = bind(&WiFiLocationService::OnHttpPostFinished, this, _1);
      params.m_contentType = "application/json";
      params.m_postData = jsonRequest;
      params.m_url = MWM_GEOLOCATION_SERVER;
      GetDownloadManager().HttpRequest(params);
    }

  public:
    virtual void StartUpdate(bool)
    {
      m_wifiInfo.RequestWiFiBSSIDs(bind(&WiFiLocationService::OnWifiScanCompleted, this, _1));
    }

    virtual void StopUpdate()
    {
    }
  };
}

location::LocationService * CreateWiFiLocationService()
{
  return new location::WiFiLocationService();
}
