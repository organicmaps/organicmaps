//
//  MPNativeAdRenderingImageLoader.m
//  Copyright (c) 2015 MoPub. All rights reserved.
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
