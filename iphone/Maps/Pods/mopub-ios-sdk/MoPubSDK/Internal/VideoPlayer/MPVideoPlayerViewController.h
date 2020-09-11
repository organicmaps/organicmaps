//
//  MPVideoPlayerViewController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPInterstitialViewController.h"
#import "MPVideoPlayer.h"
#import "MPVideoPlayerContainerView.h"

NS_ASSUME_NONNULL_BEGIN

/**
 A compositional delegate for @c MPVideoPlayerViewController.
 */
@protocol MPVideoPlayerViewControllerDelegate
<
MPInterstitialViewControllerAppearanceDelegate,
MPVideoPlayerContainerViewDelegate
>
@end

/**
 @c MPVideoPlayerViewController uses @c MPVideoPlayerContainerView for @c self.view instead of the
 plain @c UIView. All the video playing logics are contained in @c MPVideoPlayerView, and this view
 controller is designed to be a thin container of the video player view. If this view controller
 should have extra functionalities, consider do it in @c MPVideoPlayerView first since the video
 player view is reused as the subview of some other view controller; alternatively, consider to
 expand the compositional @c MPVideoPlayerViewControllerDelegate to keep this view controller thin.
 */
@interface MPVideoPlayerViewController : UIViewController
<
MPInterstitialViewController,
MPVideoPlayer
>

@property (nonatomic, weak) id<MPVideoPlayerViewControllerDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
