//
//  MPNativeCustomEvent.m
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeCustomEvent.h"
#import "MPNativeAdError.h"
#import "MPImageDownloadQueue.h"
#import "MPLogging.h"

@interface MPNativeCustomEvent ()

@property (nonatomic, strong) MPImageDownloadQueue *imageDownloadQueue;

@end

@implementation MPNativeCustomEvent

- (id)init
{
    self = [super init];
    if (self) {
        _imageDownloadQueue = [[MPImageDownloadQueue alloc] init];
    }

    return self;
}

- (void)precacheImagesWithURLs:(NSArray *)imageURLs completionBlock:(void (^)(NSArray *errors))completionBlock
{
    if (imageURLs.count > 0) {
        [_imageDownloadQueue addDownloadImageURLs:imageURLs completionBlock:^(NSArray *errors) {
            if (completionBlock) {
                completionBlock(errors);
            }
        }];
    } else {
        if (completionBlock) {
            completionBlock(nil);
        }
    }
}

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info
{
    /*override with custom network behavior*/
}

@end
