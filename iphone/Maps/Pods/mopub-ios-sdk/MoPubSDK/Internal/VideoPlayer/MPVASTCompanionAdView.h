//
//  MPVASTCompanionAdView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTCompanionAd.h"
#import "MPVASTResourceView.h"

NS_ASSUME_NONNULL_BEGIN

@class MPVASTCompanionAdView;

@protocol MPVASTCompanionAdViewDelegate <NSObject>

- (UIViewController *)viewControllerForPresentingModalMRAIDExpandedView;

- (void)companionAdView:(MPVASTCompanionAdView *)companionAdView
        didTriggerEvent:(MPVASTResourceViewEvent)event;

- (void)companionAdView:(MPVASTCompanionAdView *)companionAdView
didTriggerOverridingClickThrough:(NSURL *)url;

- (void)companionAdViewRequestDismiss:(MPVASTCompanionAdView *)companionAdView;

@end

/**
 This view is for showing the companion ad of a VAST video.
 See VAST spec for expected behavior: https://www.iab.com/guidelines/digital-video-ad-serving-template-vast-3-0/
 */
@interface MPVASTCompanionAdView : UIView

@property (nonatomic, readonly) MPVASTCompanionAd *ad;
@property (nonatomic, readonly) BOOL isLoaded;
@property (nonatomic, readonly) BOOL isWebContent;
@property (nonatomic, weak) id<MPVASTCompanionAdViewDelegate> delegate;

- (instancetype)initWithCompanionAd:(MPVASTCompanionAd *)ad;

- (void)loadCompanionAd;

@end

NS_ASSUME_NONNULL_END
