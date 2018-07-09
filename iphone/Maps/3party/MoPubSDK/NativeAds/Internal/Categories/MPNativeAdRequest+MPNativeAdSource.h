//
//  MPNativeAdRequest+MPNativeAdSource.h
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeAdRequest.h"

@interface MPNativeAdRequest (MPNativeAdSource)

- (void)startForAdSequence:(NSInteger)adSequence withCompletionHandler:(MPNativeAdRequestHandler)handler;

@end
