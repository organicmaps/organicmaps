//
//  MTRGNativeAppwallAd.h
//  myTargetSDK 4.5.10
//
//  Created by Anton Bulankin on 13.01.15.
//  Copyright (c) 2015 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <MyTargetSDK/MTRGCustomParams.h>
#import <MyTargetSDK/MTRGNativeAppwallBanner.h>
#import <MyTargetSDK/MTRGAppwallAdView.h>


@class MTRGNativeAppwallAd;

@protocol MTRGNativeAppwallAdDelegate <NSObject>

- (void)onLoadWithAppwallBanners:(NSArray *)appwallBanners appwallAd:(MTRGNativeAppwallAd *)appwallAd;

- (void)onNoAdWithReason:(NSString *)reason appwallAd:(MTRGNativeAppwallAd *)appwallAd;

@optional

- (void)onAdClickWithNativeAppwallAd:(MTRGNativeAppwallAd *)appwallAd appwallBanner:(MTRGNativeAppwallBanner *)appwallBanner;

@end


@interface MTRGNativeAppwallAd : NSObject

@property(nonatomic, weak) id <MTRGNativeAppwallAdDelegate> delegate;
@property(nonatomic, copy) NSString *appWallTitle;
@property(nonatomic, copy) NSString *closeButtonTitle;
@property(nonatomic) NSUInteger cachePeriodInSec;
@property(nonatomic, readonly) MTRGCustomParams *customParams;
@property(nonatomic, readonly) NSArray *banners;
@property(nonatomic) BOOL trackEnvironmentEnabled;

- (instancetype)initWithSlotId:(NSUInteger)slotId;

- (void)load;

- (void)showWithController:(UIViewController *)controller onComplete:(void (^)())onComplete
                   onError:(void (^)(NSError *error))onError;

- (void)registerAppWallAdView:(MTRGAppwallAdView *)appWallAdView withController:(UIViewController *)controller;

- (void)close;

- (BOOL)hasNotifications;

- (void)handleShow:(MTRGNativeAppwallBanner *)appWallBanner;

- (void)handleClick:(MTRGNativeAppwallBanner *)appWallBanner withController:(UIViewController *)controller;

@end
