//
//  MPNetworkStatus.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

// Values chosen to match the IAB Connection Type Spec, where:
// Unknown: 0
// Ethernet: 1 (skipped because it's not possible on a phone)
// Wifi: 2
// Cellular Unknown: 3
// Cellular 2G: 4
// Cellular 3G: 5
// Cellular 4G: 6
typedef NS_ENUM(NSInteger, MPNetworkStatus) {
    MPNotReachable = 0,
    MPReachableViaWiFi = 2,
    MPReachableViaCellularNetworkUnknownGeneration,
    MPReachableViaCellularNetwork2G,
    MPReachableViaCellularNetwork3G,
    MPReachableViaCellularNetwork4G
};
