//
//  MPNativeAdRenderingImageLoader.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@class MPNativeAdRendererImageHandler;

@interface MPNativeAdRenderingImageLoader : NSObject

- (instancetype)initWithImageHandler:(MPNativeAdRendererImageHandler *)imageHandler;

- (void)loadImageForURL:(NSURL *)url intoImageView:(UIImageView *)imageView;

@end
