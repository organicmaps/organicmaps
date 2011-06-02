#include "../std/target_os.hpp"
#ifdef OMIM_OS_MAC

#include "wifi_info.hpp"

#include "../base/string_utils.hpp"

#import <Foundation/Foundation.h>
#import <CoreWLAN/CWInterface.h>
#import <CoreWLAN/CWNetwork.h>

static string AppendZeroIfNeeded(string const & macAddrPart)
{
  string res(macAddrPart);
  if (res.size() == 1)
    res.insert(0, "0");
  return res;
}

@interface WiFiInfoMac : NSObject {
@private
  WiFiInfo::WifiRequestCallbackT m_callback;
  vector<WiFiInfo::AccessPoint> m_accessPoints;
}
@end
//- (id)InitWithCallback:(WiFiInfo::WifiRequestCallbackT)callback;
@implementation WiFiInfoMac

- (id)InitWithCallback:(WiFiInfo::WifiRequestCallbackT)callback
{
  self = [super init];
  m_callback = callback;
  [self performSelectorInBackground:@selector(WifiScanThread) withObject:nil];
  return self;
}

- (void)dealloc
{
  [super dealloc];
//  NSLog(@"Mac OS WiFiInfo selfdestructed successfully");
}

/// Executed on main thread
- (void)NotifyGUI
{
  m_callback(m_accessPoints);
  // selfdestruct
  [self release];
}

/// new background thread
- (void)WifiScanThread
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  m_accessPoints.clear();

  // avoid crash on systems without wlan interfaces
  NSArray * ifaces = [CWInterface supportedInterfaces];
  if ([ifaces count])
  {
    CWInterface * iFace = [CWInterface interface];
    NSArray * nets = [iFace scanForNetworksWithParameters:nil error:nil];
    for (NSUInteger i = 0; i < [nets count]; ++i)
    {
      CWNetwork * net = (CWNetwork *)[nets objectAtIndex:i];
      WiFiInfo::AccessPoint apn;
      apn.m_ssid = [net.ssid UTF8String];
      apn.m_signalStrength = strings::to_string([net.rssi intValue]);
      // fix formatting for wifi address
      string const rawBssid = [net.bssid UTF8String];
      if (!rawBssid.empty())
      {
        strings::SimpleTokenizer tokIt(rawBssid, ":");
        apn.m_bssid = AppendZeroIfNeeded(*tokIt);
        do
        {
          ++tokIt;
          apn.m_bssid += "-";
          apn.m_bssid += AppendZeroIfNeeded(*tokIt);
        } while (!tokIt.IsLast());
      }
      m_accessPoints.push_back(apn);
    }
  }
  [pool drain];

  [self performSelectorOnMainThread:@selector(NotifyGUI) withObject:nil waitUntilDone:NO];
}
@end
/////////////////////////////////////////////////////////////////////////////
WiFiInfo::WiFiInfo()
{
}

WiFiInfo::~WiFiInfo()
{
}

void WiFiInfo::RequestWiFiBSSIDs(WifiRequestCallbackT callback)
{
  // it will be self-destroyed when finished
  [[WiFiInfoMac alloc] InitWithCallback:callback];
}

void WiFiInfo::Stop()
{
}

#endif
