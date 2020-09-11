//
//  MPNativeAdRendererImageHandler.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPImageLoader.h"

@protocol MPNativeAdRendererImageHandlerDelegate <MPImageLoaderDelegate>
@end

@interface MPNativeAdRendererImageHandler : MPImageLoader

@property (nonatomic, weak) id<MPNativeAdRendererImageHandlerDelegate> delegate;

@end
