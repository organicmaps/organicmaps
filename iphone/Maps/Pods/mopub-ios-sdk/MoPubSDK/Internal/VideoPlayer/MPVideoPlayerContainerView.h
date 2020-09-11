//
//  MPVideoPlayerContainerView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPVASTCompanionAdView.h"
#import "MPVASTIndustryIconView.h"
#import "MPVideoPlayer.h"

NS_ASSUME_NONNULL_BEGIN

@protocol MPVideoPlayerContainerViewDelegate;

@interface MPVideoPlayerContainerView : UIView <MPVideoPlayer>

@property (nonatomic, weak) id<MPVideoPlayerContainerViewDelegate> delegate;

@end

#pragma mark -

@protocol MPVideoPlayerContainerViewDelegate <NSObject>

#pragma mark - video player view

- (UIViewController *)viewControllerForPresentingModalMRAIDExpandedView;

- (void)videoPlayerContainerViewDidLoadVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView;

- (void)videoPlayerContainerViewDidFailToLoadVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView
                                             error:(NSError *)error;

- (void)videoPlayerContainerViewDidStartVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView
                                     duration:(NSTimeInterval)duration;

- (void)videoPlayerContainerViewDidCompleteVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView
                                        duration:(NSTimeInterval)duration;

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
       videoDidReachProgressTime:(NSTimeInterval)videoProgress
                        duration:(NSTimeInterval)duration;

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
                 didTriggerEvent:(MPVideoPlayerEvent)event
                   videoProgress:(NSTimeInterval)videoProgress;

#pragma mark - industry icon view

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
         didShowIndustryIconView:(MPVASTIndustryIconView *)iconView;

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
        didClickIndustryIconView:(MPVASTIndustryIconView *)iconView
       overridingClickThroughURL:(NSURL * _Nullable)url;

#pragma mark - companion ad view

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
          didShowCompanionAdView:(MPVASTCompanionAdView *)companionAdView;

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
         didClickCompanionAdView:(MPVASTCompanionAdView *)companionAdView
       overridingClickThroughURL:(NSURL * _Nullable)url;

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
    didFailToLoadCompanionAdView:(MPVASTCompanionAdView *)companionAdView;

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
   companionAdViewRequestDismiss:(MPVASTCompanionAdView *)companionAdView;

@end


NS_ASSUME_NONNULL_END
