#import <SystemConfiguration/SCNetworkReachability.h>

#include <sys/socket.h>
#include <netinet/in.h>

enum TActiveConnectionType
{
  ENotConnected,
  EConnectedByWiFi,
  EConnectedBy3G
};

TActiveConnectionType GetActiveConnectionType()
{
	struct sockaddr_in zeroAddress;
	bzero(&zeroAddress, sizeof(zeroAddress));
	zeroAddress.sin_len = sizeof(zeroAddress);
	zeroAddress.sin_family = AF_INET;
  
	// Recover reachability flags
	SCNetworkReachabilityRef defaultRoute = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
	SCNetworkReachabilityFlags flags;
	BOOL didRetrieveFlags = SCNetworkReachabilityGetFlags(defaultRoute, &flags);
	CFRelease(defaultRoute);
	if (!didRetrieveFlags)
		return ENotConnected;
  
	BOOL isReachable = flags & kSCNetworkFlagsReachable;
  BOOL isWifi = !(flags & kSCNetworkReachabilityFlagsIsWWAN);
	BOOL needsConnection = flags & kSCNetworkFlagsConnectionRequired;
	BOOL isConnected = isReachable && !needsConnection;
  if (isConnected)
  {
    if (isWifi)
      return EConnectedByWiFi;
    else
      return EConnectedBy3G;
  }
  return ENotConnected;
}
