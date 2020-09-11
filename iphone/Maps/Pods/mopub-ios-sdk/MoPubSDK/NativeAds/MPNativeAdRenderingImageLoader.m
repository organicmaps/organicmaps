//
//  MPNativeAdRenderingImageLoader.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdRenderingImageLoader.h"
#import "MPNativeAdRendererImageHandler.h"

@interface MPNativeAdRenderingImageLoader ()

@property (nonatomic) MPNativeAdRendererImageHandler *imageHandler;

@end

@implementation MPNativeAdRenderingImageLoader

- (instancetype)initWithImageHandler:(MPNativeAdRendererImageHandler *)imageHandler
{
    if (self = [super init]) {
        _imageHandler = imageHandler;
    }

    return self;
}

- (void)loadImageForURL:(NSURL *)url intoImageView:(UIImageView *)imageView
{
    [self.imageHandler loadImageForURL:url intoImageView:imageView];
}

@end
