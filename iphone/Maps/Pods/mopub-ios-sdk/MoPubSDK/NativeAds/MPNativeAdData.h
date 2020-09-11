//
//  MPNativeAdData.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPNativeAd;

@interface MPNativeAdData : NSObject

@property (nonatomic, copy) NSString *adUnitID;
@property (nonatomic, strong) MPNativeAd *ad;

@end
