//
//  MPCoreInstanceProvider.m
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPCoreInstanceProvider.h"

#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <CoreTelephony/CTCarrier.h>

#import "MPAdDestinationDisplayAgent.h"
#import "MPAdServerCommunicator.h"
#import "MPAnalyticsTracker.h"
#import "MPGeolocationProvider.h"
#import "MPTimer.h"
#import "MPURLResolver.h"

#define MOPUB_CARRIER_INFO_DEFAULTS_KEY @"com.mopub.carrierinfo"

static NSString *const kMoPubAppTransportSecurityDictionaryKey = @"NSAppTransportSecurity";
static NSString *const kMoPubAppTransportSecurityAllowsArbitraryLoadsKey = @"NSAllowsArbitraryLoads";
static NSString *const kMoPubAppTransportSecurityAllowsArbitraryLoadsForMediaKey = @"NSAllowsArbitraryLoadsForMedia";
static NSString *const kMoPubAppTransportSecurityAllowsArbitraryLoadsInWebContentKey = @"NSAllowsArbitraryLoadsInWebContent";
static NSString *const kMoPubAppTransportSecurityRequiresCertificateTransparencyKey = @"NSRequiresCertificateTransparency";
static NSString *const kMoPubAppTransportSecurityAllowsLocalNetworkingKey = @"NSAllowsLocalNetworking";

typedef enum
{
    MPTwitterDeepLinkNotChecked,
    MPTwitterDeepLinkEnabled,
    MPTwitterDeepLinkDisabled
} MPTwitterDeepLink;

@interface MPCoreInstanceProvider ()

@property (nonatomic, strong) NSMutableDictionary *singletons;
@property (nonatomic, strong) NSMutableDictionary *carrierInfo;
@property (nonatomic, assign) MPTwitterDeepLink twitterDeepLinkStatus;

@end

@implementation MPCoreInstanceProvider

static MPCoreInstanceProvider *sharedProvider = nil;

+ (instancetype)sharedProvider
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sharedProvider = [[self alloc] init];
    });

    return sharedProvider;
}

- (id)init
{
    self = [super init];
    if (self) {
        self.singletons = [NSMutableDictionary dictionary];

        [self initializeCarrierInfo];
    }
    return self;
}


- (id)singletonForClass:(Class)klass provider:(MPSingletonProviderBlock)provider
{
    id singleton = [self.singletons objectForKey:klass];
    if (!singleton) {
        singleton = provider();
        [self.singletons setObject:singleton forKey:(id<NSCopying>)klass];
    }
    return singleton;
}

// This method ensures that "anObject" is retained until the next runloop iteration when
// performNoOp: is executed.
//
// This is useful in situations where, potentially due to a callback chain reaction, an object
// is synchronously deallocated as it's trying to do more work, especially invoking self, after
// the callback.
- (void)keepObjectAliveForCurrentRunLoopIteration:(id)anObject
{
    [self performSelector:@selector(performNoOp:) withObject:anObject afterDelay:0];
}

- (void)performNoOp:(id)anObject
{
    ; // noop
}

#pragma mark - Initializing Carrier Info

- (void)initializeCarrierInfo
{
    self.carrierInfo = [NSMutableDictionary dictionary];

    // check if we have a saved copy
    NSDictionary *saved = [[NSUserDefaults standardUserDefaults] dictionaryForKey:MOPUB_CARRIER_INFO_DEFAULTS_KEY];
    if (saved != nil) {
        [self.carrierInfo addEntriesFromDictionary:saved];
    }

    // now asynchronously load a fresh copy
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        CTTelephonyNetworkInfo *networkInfo = [[CTTelephonyNetworkInfo alloc] init];
        [self performSelectorOnMainThread:@selector(updateCarrierInfoForCTCarrier:) withObject:networkInfo.subscriberCellularProvider waitUntilDone:NO];
    });
}

- (void)updateCarrierInfoForCTCarrier:(CTCarrier *)ctCarrier
{
    // use setValue instead of setObject here because ctCarrier could be nil, and any of its properties could be nil
    [self.carrierInfo setValue:ctCarrier.carrierName forKey:@"carrierName"];
    [self.carrierInfo setValue:ctCarrier.isoCountryCode forKey:@"isoCountryCode"];
    [self.carrierInfo setValue:ctCarrier.mobileCountryCode forKey:@"mobileCountryCode"];
    [self.carrierInfo setValue:ctCarrier.mobileNetworkCode forKey:@"mobileNetworkCode"];

    [[NSUserDefaults standardUserDefaults] setObject:self.carrierInfo forKey:MOPUB_CARRIER_INFO_DEFAULTS_KEY];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

#pragma mark - Utilities

- (MPGeolocationProvider *)sharedMPGeolocationProvider
{
    return [self singletonForClass:[MPGeolocationProvider class] provider:^id{
        return [MPGeolocationProvider sharedProvider];
    }];
}

- (CLLocationManager *)buildCLLocationManager
{
    return [[CLLocationManager alloc] init];
}

- (id<MPAdAlertManagerProtocol>)buildMPAdAlertManagerWithDelegate:(id)delegate
{
    id<MPAdAlertManagerProtocol> adAlertManager = nil;

    Class adAlertManagerClass = NSClassFromString(@"MPAdAlertManager");
    if (adAlertManagerClass != nil) {
        adAlertManager = [[adAlertManagerClass alloc] init];
        [adAlertManager performSelector:@selector(setDelegate:) withObject:delegate];
    }

    return adAlertManager;
}

- (MPAdAlertGestureRecognizer *)buildMPAdAlertGestureRecognizerWithTarget:(id)target action:(SEL)action
{
    MPAdAlertGestureRecognizer *gestureRecognizer = nil;

    Class gestureRecognizerClass = NSClassFromString(@"MPAdAlertGestureRecognizer");
    if (gestureRecognizerClass != nil) {
        gestureRecognizer = [[gestureRecognizerClass alloc] initWithTarget:target action:action];
    }

    return gestureRecognizer;
}

- (MPATSSetting)appTransportSecuritySettings
{
    // Keep track of ATS settings statically, as they'll never change in the lifecycle of the application.
    // This way, the setting value only gets assembled once.
    static BOOL gCheckedAppTransportSettings = NO;
    static MPATSSetting gSetting = MPATSSettingEnabled;

    // If we've already checked ATS settings, just use what we have
    if (gCheckedAppTransportSettings) {
        return gSetting;
    }

    // Otherwise, figure out ATS settings
    // Start with the assumption that ATS is enabled
    gSetting = MPATSSettingEnabled;

    // Grab the ATS dictionary from the Info.plist
    NSDictionary *atsSettingsDictionary = [NSBundle mainBundle].infoDictionary[kMoPubAppTransportSecurityDictionaryKey];

    // Check if ATS is entirely disabled, and if so, add that to the setting value
    if ([atsSettingsDictionary[kMoPubAppTransportSecurityAllowsArbitraryLoadsKey] boolValue]) {
        gSetting |= MPATSSettingAllowsArbitraryLoads;
    }

    // New App Transport Security keys were introduced in iOS 10. Only send settings for these keys if we're running iOS 10 or greater.
    // They may exist in the dictionary if we're running iOS 9, but they won't do anything, so the server shouldn't know about them.
    if (@available(iOS 10, *)) {
        // In iOS 10, NSAllowsArbitraryLoads gets ignored if ANY keys of NSAllowsArbitraryLoadsForMedia,
        // NSAllowsArbitraryLoadsInWebContent, or NSAllowsLocalNetworking are PRESENT (i.e., they can be set to `false`)
        // See: https://developer.apple.com/library/content/documentation/General/Reference/InfoPlistKeyReference/Articles/CocoaKeys.html#//apple_ref/doc/uid/TP40009251-SW34
        // If needed, flip NSAllowsArbitraryLoads back to 0 if any of these keys are present.
        if (atsSettingsDictionary[kMoPubAppTransportSecurityAllowsArbitraryLoadsForMediaKey] != nil
            || atsSettingsDictionary[kMoPubAppTransportSecurityAllowsArbitraryLoadsInWebContentKey] != nil
            || atsSettingsDictionary[kMoPubAppTransportSecurityAllowsLocalNetworkingKey] != nil) {
            gSetting &= (~MPATSSettingAllowsArbitraryLoads);
        }

        if ([atsSettingsDictionary[kMoPubAppTransportSecurityAllowsArbitraryLoadsForMediaKey] boolValue]) {
            gSetting |= MPATSSettingAllowsArbitraryLoadsForMedia;
        }
        if ([atsSettingsDictionary[kMoPubAppTransportSecurityAllowsArbitraryLoadsInWebContentKey] boolValue]) {
            gSetting |= MPATSSettingAllowsArbitraryLoadsInWebContent;
        }
        if ([atsSettingsDictionary[kMoPubAppTransportSecurityRequiresCertificateTransparencyKey] boolValue]) {
            gSetting |= MPATSSettingRequiresCertificateTransparency;
        }
        if ([atsSettingsDictionary[kMoPubAppTransportSecurityAllowsLocalNetworkingKey] boolValue]) {
            gSetting |= MPATSSettingAllowsLocalNetworking;
        }
    }

    gCheckedAppTransportSettings = YES;
    return gSetting;
}

- (NSDictionary *)sharedCarrierInfo
{
    return self.carrierInfo;
}

static CTTelephonyNetworkInfo * gTelephonyNetworkInfo;
- (MPNetworkStatus)currentRadioAccessTechnology {
    if (!gTelephonyNetworkInfo) {
        gTelephonyNetworkInfo = [[CTTelephonyNetworkInfo alloc] init];
    }
    NSString * accessTechnology = gTelephonyNetworkInfo.currentRadioAccessTechnology;

    // The determination of 2G/3G/4G technology is a best-effort.
    if ([accessTechnology isEqualToString:CTRadioAccessTechnologyLTE]) { // Source: https://en.wikipedia.org/wiki/LTE_(telecommunication)
        return MPReachableViaCellularNetwork4G;
    }
    else if ([accessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORev0] || // Source: https://www.phonescoop.com/glossary/term.php?gid=151
             [accessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORevA] || // Source: https://www.phonescoop.com/glossary/term.php?gid=151
             [accessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORevB] || // Source: https://www.phonescoop.com/glossary/term.php?gid=151
             [accessTechnology isEqualToString:CTRadioAccessTechnologyWCDMA] || // Source: https://www.techopedia.com/definition/24282/wideband-code-division-multiple-access-wcdma
             [accessTechnology isEqualToString:CTRadioAccessTechnologyHSDPA] || // Source: https://en.wikipedia.org/wiki/High_Speed_Packet_Access#High_Speed_Downlink_Packet_Access_(HSDPA)
             [accessTechnology isEqualToString:CTRadioAccessTechnologyHSUPA]) { // Source: https://en.wikipedia.org/wiki/High_Speed_Packet_Access#High_Speed_Uplink_Packet_Access_(HSUPA)
        return MPReachableViaCellularNetwork3G;
    }
    else if ([accessTechnology isEqualToString:CTRadioAccessTechnologyCDMA1x] || // Source: In testing, this mode showed up when the phone was in Verizon 1x mode
             [accessTechnology isEqualToString:CTRadioAccessTechnologyGPRS] || // Source: https://en.wikipedia.org/wiki/General_Packet_Radio_Service
             [accessTechnology isEqualToString:CTRadioAccessTechnologyEdge] || // Source: https://en.wikipedia.org/wiki/2G#2.75G_(EDGE)
             [accessTechnology isEqualToString:CTRadioAccessTechnologyeHRPD]) { // Source: https://www.phonescoop.com/glossary/term.php?gid=155
        return MPReachableViaCellularNetwork2G;
    }

    return MPReachableViaCellularNetworkUnknownGeneration;
}

@end
