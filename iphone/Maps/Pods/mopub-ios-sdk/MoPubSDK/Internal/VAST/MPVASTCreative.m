//
//  MPVASTCreative.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTCreative.h"
#import "MPVASTLinearAd.h"
#import "MPVASTCompanionAd.h"

@implementation MPVASTCreative

+ (NSDictionary *)modelMap
{
    return @{@"identifier":     @"id",
             @"sequence":       @"sequence",
             @"adID":           @"adID",
             @"linearAd":       @[@"Linear", MPParseClass([MPVASTLinearAd class])],
             @"companionAds":   @[@"CompanionAds.Companion", MPParseArrayOf(MPParseClass([MPVASTCompanionAd class]))]};
}

@end
