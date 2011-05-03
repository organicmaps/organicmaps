#include "../std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS)

#include "wifi_info.hpp"

class WiFiInfo::Impl
{
  WiFiInfo::WifiRequestCallbackT m_callback;

public:
  void RequestWiFiBSSIDs(WifiRequestCallbackT callback)
  {
  }
};


WiFiInfo::WiFiInfo() : m_pImpl(new WiFiInfo::Impl)
{
}

WiFiInfo::~WiFiInfo()
{
  delete m_pImpl;
}

void WiFiInfo::RequestWiFiBSSIDs(WifiRequestCallbackT callback)
{
  m_pImpl->RequestWiFiBSSIDs(callback);
}

#endif
