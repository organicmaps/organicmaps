//
//  MPVideoPlayerFullScreenVASTAdOverlay.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPVASTIndustryIconView.h"
#import "MPVideoPlayerViewOverlay.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MPVideoPlayerFullScreenVASTAdOverlayDelegate
<
MPVideoPlayerViewOverlayDelegate,
MPVASTIndustryIconViewDelegate
>
@end

/**
 This is an overlay of @c MPVideoPlayerView for full screen VAST ad, which should be added as the
 top-most subview that covers the whole area of the @c MPVideoPlayerView. Timer related activities
 are affected by app life cycle events.
 
 See documentation at https://developers.mopub.com/dsps/ad-formats/video/
 
 Note: Industry icon placing logic is different from the VAST spec per MoPub video format
 documentation: "We will ignore x/y coordinates for the icon and will always place it in the top
 left corner to ensure a consistent user experience."
 */
@interface MPVideoPlayerFullScreenVASTAdOverlay : UIView <MPVideoPlayerViewOverlay>

@property (nonatomic, weak) id<MPVideoPlayerFullScreenVASTAdOverlayDelegate> delegate;

- (void)pauseTimer;

- (void)resumeTimer;

@end

NS_ASSUME_NONNULL_END
