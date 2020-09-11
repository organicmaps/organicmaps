//
//  MPNativeAdRequest+MPNativeAdSource.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdRequest.h"

@interface MPNativeAdRequest (MPNativeAdSource)

- (void)startForAdSequence:(NSInteger)adSequence withCompletionHandler:(MPNativeAdRequestHandler)handler;

@end
