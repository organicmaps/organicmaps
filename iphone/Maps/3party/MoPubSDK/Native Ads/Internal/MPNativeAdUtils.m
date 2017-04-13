//
//  MPNativeAdUtils.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeAdUtils.h"

NSTimeInterval const kUpdateVisibleCellsInterval = 0.25;

@implementation MPNativeAdUtils

+ (BOOL)addURLString:(NSString *)urlString toURLArray:(NSMutableArray *)urlArray
{
    if (urlString.length == 0) {
        return NO;
    }

    NSURL *url = [NSURL URLWithString:urlString];
    if (url) {
        [urlArray addObject:url];
        return YES;
    } else {
        return NO;
    }
}
@end
