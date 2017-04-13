//
//  MPVASTCreative.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
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
