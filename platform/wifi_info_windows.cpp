#include "std/target_os.hpp"

#ifdef OMIM_OS_WINDOWS

#include "platform/wifi_info.hpp"

#include "base/logging.hpp"

#include "std/windows.hpp"
#include "std/cstdio.hpp"

#include <winsock2.h>
#include <iphlpapi.h>

bool GatewaysInfo(vector<WiFiInfo::AccessPoint> & out)
{
  vector<string> ips;

  ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
  PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
  if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
  {
    free(pAdapterInfo);
    pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
  }

  if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
  {
    PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
    while (pAdapter)
    {
      if (strcmp(pAdapter->GatewayList.IpAddress.String, "0.0.0.0"))
        ips.push_back(pAdapter->GatewayList.IpAddress.String);
      pAdapter = pAdapter->Next;
    }
  }
  if (pAdapterInfo)
    free(pAdapterInfo);

  if (!ips.empty())
  {
    DWORD dwSize = 0;
    GetIpNetTable(NULL, &dwSize, 0);
    PMIB_IPNETTABLE pIpNetTable = (MIB_IPNETTABLE *)malloc(dwSize);
    if (GetIpNetTable(pIpNetTable, &dwSize, 0) == NO_ERROR)
    {
      for (DWORD i = 0; i < pIpNetTable->dwNumEntries; ++i)
      {
        for (size_t j = 0; j < ips.size(); ++j)
        {
          if (ips[j] == inet_ntoa(*(struct in_addr *)&pIpNetTable->table[i].dwAddr)
              && pIpNetTable->table[i].dwPhysAddrLen == 6)
          {
            char buff[20] = {};
            sprintf(buff, "%02X-%02X-%02X-%02X-%02X-%02X",
                pIpNetTable->table[i].bPhysAddr[0],
                pIpNetTable->table[i].bPhysAddr[1],
                pIpNetTable->table[i].bPhysAddr[2],
                pIpNetTable->table[i].bPhysAddr[3],
                pIpNetTable->table[i].bPhysAddr[4],
                pIpNetTable->table[i].bPhysAddr[5]);
            WiFiInfo::AccessPoint apn;
            apn.m_bssid = buff;
            apn.m_signalStrength = "-34";
            apn.m_ssid = "dlink";
            out.push_back(apn);
          }
        }
      }
    }
    free(pIpNetTable);
  }
  return !out.empty();
}

#if defined(OMIM_OS_WINDOWS_NATIVE)

#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>

/////////////////////////////////////////////////////////////////
class WiFiInfo::Impl
{
  WiFiInfo::WifiRequestCallbackT m_callback;
  HANDLE m_hClient;
  bool m_isNotificationSupported;

  /// pointers to potential Vista-and-above functions
  typedef DWORD (WINAPI *PWLANGETNETWORKBSSLIST)(HANDLE, const GUID *, const PDOT11_SSID, DOT11_BSS_TYPE,
                  BOOL, PVOID, PWLAN_BSS_LIST *);
  PWLANGETNETWORKBSSLIST pWlanGetNetworkBssList;

  /// used only on XP
  HANDLE m_timer;

public:
  Impl();
  ~Impl();
  HANDLE Handle() const { return m_hClient; }
  void RequestWiFiBSSIDs(WifiRequestCallbackT callback);
  void GetApnsAndNotifyClient();
  void StopNotifications();
};
/////////////////////////////////////////////////////////////////

/// Called on Vista and above
VOID WINAPI OnWlanScanCompleted(PWLAN_NOTIFICATION_DATA data, PVOID context)
{
  switch (data->NotificationCode)
  {
  case wlan_notification_acm_scan_fail:
    LOG(LDEBUG, ("WiFi scan failed with code", *((DWORD *)data->pData)));
    // no break here! try to collect bssids anyway
  case wlan_notification_acm_scan_complete:
    {
      WiFiInfo::Impl * impl = reinterpret_cast<WiFiInfo::Impl *>(context);
      impl->GetApnsAndNotifyClient();
    }
    break;
  default:
    LOG(LDEBUG, ("Unknown WiFi notification", data->NotificationCode));
  }
}

/// Called only on XP by timer
VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN)
{
  WiFiInfo::Impl * impl = reinterpret_cast<WiFiInfo::Impl *>(lpParameter);
  impl->GetApnsAndNotifyClient();
}

WiFiInfo::Impl::Impl() : m_hClient(NULL), m_timer(NULL)
{
  DWORD dwMaxClient = 2;  // 1 for WinXP, 2 for Vista and above
  DWORD dwCurVersion = 0;
  DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &m_hClient);
  if (dwResult != ERROR_SUCCESS)
  {
    LOG(LWARNING, ("Error while opening WLAN handle", dwResult));
  }
  m_isNotificationSupported = (dwCurVersion > 1);
  pWlanGetNetworkBssList = (PWLANGETNETWORKBSSLIST)GetProcAddress(GetModuleHandleA("wlanapi.dll"),
                "WlanGetNetworkBssList");
}

WiFiInfo::Impl::~Impl()
{
  WlanCloseHandle(m_hClient, NULL);
}

void WiFiInfo::Impl::RequestWiFiBSSIDs(WifiRequestCallbackT callback)
{
  m_callback = callback;
  // if it's XP without necessary api... use gateway instead
  if (!pWlanGetNetworkBssList)
  {
    vector<WiFiInfo::AccessPoint> apns;
    GatewaysInfo(apns);
    callback(apns);
    return;
  }

  if (!m_isNotificationSupported)
  { // request timer after 4 seconds, when scanning completes
    CreateTimerQueueTimer(&m_timer, NULL, &WaitOrTimerCallback, this,
                          4100, 0, WT_EXECUTEONLYONCE);
  }
  else
  { // subscribe to notification when scanning is completed
    WlanRegisterNotification(m_hClient,
                             WLAN_NOTIFICATION_SOURCE_ACM,
                             TRUE,
                             &OnWlanScanCompleted,
                             this,
                             NULL,
                             NULL);
  }
  // start scanning
  PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
  DWORD dwResult = WlanEnumInterfaces(m_hClient, NULL, &pIfList);
  if (dwResult == ERROR_SUCCESS)
  {
    for (int ifIndex = 0; ifIndex < static_cast<int>(pIfList->dwNumberOfItems); ++ifIndex)
    {
      PWLAN_INTERFACE_INFO pIfInfo = (PWLAN_INTERFACE_INFO)&pIfList->InterfaceInfo[ifIndex];
      WlanScan(m_hClient, &pIfInfo->InterfaceGuid, NULL, NULL, NULL);
    }
    WlanFreeMemory(pIfList);
  }
}

void WiFiInfo::Impl::GetApnsAndNotifyClient()
{
  vector<WiFiInfo::AccessPoint> apns;
  PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
  DWORD dwResult = WlanEnumInterfaces(m_hClient, NULL, &pIfList);
  if (dwResult == ERROR_SUCCESS)
  {
    for (int ifIndex = 0; ifIndex < static_cast<int>(pIfList->dwNumberOfItems); ++ifIndex)
    {
      PWLAN_INTERFACE_INFO pIfInfo = (PWLAN_INTERFACE_INFO)&pIfList->InterfaceInfo[ifIndex];
      PWLAN_BSS_LIST pWlanBssList = NULL;
      if (pWlanGetNetworkBssList)
      { // on WinXP we don't have this function :(
        dwResult = pWlanGetNetworkBssList(m_hClient,
                                         &pIfInfo->InterfaceGuid,
                                         0,
                                         dot11_BSS_type_any,
                                         FALSE,
                                         NULL,
                                         &pWlanBssList);
        if (dwResult == ERROR_SUCCESS)
        {
          for (int wlanIndex = 0; wlanIndex < static_cast<int>(pWlanBssList->dwNumberOfItems); ++wlanIndex)
          {
            PWLAN_BSS_ENTRY pBssEntry = &pWlanBssList->wlanBssEntries[wlanIndex];
            WiFiInfo::AccessPoint apn;
            apn.m_ssid.assign(&pBssEntry->dot11Ssid.ucSSID[0],
                              &pBssEntry->dot11Ssid.ucSSID[pBssEntry->dot11Ssid.uSSIDLength]);
            char buff[20] = {};
            sprintf(buff, "%02X-%02X-%02X-%02X-%02X-%02X",
                    pBssEntry->dot11Bssid[0], pBssEntry->dot11Bssid[1],
                    pBssEntry->dot11Bssid[2], pBssEntry->dot11Bssid[3],
                    pBssEntry->dot11Bssid[4], pBssEntry->dot11Bssid[5]);
            apn.m_bssid = buff;
            sprintf(buff, "%ld", pBssEntry->lRssi);
            apn.m_signalStrength = buff;
            apns.push_back(apn);
          }
          WlanFreeMemory(pWlanBssList);
        }
      }
    }
    WlanFreeMemory(pIfList);
  }

  m_callback(apns);

  // on WinXP, clean up timer if it was used
  if (m_timer)
  {
    DeleteTimerQueueTimer(NULL, m_timer, NULL);
    m_timer = NULL;
  }
}

void WiFiInfo::Impl::StopNotifications()
{
  WlanRegisterNotification(m_hClient, WLAN_NOTIFICATION_SOURCE_NONE, TRUE, NULL, NULL, NULL, NULL);
}
#else
///////////////////////////////////////////////////////////////////
// MinGW doesn't support wlan api, so simply return mac address of the router

class WiFiInfo::Impl
{
public:
  Impl() {}
  ~Impl() {}
  void RequestWiFiBSSIDs(WifiRequestCallbackT callback)
  {
    vector<WiFiInfo::AccessPoint> apns;
    GatewaysInfo(apns);
    callback(apns);
  }
  void StopNotifications() {}
};

#endif
///////////////////////////////////////////////////////////////////


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

void WiFiInfo::Stop()
{
  m_pImpl->StopNotifications();
}

#endif
