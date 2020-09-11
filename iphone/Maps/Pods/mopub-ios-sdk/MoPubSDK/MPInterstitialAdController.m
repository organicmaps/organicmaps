//
//  MPInterstitialAdController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPInterstitialAdController.h"
#import "MoPub+Utility.h"
#import "MPAdTargeting.h"
#import "MPGlobal.h"
#import "MPImpressionTrackedNotification.h"
#import "MPInterstitialAdManager.h"
#import "MPInterstitialAdManagerDelegate.h"
#import "MPLogging.h"

@interface MPInterstitialAdController () <MPInterstitialAdManagerDelegate>

@property (nonatomic, strong) MPInterstitialAdManager *manager;

+ (NSMutableDictionary *)sharedInterstitials;
- (id)initWithAdUnitId:(NSString *)adUnitId;

@end

@implementation MPInterstitialAdController

- (id)initWithAdUnitId:(NSString *)adUnitId
{
    if (self = [super init]) {
        self.manager = [[MPInterstitialAdManager alloc] initWithDelegate:self];
        self.adUnitId = adUnitId;
    }
    return self;
}

- (void)dealloc
{
    [self.manager setDelegate:nil];
}

#pragma mark - Public

+ (MPInterstitialAdController *)interstitialAdControllerForAdUnitId:(NSString *)adUnitId
{
    NSMutableDictionary *interstitials = [[self class] sharedInterstitials];

    @synchronized(self) {
        // Find the correct ad controller based on the ad unit ID.
        MPInterstitialAdController * interstitial = interstitials[adUnitId];

        // Create a new ad controller for this ad unit ID if one doesn't already exist.
        if (interstitial == nil) {
            interstitial = [[[self class] alloc] initWithAdUnitId:adUnitId];
            interstitials[adUnitId] = interstitial;
        }

        return interstitial;
    }
}

- (BOOL)ready
{
    return self.manager.ready;
}

- (void)loadAd
{
    MPAdTargeting * targeting = [MPAdTargeting targetingWithCreativeSafeSize:MPApplicationFrame(YES).size];
    targeting.keywords = self.keywords;
    targeting.localExtras = self.localExtras;
    targeting.userDataKeywords = self.userDataKeywords;

    [self.manager loadInterstitialWithAdUnitID:self.adUnitId targeting:targeting];
}

- (void)showFromViewController:(UIViewController *)controller
{
    if (!controller) {
        MPLogInfo(@"The interstitial could not be shown: "
                  @"a nil view controller was passed to -showFromViewController:.");
        return;
    }

    if (![controller.view.window isKeyWindow]) {
        MPLogInfo(@"Attempted to present an interstitial ad in non-key window. The ad may not render properly");
    }

    [self.manager presentInterstitialFromViewController:controller];
}

#pragma mark - Internal

+ (NSMutableDictionary *)sharedInterstitials
{
    static NSMutableDictionary *sharedInterstitials;

    @synchronized(self) {
        if (!sharedInterstitials) {
            sharedInterstitials = [NSMutableDictionary dictionary];
        }
    }

    return sharedInterstitials;
}

#pragma mark - MPInterstitialAdManagerDelegate

- (MPInterstitialAdController *)interstitialAdController
{
    return self;
}

- (id)interstitialDelegate
{
    return self.delegate;
}

- (void)managerDidLoadInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidLoadAd:)]) {
        [self.delegate interstitialDidLoadAd:self];
    }
}

- (void)manager:(MPInterstitialAdManager *)manager
        didFailToLoadInterstitialWithError:(NSError *)error
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidFailToLoadAd:withError:)]) {
        [self.delegate interstitialDidFailToLoadAd:self withError:error];
    } else if ([self.delegate respondsToSelector:@selector(interstitialDidFailToLoadAd:)]) {
        [self.delegate interstitialDidFailToLoadAd:self];
    }
}

- (void)managerWillPresentInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialWillAppear:)]) {
        [self.delegate interstitialWillAppear:self];
    }
}

- (void)managerDidPresentInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidAppear:)]) {
        [self.delegate interstitialDidAppear:self];
    }
}

- (void)managerWillDismissInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialWillDisappear:)]) {
        [self.delegate interstitialWillDisappear:self];
    }
}

- (void)managerDidDismissInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidDisappear:)]) {
        [self.delegate interstitialDidDisappear:self];
    }
}

- (void)managerDidExpireInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidExpire:)]) {
        [self.delegate interstitialDidExpire:self];
    }
}

- (void)managerDidReceiveTapEventFromInterstitial:(MPInterstitialAdManager *)manager
{
    if ([self.delegate respondsToSelector:@selector(interstitialDidReceiveTapEvent:)]) {
        [self.delegate interstitialDidReceiveTapEvent:self];
    }
}

- (void)interstitialAdManager:(MPInterstitialAdManager *)manager didReceiveImpressionEventWithImpressionData:(MPImpressionData *)impressionData {
    [MoPub sendImpressionDelegateAndNotificationFromAd:self
                                              adUnitID:self.adUnitId
                                        impressionData:impressionData];
}

+ (NSMutableArray *)sharedInterstitialAdControllers
{
    return [NSMutableArray arrayWithArray:[[self class] sharedInterstitials].allValues];
}

+ (void)removeSharedInterstitialAdController:(MPInterstitialAdController *)controller
{
    @synchronized(self) {
        NSMutableDictionary * sharedInterstitials = [[self class] sharedInterstitials];
        [sharedInterstitials removeObjectForKey:controller.adUnitId];
    }
}

@end
