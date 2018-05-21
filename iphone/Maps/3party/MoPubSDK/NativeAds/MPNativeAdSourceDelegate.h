//
//  MPNativeAdSourceDelegate.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MPNativeAdSource;

@protocol MPNativeAdSourceDelegate <NSObject>

- (void)adSourceDidFinishRequest:(MPNativeAdSource *)source;

@end
