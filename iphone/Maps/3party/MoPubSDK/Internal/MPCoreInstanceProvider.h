//
//  MPCoreInstanceProvider.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPGlobal.h"
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
@class MPReachability;
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
- (MPAnalyticsTracker *)sharedMPAnalyticsTracker;
- (MPReachability *)sharedMPReachability;
- (MPATSSetting)appTransportSecuritySettings;

// This call may return nil and may not update if the user hot-swaps the device's sim card.
- (NSDictionary *)sharedCarrierInfo;

- (MPTimer *)buildMPTimerWithTimeInterval:(NSTimeInterval)seconds target:(id)target selector:(SEL)selector repeats:(BOOL)repeats;

@end
