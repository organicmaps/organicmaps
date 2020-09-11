//
//  MPImageDownloadQueue.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

typedef void (^MPImageDownloadQueueCompletionBlock)(NSDictionary <NSURL *, UIImage *> *result, NSArray *errors);

@interface MPImageDownloadQueue : NSObject

/**
 Return cached image from @c MPNativeCache if available.
 */
- (void)addDownloadImageURLs:(NSArray<NSURL *> *)imageURLs
             completionBlock:(MPImageDownloadQueueCompletionBlock)completionBlock;

- (void)cancelAllDownloads;

@end
