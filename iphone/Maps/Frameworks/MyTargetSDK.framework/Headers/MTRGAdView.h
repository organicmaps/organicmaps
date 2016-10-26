//
//  MTRGAdView.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 05.03.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGCustomParams.h>

@class MTRGAdView;

@protocol MTRGAdViewDelegate <NSObject>

- (void)onLoadWithAdView:(MTRGAdView *)adView;

- (void)onNoAdWithReason:(NSString *)reason adView:(MTRGAdView *)adView;

@optional

- (void)onAdClickWithAdView:(MTRGAdView *)adView;

- (void)onShowModalWithAdView:(MTRGAdView *)adView;

- (void)onDismissModalWithAdView:(MTRGAdView *)adView;

- (void)onLeaveApplicationWithAdView:(MTRGAdView *)adView;

@end

@interface MTRGAdView : UIView

@property(nonatomic, weak) id <MTRGAdViewDelegate> delegate;
@property(nonatomic, readonly) MTRGCustomParams *customParams;
@property(nonatomic, weak) UIViewController *viewController;
@property(nonatomic) BOOL trackEnvironmentEnabled;

- (instancetype)initWithSlotId:(NSUInteger)slotId;

- (instancetype)initWithSlotId:(NSUInteger)slotId withRefreshAd:(BOOL)refreshAd;

- (void)load;

- (void)start;

- (void)stop;

@end
