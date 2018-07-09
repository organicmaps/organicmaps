//
//  MTRGAdView.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 05.03.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGCustomParams.h>

typedef enum : NSUInteger
{
	MTRGAdSize_320x50 = 0,
	MTRGAdSize_300x250 = 1,
	MTRGAdSize_728x90 = 2
} MTRGAdSize;

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

+ (void)setDebugMode:(BOOL)enabled;

+ (BOOL)isDebugMode;

- (instancetype)initWithSlotId:(NSUInteger)slotId;
- (instancetype)initWithSlotId:(NSUInteger)slotId adSize:(MTRGAdSize)adSize;

- (instancetype)initWithSlotId:(NSUInteger)slotId withRefreshAd:(BOOL)refreshAd;
- (instancetype)initWithSlotId:(NSUInteger)slotId withRefreshAd:(BOOL)refreshAd adSize:(MTRGAdSize)adSize;

- (void)load;

- (void)start;

- (void)stop;

@end
