#pragma once

#include "std/function.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"

class WiFiInfo
{
public:
  class Impl;

  struct AccessPoint
  {
    string m_bssid;           //!< for example, "33-12-03-5b-44-9a"
    string m_ssid;            //!< name for APN
    string m_signalStrength;  //!< for example, "-54"
  };

  WiFiInfo();
  ~WiFiInfo();

  typedef function<void (vector<WiFiInfo::AccessPoint> const &)> WifiRequestCallbackT;
  void RequestWiFiBSSIDs(WifiRequestCallbackT callback);
  /// Stops any active updates
  void Stop();

private:
    Impl * m_pImpl;
};
