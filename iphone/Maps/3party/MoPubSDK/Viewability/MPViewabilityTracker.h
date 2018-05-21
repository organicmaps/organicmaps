//
//  MPViewabilityTracker.h
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPViewabilityOption.h"

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
- (instancetype)initWithAdView:(MPWebView *)webView
                       isVideo:(BOOL)isVideo
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
 * `init` is not available.
 */
- (instancetype)init __attribute__((unavailable("init not available")));

@end
