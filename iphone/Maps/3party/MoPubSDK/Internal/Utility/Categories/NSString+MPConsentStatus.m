//
//  NSString+MPConsentStatus.m
//  MoPubSDK
//
//  Copyright © 2018 MoPub. All rights reserved.
//

#import "NSString+MPConsentStatus.h"

// The consent value strings that are given back by the server.
static NSString * const kConsentStatusDoNotTrack         = @"dnt";
static NSString * const kConsentStatusExplicitNo         = @"explicit_no";
static NSString * const kConsentStatusExplicitYes        = @"explicit_yes";
static NSString * const kConsentStatusPotentialWhitelist = @"potential_whitelist";
static NSString * const kConsentStatusUnknown            = @"unknown";

@implementation NSString (MPConsentStatus)

- (MPConsentStatus)consentStatusValue {
    static NSDictionary * stringToEnumTable = nil;
    if (stringToEnumTable == nil) {
        stringToEnumTable = @{ kConsentStatusDoNotTrack: @(MPConsentStatusDoNotTrack),
                               kConsentStatusExplicitNo: @(MPConsentStatusDenied),
                               kConsentStatusExplicitYes: @(MPConsentStatusConsented),
                               kConsentStatusPotentialWhitelist: @(MPConsentStatusPotentialWhitelist),
                               kConsentStatusUnknown: @(MPConsentStatusUnknown)
                               };
    }

    return (MPConsentStatus)[stringToEnumTable[self] integerValue];
}

+ (NSString * _Nullable)stringFromConsentStatus:(MPConsentStatus)status {
    switch (status) {
        case MPConsentStatusDoNotTrack: return kConsentStatusDoNotTrack;
        case MPConsentStatusDenied: return kConsentStatusExplicitNo;
        case MPConsentStatusConsented: return kConsentStatusExplicitYes;
        case MPConsentStatusPotentialWhitelist: return kConsentStatusPotentialWhitelist;
        case MPConsentStatusUnknown: return kConsentStatusUnknown;
        default: return nil;
    }
}

@end
