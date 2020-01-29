/*
 Copyright (C) 2016 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information

 Abstract:
 Basic demonstration of how to use the SystemConfiguration Reachablity APIs.
 */

#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>


// Values chosen to match the IAB Connection Type Spec, where:
// Unknown: 0
// Ethernet: 1 (skipped because it's not possible on a phone)
// Wifi: 2
// Cellular Unknown: 3
// Cellular 2G: 4
// Cellular 3G: 5
// Cellular 4G: 6
typedef enum : NSInteger {
    MPNotReachable = 0,
    MPReachableViaWiFi = 2,
    MPReachableViaCellularNetworkUnknownGeneration,
    MPReachableViaCellularNetwork2G,
    MPReachableViaCellularNetwork3G,
    MPReachableViaCellularNetwork4G
} MPNetworkStatus;

#pragma mark IPv6 Support
//Reachability fully support IPv6.  For full details, see ReadMe.md.


extern NSString *kMPReachabilityChangedNotification;


@interface MPReachability : NSObject

/*!
 * Use to check the reachability of a given host name.
 */
+ (instancetype)reachabilityWithHostName:(NSString *)hostName;

/*!
 * Use to check the reachability of a given IP address.
 */
+ (instancetype)reachabilityWithAddress:(const struct sockaddr *)hostAddress;

/*!
 * Checks whether the default route is available. Should be used by applications that do not connect to a particular host.
 */
+ (instancetype)reachabilityForInternetConnection;


#pragma mark reachabilityForLocalWiFi
//reachabilityForLocalWiFi has been removed from the sample.  See ReadMe.md for more information.
//+ (instancetype)reachabilityForLocalWiFi;

/*!
 * Start listening for reachability notifications on the current run loop.
 */
- (BOOL)startNotifier;
- (void)stopNotifier;

- (MPNetworkStatus)currentReachabilityStatus;

/*!
 * WWAN may be available, but not active until a connection has been established. WiFi may require a connection for VPN on Demand.
 */
- (BOOL)connectionRequired;

@end
