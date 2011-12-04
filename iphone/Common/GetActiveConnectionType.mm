#import "GetActiveConnectionType.h"
#import <SystemConfiguration/SCNetworkReachability.h>

#include <sys/socket.h>
#include <netinet/in.h>

TActiveConnectionType GetActiveConnectionType()
{
	struct sockaddr_in zeroAddress;
	bzero(&zeroAddress, sizeof(zeroAddress));
	zeroAddress.sin_len = sizeof(zeroAddress);
	zeroAddress.sin_family = AF_INET;

	// Recover reachability flags
	SCNetworkReachabilityRef defaultRoute = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
	SCNetworkReachabilityFlags flags;
	Boolean didRetrieveFlags = SCNetworkReachabilityGetFlags(defaultRoute, &flags);
	CFRelease(defaultRoute);
	if (!didRetrieveFlags)
		return ENotConnected;

	Boolean isReachable = flags & kSCNetworkFlagsReachable;
  Boolean isWifi = !(flags & kSCNetworkReachabilityFlagsIsWWAN);
	Boolean needsConnection = flags & kSCNetworkFlagsConnectionRequired;
	Boolean isConnected = isReachable && !needsConnection;
  if (isConnected)
  {
    if (isWifi)
      return EConnectedByWiFi;
    else
      return EConnectedBy3G;
  }
  return ENotConnected;
}
