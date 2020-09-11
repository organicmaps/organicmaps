//
//  MPDeviceInformation.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import "MPDeviceInformation.h"
#import "NSDictionary+MPAdditions.h"

// ATS Constants
static NSString *const kMoPubAppTransportSecurityDictionaryKey                       = @"NSAppTransportSecurity";
static NSString *const kMoPubAppTransportSecurityAllowsArbitraryLoadsKey             = @"NSAllowsArbitraryLoads";
static NSString *const kMoPubAppTransportSecurityAllowsArbitraryLoadsForMediaKey     = @"NSAllowsArbitraryLoadsForMedia";
static NSString *const kMoPubAppTransportSecurityAllowsArbitraryLoadsInWebContentKey = @"NSAllowsArbitraryLoadsInWebContent";
static NSString *const kMoPubAppTransportSecurityAllowsLocalNetworkingKey            = @"NSAllowsLocalNetworking";
static NSString *const kMoPubAppTransportSecurityRequiresCertificateTransparencyKey  = @"NSRequiresCertificateTransparency";

// Carrier Constants
static NSString *const kMoPubCarrierInfoDictionaryKey    = @"com.mopub.carrierinfo";
static NSString *const kMoPubCarrierNameKey              = @"carrierName";
static NSString *const kMoPubCarrierISOCountryCodeKey    = @"isoCountryCode";
static NSString *const kMoPubCarrierMobileCountryCodeKey = @"mobileCountryCode";
static NSString *const kMoPubCarrierMobileNetworkCodeKey = @"mobileNetworkCode";

@implementation MPDeviceInformation

#pragma mark - Initialization

+ (void)initialize {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        // Asynchronously fetch an updated copy of the device's carrier settings
        // and cache it. This must be performed on the main thread.
        dispatch_async(dispatch_get_main_queue(), ^{
            CTTelephonyNetworkInfo *networkInfo = CTTelephonyNetworkInfo.new;
            [MPDeviceInformation updateCarrierInfoCache:networkInfo.subscriberCellularProvider];
        });
    });
}

#pragma mark - ATS

+ (MPATSSetting)appTransportSecuritySettings {
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

#pragma mark - Connectivity

+ (MPNetworkStatus)currentRadioAccessTechnology {
    static CTTelephonyNetworkInfo *gTelephonyNetworkInfo = nil;

    if (gTelephonyNetworkInfo == nil) {
        gTelephonyNetworkInfo = [[CTTelephonyNetworkInfo alloc] init];
    }
    NSString *accessTechnology = gTelephonyNetworkInfo.currentRadioAccessTechnology;

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

+ (void)updateCarrierInfoCache:(CTCarrier *)carrierInfo {
    // Using `setValue` instead of `setObject` here because `carrierInfo` could be `nil`,
    // and any of its properties could be `nil`.
    NSMutableDictionary *updatedCarrierInfo = [NSMutableDictionary dictionaryWithCapacity:4];
    [updatedCarrierInfo setValue:carrierInfo.carrierName forKey:kMoPubCarrierNameKey];
    [updatedCarrierInfo setValue:carrierInfo.isoCountryCode forKey:kMoPubCarrierISOCountryCodeKey];
    [updatedCarrierInfo setValue:carrierInfo.mobileCountryCode forKey:kMoPubCarrierMobileCountryCodeKey];
    [updatedCarrierInfo setValue:carrierInfo.mobileNetworkCode forKey:kMoPubCarrierMobileNetworkCodeKey];

    [NSUserDefaults.standardUserDefaults setObject:updatedCarrierInfo forKey:kMoPubCarrierInfoDictionaryKey];
}

+ (NSString *)carrierName {
    NSDictionary *carrierInfo = [NSUserDefaults.standardUserDefaults objectForKey:kMoPubCarrierInfoDictionaryKey];
    return [carrierInfo mp_stringForKey:kMoPubCarrierNameKey];
}

+ (NSString *)isoCountryCode {
    NSDictionary *carrierInfo = [NSUserDefaults.standardUserDefaults objectForKey:kMoPubCarrierInfoDictionaryKey];
    return [carrierInfo mp_stringForKey:kMoPubCarrierISOCountryCodeKey];
}

+ (NSString *)mobileCountryCode {
    NSDictionary *carrierInfo = [NSUserDefaults.standardUserDefaults objectForKey:kMoPubCarrierInfoDictionaryKey];
    return [carrierInfo mp_stringForKey:kMoPubCarrierMobileCountryCodeKey];
}

+ (NSString *)mobileNetworkCode {
    NSDictionary *carrierInfo = [NSUserDefaults.standardUserDefaults objectForKey:kMoPubCarrierInfoDictionaryKey];
    return [carrierInfo mp_stringForKey:kMoPubCarrierMobileNetworkCodeKey];
}

@end
