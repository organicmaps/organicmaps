//
//  MPViewabilityAdapter.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPVideoEvent.h"

NS_ASSUME_NONNULL_BEGIN

/**
 This protocol is for the adapters of various viewability SDK's.
 */
@protocol MPViewabilityAdapter <NSObject>

@property (nonatomic, readonly) BOOL isTracking;

/**
 Start tracking.
 */
- (void)startTracking;

/**
 Stop tracking.
 */
- (void)stopTracking;

/**
 Register a view that is laid out above the tracked ad view, which also belongs to the same UI
 component as the tracked view, such as the Learn More button, Close button, and video progress bar
 of the ad view.
 */
- (void)registerFriendlyObstructionView:(UIView *)view;

@end

#pragma mark - MPViewabilityAdapterForWebView

@protocol MPViewabilityAdapterForWebView <MPViewabilityAdapter>

/**
 Instantiate a viewability adapter for a web view.
 @param webView Either a `WKWebView` or its subclass, or a superview of a `WKWebView` or its subclass.
 @param isVideo Whether the web view is a video player.
 @param startTracking Whether to start tracking right away.
 */
- (instancetype)initWithWebView:(UIView *)webView
                        isVideo:(BOOL)isVideo
       startTrackingImmediately:(BOOL)startTracking;

@end

#pragma mark - MPViewabilityAdapterForNativeVideoView

@protocol MPViewabilityAdapterForNativeVideoView <MPViewabilityAdapter>

/**
 Instantiate a viewability adapter for a native video view.
 * @param nativeVideoView A view that is backed by `AVPlayerLayer`, or a superview of it
 * @param startTracking Flag indicating that viewability tracking should start immediately.
 */
- (instancetype)initWithNativeVideoView:(UIView *)nativeVideoView startTrackingImmediately:(BOOL)startTracking;

/**
 * Track an `MPVideoEvent` event for a native video view.
 * @param event The event to track.
 * @param eventInfo For `MPVideoEventError`, it's a dictionary with "message" as key and an
 *        `NSString` for the message. It's nil for all other events.
 */
- (void)trackNativeVideoEvent:(MPVideoEvent)event eventInfo:(NSDictionary<NSString *, id> * _Nullable)eventInfo;

@end

NS_ASSUME_NONNULL_END
