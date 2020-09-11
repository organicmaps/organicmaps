//
//  MPImageLoader.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPImageLoader.h"
#import "MPLogging.h"
#import "MPNativeCache.h"
#import "MPImageDownloadQueue.h"

@interface MPImageLoader()

@property (nonatomic) MPImageDownloadQueue *imageDownloadQueue;

@end

@implementation MPImageLoader

- (instancetype)init
{
    if (self = [super init]) {
        _imageDownloadQueue = [[MPImageDownloadQueue alloc] init];
    }
    return self;
}

- (void)loadImageForURL:(NSURL *)imageURL intoImageView:(UIImageView *)imageView
{
    imageView.image = nil;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        __block BOOL isAdViewInHierarchy = NO;

        // Try to prevent unnecessary work if the ad view is not currently in the view hierarchy.
        // Note that this doesn't prevent 100% of the cases as the ad view can still be recycled after this passes.
        // We have an additional 100% accurate check in safeMainQueueSetImage to ensure that we don't overwrite.

        dispatch_sync(dispatch_get_main_queue(), ^{
            isAdViewInHierarchy = [self.delegate nativeAdViewInViewHierarchy];
        });

        if (!isAdViewInHierarchy) {
            MPLogDebug(@"Cell was recycled. Don't bother rendering the image.");
            return;
        }

        NSData *cachedImageData = [[MPNativeCache sharedCache] retrieveDataForKey:imageURL.absoluteString];
        UIImage *image = [UIImage imageWithData:cachedImageData];

        if (image) {
            // By default, the image data isn't decompressed until set on a UIImageView, on the main thread. This
            // can result in poor scrolling performance. To fix this, we force decompression in the background before
            // assignment to a UIImageView.
            UIGraphicsBeginImageContext(CGSizeMake(1, 1));
            [image drawAtPoint:CGPointZero];
            UIGraphicsEndImageContext();

            [self safeMainQueueSetImage:image intoImageView:imageView];
        } else if (imageURL) {
            MPLogDebug(@"Cache miss on %@. Re-downloading...", imageURL);

            __weak __typeof__(self) weakSelf = self;
            [self.imageDownloadQueue addDownloadImageURLs:@[imageURL]
                                          completionBlock:^(NSDictionary <NSURL *, UIImage *> *result, NSArray *errors) {
                                              __strong __typeof__(self) strongSelf = weakSelf;
                                              if (strongSelf) {
                                                  UIImage *image = result[imageURL];
                                                  if (image != nil && errors.count == 0) {
                                                      [strongSelf safeMainQueueSetImage:image intoImageView:imageView];
                                                  }
                                              }
                                          }];
        }
    });
}

- (void)safeMainQueueSetImage:(UIImage *)image intoImageView:(UIImageView *)imageView
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (![self.delegate nativeAdViewInViewHierarchy]) {
            MPLogDebug(@"Cell was recycled. Don't bother setting the image.");
            return;
        }

        if (image) {
            imageView.image = image;
            if ([self.delegate respondsToSelector:@selector(imageLoader:didLoadImageIntoImageView:)]) {
                [self.delegate imageLoader:self didLoadImageIntoImageView:imageView];
            }
        }
    });
}

@end
