//
//  InterstitialAd.h
//  myTargetSDK 4.6.15
//
//  Created by Anton Bulankin on 04.02.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGCustomParams.h>

@class MTRGInterstitialAd;

@protocol MTRGInterstitialAdDelegate <NSObject>

- (void)onLoadWithInterstitialAd:(MTRGInterstitialAd *)interstitialAd;

- (void)onNoAdWithReason:(NSString *)reason interstitialAd:(MTRGInterstitialAd *)interstitialAd;

@optional

- (void)onClickWithInterstitialAd:(MTRGInterstitialAd *)interstitialAd;

- (void)onCloseWithInterstitialAd:(MTRGInterstitialAd *)interstitialAd;

- (void)onVideoCompleteWithInterstitialAd:(MTRGInterstitialAd *)interstitialAd;

- (void)onDisplayWithInterstitialAd:(MTRGInterstitialAd *)interstitialAd;

- (void)onLeaveApplicationWithInterstitialAd:(MTRGInterstitialAd *)interstitialAd;

@end


@interface MTRGInterstitialAd : NSObject

@property(nonatomic, weak) id <MTRGInterstitialAdDelegate> delegate;
@property(nonatomic, readonly) MTRGCustomParams *customParams;
@property(nonatomic) BOOL trackEnvironmentEnabled;

+ (void)setDebugMode:(BOOL)enabled;

+ (BOOL)isDebugMode;

- (instancetype)initWithSlotId:(NSUInteger)slotId;

- (void)load;

- (void)showWithController:(UIViewController *)controller;

- (void)showModalWithController:(UIViewController *)controller;

- (void)close;

@end
