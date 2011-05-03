#include "wifi_info.hpp"

#if defined(OMIM_OS_WINDOWS)
  #include "internal/wifi_impl_windows.hpp"
#elif defined(OMIM_OS_MAC)
  #include "internal/wifi_impl_mac.hpp"
#else
  #error "Please add your OS implementation"
#endif

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
