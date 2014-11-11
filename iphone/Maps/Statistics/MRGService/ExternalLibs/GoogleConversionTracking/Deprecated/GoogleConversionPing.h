//
//  GoogleConversionPing.h
//  Copyright 2011 Google Inc. All rights reserved.
//
//  *** Deprecated ***
//    * Use ACTReporter.h for AdWords conversions and remarketing.
//    * Use DCTReporter.h for DoubleClick for Publishers conversions.
//

#import <Foundation/Foundation.h>

/// Supported conversion ping types.
typedef NS_ENUM(NSInteger, ConversionType) {
  /// AdWords conversion type.
  kGoogleConversion __attribute__((deprecated)),
  /// DoubleClick for Publishers conversion type.
  kDoubleClickConversion __attribute__((deprecated))
};

/// This class provides a way to make easy asynchronous requests to Google for conversion or
/// remarketing pings.
__attribute__((deprecated(" use ACTReporter subclasses.")))
@interface GoogleConversionPing : NSObject

/// Reports a conversion to Google.
+ (void)pingWithConversionId:(NSString *)conversionId
                       label:(NSString *)label
                       value:(NSString *)value
                isRepeatable:(BOOL)isRepeatable
    __attribute__((deprecated(" use ACTConversionReporter.")));

/// Reports a conversion to the ad network according to the type.
+ (void)pingWithConversionId:(NSString *)conversionId
                        type:(ConversionType)type
                       label:(NSString *)label
                       value:(NSString *)value
                isRepeatable:(BOOL)isRepeatable
    __attribute__((deprecated(" use ACTConversionReporter.")));

/// Report a remarketing ping to Google.
+ (void)pingRemarketingWithConversionId:(NSString *)conversionId
                                  label:(NSString *)label
                             screenName:(NSString *)screenName
                       customParameters:(NSDictionary *)customParameters
    __attribute__((deprecated(" use ACTRemarketingReporter.")));

/// Register a click referrer from the Google ad click URL.
+ (BOOL)registerReferrer:(NSURL *)clickURL
    __attribute__((deprecated(" use ACTConversionReporter's +registerReferrer")));

/// Returns the Google Conversion SDK version.
+ (NSString *)sdkVersion __attribute__((deprecated(" use ACTReporter's +SDKVersion.")));

/// UDID has been deprecated and this SDK only uses the IDFA as of version 1.2.0. The |idfaOnly|
/// parameter has no effect.
+ (void)pingWithConversionId:(NSString *)conversionId
                       label:(NSString *)label
                       value:(NSString *)value
                isRepeatable:(BOOL)isRepeatable
                    idfaOnly:(BOOL)idfaOnly
    __attribute__((deprecated(" use ACTConversionReporter.")));

@end
