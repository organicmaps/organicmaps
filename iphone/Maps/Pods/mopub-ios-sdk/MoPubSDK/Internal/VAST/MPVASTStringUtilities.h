//
//  MPVASTStringUtilities.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface MPVASTStringUtilities : NSObject

+ (double)doubleFromString:(NSString *)string;
+ (BOOL)stringRepresentsNonNegativePercentage:(NSString *)string;
+ (BOOL)stringRepresentsNonNegativeDuration:(NSString *)string;
+ (NSInteger)percentageFromString:(NSString *)string;
+ (NSTimeInterval)timeIntervalFromString:(NSString *)string;
+ (NSString *)stringFromTimeInterval:(NSTimeInterval)timeInterval;

@end
