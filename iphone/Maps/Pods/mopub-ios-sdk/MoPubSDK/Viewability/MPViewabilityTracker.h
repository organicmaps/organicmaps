//
//  MPViewabilityTracker.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPViewabilityOption.h"
#import "MPVASTTrackingEvent.h"

NS_ASSUME_NONNULL_BEGIN

@class MPWebView;

/**
 * Notification that is fired when at least one viewability vendor is disabled.
 */
extern NSString *const kDisableViewabilityTrackerNotification;

/**
 * Key for accessing the disabled viewability vendors bitmask in the `userInfo` of
 * `kDisableViewabilityTrackerNotification`.
 */
extern NSString *const kDisabledViewabilityTrackers;

/**
 * Provides viewability tracking of an ad view.
 * Tracking will automatically be stopped upon deallocation.
 */
@interface MPViewabilityTracker : NSObject

/**
 * Returns a bit mask indicating which viewability libraries are included and enabled. A value
 * of `MPViewabilityOptionNone` represents that no viewability vendors are enabled or included.
 */
+ (MPViewabilityOption)enabledViewabilityVendors;

/**
 * Disables viewability tracking for the specified vendors for the duration of the session.
 * @remark Viewability cannot be re-enabled for a vendor once it has been disabled.
 * @param vendors Vendors to stop viewability tracking
 */
+ (void)disableViewability:(MPViewabilityOption)vendors;

/**
 * Initializes a viewability tracker that tracks ads rendered by a web view.
 * @param webView The ad web view that should be tracked.
 * @param isVideo Flag indicating that the ad being tracked is a video.
 * @param startTracking Flag indicating that viewability tracking should start immediately.
 * @return A viewability tracker instance.
 */
- (instancetype)initWithWebView:(MPWebView *)webView
                        isVideo:(BOOL)isVideo
       startTrackingImmediately:(BOOL)startTracking NS_DESIGNATED_INITIALIZER;

/**
 Instantiate a viewability adapter for a native video view.
 * @param nativeVideoView A view that is backed by `AVPlayerLayer`, or a superview of it
 * @param startTracking Flag indicating that viewability tracking should start immediately.
 */
- (instancetype)initWithNativeVideoView:(UIView *)nativeVideoView
               startTrackingImmediately:(BOOL)startTracking NS_DESIGNATED_INITIALIZER;

/**
 * Starts viewability tracking. This will do nothing if it is currently tracking.
 */
- (void)startTracking;

/**
 * Stops viewability tracking. This will do nothing if it is not currently tracking.
 */
- (void)stopTracking;

/**
 * Use this method to register views that appear over the web view but that are supposed to be present (e.g.,
 * interstitial close buttons)
 * @param view The view obstructing the ad view
 */
- (void)registerFriendlyObstructionView:(UIView *)view;

/**
 * Track an `MPVideoEvent` event for a native video view.
 * @param event The event to track.
 * @param eventInfo For `MPVideoEventError`, it's a dictionary with "message" as key and an
 *        `NSString` for the message. It's nil for all other events.
 */
- (void)trackNativeVideoEvent:(MPVideoEvent)event eventInfo:(NSDictionary<NSString *, id> * _Nullable)eventInfo;

/**
 * `init` is not available.
 */
- (instancetype)init __attribute__((unavailable("init not available")));

@end

NS_ASSUME_NONNULL_END
