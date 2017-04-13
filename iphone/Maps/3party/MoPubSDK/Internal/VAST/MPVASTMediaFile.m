//
//  MPVASTMediaFile.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTMediaFile.h"
#import "MPVASTStringUtilities.h"

@implementation MPVASTMediaFile

+ (NSDictionary *)modelMap
{
    return @{@"bitrate":    @[@"bitrate", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"height":     @[@"height", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"width":      @[@"width", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"identifier": @"id",
             @"delivery":   @"delivery",
             @"mimeType":   @"type",
             @"URL":        @[@"text", MPParseURLFromString()]};
}

@end
