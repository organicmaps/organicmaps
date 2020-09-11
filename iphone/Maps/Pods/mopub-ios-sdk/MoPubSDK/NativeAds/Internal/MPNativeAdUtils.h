//
//  MPNativeAdUtils.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

extern NSTimeInterval const kUpdateVisibleCellsInterval;

@interface MPNativeAdUtils : NSObject

+ (BOOL)addURLString:(NSString *)urlString toURLArray:(NSMutableArray *)urlArray;

@end
