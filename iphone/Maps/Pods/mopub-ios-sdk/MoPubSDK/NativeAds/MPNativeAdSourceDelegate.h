//
//  MPNativeAdSourceDelegate.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPNativeAdSource;

@protocol MPNativeAdSourceDelegate <NSObject>

- (void)adSourceDidFinishRequest:(MPNativeAdSource *)source;

@end
