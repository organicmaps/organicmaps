//
//  MPVASTStringUtilities.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTStringUtilities.h"

// Expected format is a decimal number from 0-100 followed by the % sign.
static NSString * const kPercentageRegexString = @"^(\\d?\\d(\\.\\d*)?|100(?:\\.0*)?)%$";
static dispatch_once_t percentageRegexOnceToken;
static NSRegularExpression *percentageRegex;

// Expected format is either HH:mm:ss.mmm or simply a floating-point number.
static NSString * const kDurationRegexString = @"^(\\d{2}):([0-5]\\d):([0-5]\\d(?:\\.\\d{1,3})?)|(^[0-9]*\\.?[0-9]+$)";
static dispatch_once_t durationRegexOnceToken;
static NSRegularExpression *durationRegex;

@implementation MPVASTStringUtilities

+ (double)doubleFromString:(NSString *)string
{
    static NSNumberFormatter *formatter = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        formatter = [[NSNumberFormatter alloc] init];
        formatter.numberStyle = NSNumberFormatterDecimalStyle;
    });

    return [[formatter numberFromString:string] doubleValue];
}

+ (BOOL)stringRepresentsNonNegativePercentage:(NSString *)string
{
    dispatch_once(&percentageRegexOnceToken, ^{
        percentageRegex = [NSRegularExpression regularExpressionWithPattern:kPercentageRegexString options:0 error:nil];
    });

    NSArray *matches = [percentageRegex matchesInString:string options:0 range:NSMakeRange(0, [string length])];

    if (![matches count]) {
        return NO;
    }

    NSTextCheckingResult *match = matches[0];
    return (match.range.location != NSNotFound);
}

+ (BOOL)stringRepresentsNonNegativeDuration:(NSString *)string
{
    dispatch_once(&durationRegexOnceToken, ^{
        durationRegex = [NSRegularExpression regularExpressionWithPattern:kDurationRegexString options:0 error:nil];
    });

    NSArray *matches = [durationRegex matchesInString:string options:0 range:NSMakeRange(0, [string length])];

    if (![matches count]) {
        return NO;
    }

    NSTextCheckingResult *match = matches[0];
    return (match.range.location != NSNotFound);
}

+ (NSInteger)percentageFromString:(NSString *)string
{
    dispatch_once(&percentageRegexOnceToken, ^{
        percentageRegex = [NSRegularExpression regularExpressionWithPattern:kPercentageRegexString options:0 error:nil];
    });

    if (![string length]) {
        return 0;
    }

    NSArray *matches = [percentageRegex matchesInString:string options:0 range:NSMakeRange(0, [string length])];
    if ([matches count]) {
        NSTextCheckingResult *match = matches[0];
        if (match.range.location == NSNotFound) {
            return 0;
        }

        return [[string substringWithRange:[match rangeAtIndex:1]] integerValue];
    } else {
        return 0;
    }
}

+ (NSTimeInterval)timeIntervalFromString:(NSString *)string
{
    dispatch_once(&durationRegexOnceToken, ^{
        durationRegex = [NSRegularExpression regularExpressionWithPattern:kDurationRegexString options:0 error:nil];
    });

    if (![string length]) {
        return 0;
    }

    NSArray *matches = [durationRegex matchesInString:string options:0 range:NSMakeRange(0, [string length])];

    if (![matches count]) {
        return 0;
    }

    NSTextCheckingResult *match = matches[0];
    if (match.range.location == NSNotFound) {
        return 0;
    }

    // This is the case where the string is simply a floating-point number.
    if ([match rangeAtIndex:4].location != NSNotFound) {
        return [[string substringWithRange:[match rangeAtIndex:4]] doubleValue];
    }

    // Fail if hours, minutes, or seconds are missing.
    if ([match rangeAtIndex:1].location == NSNotFound ||
        [match rangeAtIndex:2].location == NSNotFound ||
        [match rangeAtIndex:3].location == NSNotFound) {
        return 0;
    }

    NSInteger hours = 0;
    NSInteger minutes = 0;
    double seconds = 0;

    hours = [[string substringWithRange:[match rangeAtIndex:1]] integerValue];
    minutes = [[string substringWithRange:[match rangeAtIndex:2]] integerValue];
    seconds = [[string substringWithRange:[match rangeAtIndex:3]] doubleValue];

    return hours * 60 * 60 + minutes * 60 + seconds;
}

+ (NSString *)stringFromTimeInterval:(NSTimeInterval)timeInterval
{
    if (timeInterval < 0) {
        return @"00:00:00.000";
    }

    NSInteger flooredTimeInterval = (NSInteger)timeInterval;
    NSInteger hours = flooredTimeInterval / 3600;
    NSInteger minutes = (flooredTimeInterval / 60) % 60;
    NSTimeInterval seconds = fmod(timeInterval, 60);
    return [NSString stringWithFormat:@"%02ld:%02ld:%06.3f", (long)hours, (long)minutes, seconds];
}

@end
