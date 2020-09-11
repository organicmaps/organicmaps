//
//  NSString+MPConsentStatus.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPConsentStatus.h"

@interface NSString (MPConsentStatus)
/**
 Converts the string into a @c MPConsentStatus value. If the conversion fails,
 @c MPConsentStatusUnknown will be returned.
 */
- (MPConsentStatus)consentStatusValue;

/**
 Converts a @c MPConsentStatus value into a string.
 @param status Consent status to convert.
 @returns A valid string or @c nil.
 */
+ (NSString * _Nullable)stringFromConsentStatus:(MPConsentStatus)status;

@end
