//
//  MTRGInstreamAudioAd.h
//  MyTargetSDK
//
//  Created by Andrey Seredkin on 20.12.16.
//  Copyright © 2016 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGInstreamAudioAdPlayer.h>
#import <MyTargetSDK/MTRGInstreamAdCompanionBanner.h>

@class MTRGInstreamAudioAd;
@class MTRGCustomParams;

@interface MTRGInstreamAudioAdBanner : NSObject

@property(nonatomic) NSTimeInterval duration;
@property(nonatomic) BOOL allowSeek;
@property(nonatomic) BOOL allowSkip;
@property(nonatomic) BOOL allowTrackChange;
@property(nonatomic, copy) NSString *adText;
@property(nonatomic) NSArray<MTRGInstreamAdCompanionBanner *> *companionBanners;

@end

@protocol MTRGInstreamAudioAdDelegate <NSObject>

- (void)onLoadWithInstreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onNoAdWithReason:(NSString *)reason instreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

@optional

- (void)onErrorWithReason:(NSString *)reason instreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onBannerStart:(MTRGInstreamAudioAdBanner *)banner instreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onBannerComplete:(MTRGInstreamAudioAdBanner *)banner instreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onBannerTimeLeftChange:(NSTimeInterval)timeLeft duration:(NSTimeInterval)duration instreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onCompleteWithSection:(NSString *)section instreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onShowModalWithInstreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onDismissModalWithInstreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

- (void)onLeaveApplicationWithInstreamAudioAd:(MTRGInstreamAudioAd *)instreamAudioAd;

@end

@interface MTRGInstreamAudioAd : NSObject

@property(nonatomic, weak) id <MTRGInstreamAudioAdDelegate> delegate;
@property(nonatomic, readonly) MTRGCustomParams *customParams;
@property(nonatomic) id <MTRGInstreamAudioAdPlayer> player;
@property(nonatomic) BOOL trackEnvironmentEnabled;
@property(nonatomic) float volume;

+ (void)setDebugMode:(BOOL)enabled;

+ (BOOL)isDebugMode;

- (instancetype)initWithSlotId:(NSUInteger)slotId;

- (void)load;

- (void)pause;

- (void)resume;

- (void)stop;

- (void)skip;

- (void)skipBanner;

- (void)handleCompanionClick:(MTRGInstreamAdCompanionBanner *)companionBanner withController:(UIViewController *)controller;

- (void)handleCompanionShow:(MTRGInstreamAdCompanionBanner *)companionBanner;

- (void)startPreroll;

- (void)startPostroll;

- (void)startPauseroll;

- (void)startMidrollWithPoint:(NSNumber *)point;

- (void)configureMidpointsP:(NSArray<NSNumber *> *)midpointsP forAudioDuration:(NSTimeInterval)audioDuration;

- (void)configureMidpoints:(NSArray<NSNumber *> *)midpoints forAudioDuration:(NSTimeInterval)audioDuration;

- (void)configureMidpointsForAudioDuration:(NSTimeInterval)audioDuration;

- (NSArray<NSNumber *> *)midpoints;

@end
