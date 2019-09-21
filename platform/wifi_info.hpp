#pragma once

#include <functional>
#include <string>
#include <vector>

class WiFiInfo
{
public:
  class Impl;

  struct AccessPoint
  {
    std::string m_bssid;           //!< for example, "33-12-03-5b-44-9a"
    std::string m_ssid;            //!< name for APN
    std::string m_signalStrength;  //!< for example, "-54"
  };

  WiFiInfo();
  ~WiFiInfo();

  using WifiRequestCallbackT = std::function<void(std::vector<WiFiInfo::AccessPoint> const &)>;
  void RequestWiFiBSSIDs(WifiRequestCallbackT callback);
  /// Stops any active updates
  void Stop();

private:
    Impl * m_pImpl;
};
