//
//  MTRGNativeAd.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 10.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <MyTargetSDK/MTRGNativePromoBanner.h>

@class MTRGNativeAd;
@class MTRGCustomParams;

@protocol MTRGNativeAdDelegate <NSObject>

- (void)onLoadWithNativePromoBanner:(MTRGNativePromoBanner *)promoBanner nativeAd:(MTRGNativeAd *)nativeAd;

- (void)onNoAdWithReason:(NSString *)reason nativeAd:(MTRGNativeAd *)nativeAd;

@optional

- (void)onAdClickWithNativeAd:(MTRGNativeAd *)nativeAd;

- (void)onShowModalWithNativeAd:(MTRGNativeAd *)nativeAd;

- (void)onDismissModalWithNativeAd:(MTRGNativeAd *)nativeAd;

- (void)onLeaveApplicationWithNativeAd:(MTRGNativeAd *)nativeAd;

@end

@interface MTRGNativeAd : NSObject

@property(nonatomic, weak) id <MTRGNativeAdDelegate> delegate;
@property(nonatomic, readonly) MTRGNativePromoBanner *banner;
@property(nonatomic, readonly) MTRGCustomParams *customParams;
@property(nonatomic) BOOL autoLoadImages;
@property(nonatomic) BOOL trackEnvironmentEnabled;

- (instancetype)initWithSlotId:(NSUInteger)slotId;

- (void)load;

- (void)registerView:(UIView *)view withController:(UIViewController *)controller;

- (void)unregisterView;

- (void)loadImageToView:(UIImageView *)imageView;

- (void)loadIconToView:(UIImageView *)imageView;
@end