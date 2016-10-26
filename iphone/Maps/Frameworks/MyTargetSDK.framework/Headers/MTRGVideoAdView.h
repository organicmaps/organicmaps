//
//  MTRGVideoAdView.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 17.06.15.
//  Copyright (c) 2015 Mail.ru. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGCustomParams.h>

@interface MTRGVideoBannerInfo : NSObject

@property(nonatomic) NSTimeInterval duration;
@property(nonatomic) BOOL allowClose;
@property(nonatomic) NSTimeInterval allowCloseDelay;
@property(nonatomic) CGSize size;
@property(nonatomic, copy) NSString *ctaText;

@end

@class MTRGVideoAdView;

@protocol MTRGVideoAdViewDelegate <NSObject>
- (void)onLoadWithVideoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onNoAdWithReason:(NSString *)reason videoAdView:(MTRGVideoAdView *)videoAdView;

@optional

- (void)onClickWithVideoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onTimeLeftChanged:(NSTimeInterval)timeLeft duration:(NSTimeInterval)duration videoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onBannerStartWithInfo:(MTRGVideoBannerInfo *)info videoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onBannerCompleteWithInfo:(MTRGVideoBannerInfo *)info videoAdView:(MTRGVideoAdView *)videoAdView status:(NSString *)status;

- (void)onCompleteWithSection:(NSString *)section videoAdView:(MTRGVideoAdView *)videoAdView status:(NSString *)status;

- (void)onBannerSuspenseWithInfo:(MTRGVideoBannerInfo *)info videoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onBannerResumptionWithInfo:(MTRGVideoBannerInfo *)info videoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onAirPlayVideoActiveChanged:(BOOL)airPlayVideoActive videoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onShowModalWithVideoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onDismissModalWithVideoAdView:(MTRGVideoAdView *)videoAdView;

- (void)onLeaveApplicationWithVideoAdView:(MTRGVideoAdView *)videoAdView;


@end

@interface MTRGVideoAdView : UIView

@property(nonatomic, strong, readonly) MTRGCustomParams *customParams;
@property(weak, nonatomic) id <MTRGVideoAdViewDelegate> delegate;
@property(nonatomic, weak) UIViewController *viewController;
@property(nonatomic) BOOL trackEnvironmentEnabled;

- (instancetype)initWithSlotId:(NSUInteger)slotId;

- (void)load;

- (void)startPreroll;

- (void)startPostroll;

- (void)startPauseroll;

- (void)startMidroll;

- (void)pause;

- (void)resume;

- (void)stop;

- (void)setFullscreen:(BOOL)isFullscreen;

- (void)setVideoQuality:(NSUInteger)quality;

- (void)setVideoPosition:(NSTimeInterval)time duration:(NSTimeInterval)duration;

- (void)closedByUser;

- (void)skipBanner;

- (void)handleClick;

@end
