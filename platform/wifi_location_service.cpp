#include "platform/location_service.hpp"
#include "platform/wifi_info.hpp"
#include "platform/http_request.hpp"

#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/ctime.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"


namespace location
{
  class WiFiLocationService : public LocationService
  {
    WiFiInfo m_wifiInfo;

    void OnHttpPostFinished(downloader::HttpRequest & response)
    {
      if (response.GetStatus() == downloader::HttpRequest::Status::Completed)
      {
        // stop requesting wifi updates if reply from server is received
        m_wifiInfo.Stop();
        // here we should receive json reply with coordinates and accuracy
        try
        {
          bool success = false;
          my::Json root(response.GetData().c_str());
          if (json_is_object(root.get()))
          {
            json_t * location = json_object_get(root.get(), "location");
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
                if (IsLatValid(info.m_latitude) && IsLonValid(info.m_latitude))
                {
                  info.m_horizontalAccuracy = json_real_value(acc);
                  // @TODO introduce flags to mark valid values
                  info.m_timestamp = static_cast<double>(time(NULL));
                  info.m_source = location::EGoogle;
                  m_observer.OnLocationUpdated(info);
                  success = true;
                }
              }
            }
          }
          if (!success)
            LOG(LWARNING, ("Invalid reply from location server:", response.GetData()));
        }
        catch (my::Json::Exception const & e)
        {
          LOG(LWARNING, ("Invalid reply from location server:", e.what(), response.GetData()));
        }
      }
      else
        LOG(LWARNING, ("Location server is not available"));
      // free memory
      delete &response;
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

      // memory will be freed in callback
      downloader::HttpRequest::PostJson(MWM_GEOLOCATION_SERVER,
                                        jsonRequest,
                                        bind(&WiFiLocationService::OnHttpPostFinished, this, _1));
    }

  public:
    WiFiLocationService(LocationObserver & observer) : LocationService(observer)
    {
    }

    virtual void Start()
    {
      m_wifiInfo.RequestWiFiBSSIDs(bind(&WiFiLocationService::OnWifiScanCompleted, this, _1));
    }

    virtual void Stop()
    {
      m_wifiInfo.Stop();
    }
  };
}

extern "C" location::LocationService * CreateWiFiLocationService(location::LocationObserver & observer)
{
  return new location::WiFiLocationService(observer);
}
