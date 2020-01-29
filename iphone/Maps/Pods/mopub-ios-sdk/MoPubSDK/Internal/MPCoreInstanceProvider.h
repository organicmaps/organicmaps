//
//  MPCoreInstanceProvider.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPGlobal.h"
#import "MPReachability.h"
#import "MPURLResolver.h"

@class MPAdConfiguration;

// Fetching Ads
@class MPAdServerCommunicator;
@protocol MPAdServerCommunicatorDelegate;

// URL Handling
@class MPAdDestinationDisplayAgent;
@protocol MPAdDestinationDisplayAgentDelegate;

// Utilities
@class MPAdAlertManager, MPAdAlertGestureRecognizer;
@class MPAnalyticsTracker;
@class MPTimer;
@class MPGeolocationProvider;
@class CLLocationManager;

typedef id(^MPSingletonProviderBlock)(void);


typedef NS_OPTIONS(NSUInteger, MPATSSetting) {
    MPATSSettingEnabled = 0,
    MPATSSettingAllowsArbitraryLoads = (1 << 0),
    MPATSSettingAllowsArbitraryLoadsForMedia = (1 << 1),
    MPATSSettingAllowsArbitraryLoadsInWebContent = (1 << 2),
    MPATSSettingRequiresCertificateTransparency = (1 << 3),
    MPATSSettingAllowsLocalNetworking = (1 << 4),
};

@interface MPCoreInstanceProvider : NSObject

+ (instancetype)sharedProvider;
- (id)singletonForClass:(Class)klass provider:(MPSingletonProviderBlock)provider;

- (void)keepObjectAliveForCurrentRunLoopIteration:(id)anObject;

#pragma mark - Utilities
- (MPGeolocationProvider *)sharedMPGeolocationProvider;
- (CLLocationManager *)buildCLLocationManager;
- (id<MPAdAlertManagerProtocol>)buildMPAdAlertManagerWithDelegate:(id)delegate;
- (MPAdAlertGestureRecognizer *)buildMPAdAlertGestureRecognizerWithTarget:(id)target action:(SEL)action;
- (MPATSSetting)appTransportSecuritySettings;

// This call may return nil and may not update if the user hot-swaps the device's sim card.
- (NSDictionary *)sharedCarrierInfo;

- (MPNetworkStatus)currentRadioAccessTechnology;

@end
