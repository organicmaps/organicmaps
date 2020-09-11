//
//  MPVASTTracking.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTError.h"
#import "MPVideoConfig.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MPVASTTracking <NSObject>

- (instancetype)initWithVideoConfig:(MPVideoConfig *)videoConfig videoURL:(NSURL *)videoURL;

/**
Register the video view for viewability tracking.
* @param videoView A view that is backed by `AVPlayerLayer`, or a superview of it
*/
- (void)registerVideoViewForViewabilityTracking:(UIView *)videoView;

/**
 Stop viewability tracking activities.
 */
- (void)stopViewabilityTracking;

/**
 Call this when a new video event (@c MPVideoEvent) happens.

 @note Some events allows repetition, and some don't.
 @note For @c MPVideoEventProgress, call @c handleVideoProgressEvent:videoDuration: instead.
 */
- (void)handleVideoEvent:(MPVideoEvent)videoEvent videoTimeOffset:(NSTimeInterval)videoTimeOffset;

/**
 Call this when the video play progress is updated.

 @note Do not call this for video complete event. Use @c MPVideoEventComplete instead. Neither
 custom timer nor iOS video player time observer manages the video complete event very well (with a
 high chance of not firing at all due to timing issue), and iOS provides a specific notification for
 the video complete event.
 */
- (void)handleVideoProgressEvent:(NSTimeInterval)videoTimeOffset videoDuration:(NSTimeInterval)videoDuration;

/**
 Send tracking URL's that are not defined in @c MPVideoConfig, such as the click tracking and impression
 tracking URL's defined in a @c MPAdConfiguration that is not available in @c MPVideoConfig.
 */
- (void)uniquelySendURLs:(NSArray<NSURL *> *)urls;

/**
 Call this when a VAST related error happens.
 */
- (void)handleVASTError:(MPVASTError)error videoTimeOffset:(NSTimeInterval)videoTimeOffset;

@end

@interface MPVASTTracking : NSObject <MPVASTTracking>
@end

NS_ASSUME_NONNULL_END
