#import "MWMPlatform.h"

#include "platform/platform.hpp"

@implementation MWMPlatform

+ (MWMNetworkConnectionType)networkConnectionType
{
  using ct = Platform::EConnectionType;
  switch (GetPlatform().ConnectionStatus())
  {
  case ct::CONNECTION_NONE: return MWMNetworkConnectionTypeNone;
  case ct::CONNECTION_WIFI: return MWMNetworkConnectionTypeWifi;
  case ct::CONNECTION_WWAN: return MWMNetworkConnectionTypeWwan;
  }
}

@end
