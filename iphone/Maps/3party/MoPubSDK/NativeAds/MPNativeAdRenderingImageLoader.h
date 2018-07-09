//
//  MPNativeAdRenderingImageLoader.h
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MPNativeAdRendererImageHandler;

@interface MPNativeAdRenderingImageLoader : NSObject

- (instancetype)initWithImageHandler:(MPNativeAdRendererImageHandler *)imageHandler;

- (void)loadImageForURL:(NSURL *)url intoImageView:(UIImageView *)imageView;

@end
