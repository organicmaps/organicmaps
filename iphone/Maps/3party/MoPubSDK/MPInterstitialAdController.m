//
//  MPInterstitialAdController.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPInterstitialAdController.h"

#import "MPLogging.h"
#import "MPInstanceProvider.h"
#import "MPInterstitialAdManager.h"
#import "MPInterstitialAdManagerDelegate.h"

@interface MPInterstitialAdController () <MPInterstitialAdManagerDelegate>

@property (nonatomic, strong) MPInterstitialAdManager *manager;

+ (NSMutableArray *)sharedInterstitials;
- (id)initWithAdUnitId:(NSString *)adUnitId;

@end

@implementation MPInterstitialAdController

@synthesize manager = _manager;
@synthesize delegate = _delegate;
@synthesize adUnitId = _adUnitId;
@synthesize keywords = _keywords;
@synthesize location = _location;
@synthesize testing = _testing;

- (id)initWithAdUnitId:(NSString *)adUnitId
{
    if (self = [super init]) {
        self.manager = [[MPInstanceProvider sharedProvider] buildMPInterstitialAdManagerWithDelegate:self];
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
    NSMutableArray *interstitials = [[self class] sharedInterstitials];

    @synchronized(self) {
        // Find the correct ad controller based on the ad unit ID.
        MPInterstitialAdController *interstitial = nil;
        for (MPInterstitialAdController *currentInterstitial in interstitials) {
            if ([currentInterstitial.adUnitId isEqualToString:adUnitId]) {
                interstitial = currentInterstitial;
                break;
            }
        }

        // Create a new ad controller for this ad unit ID if one doesn't already exist.
        if (!interstitial) {
            interstitial = [[[self class] alloc] initWithAdUnitId:adUnitId];
            [interstitials addObject:interstitial];
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
    [self.manager loadInterstitialWithAdUnitID:self.adUnitId
                                      keywords:self.keywords
                                      location:self.location
                                       testing:self.testing];
}

- (void)showFromViewController:(UIViewController *)controller
{
    if (!controller) {
        MPLogWarn(@"The interstitial could not be shown: "
                  @"a nil view controller was passed to -showFromViewController:.");
        return;
    }

    if (![controller.view.window isKeyWindow]) {
        MPLogWarn(@"Attempted to present an interstitial ad in non-key window. The ad may not render properly");
    }

    [self.manager presentInterstitialFromViewController:controller];
}

#pragma mark - Internal

+ (NSMutableArray *)sharedInterstitials
{
    static NSMutableArray *sharedInterstitials;

    @synchronized(self) {
        if (!sharedInterstitials) {
            sharedInterstitials = [NSMutableArray array];
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
    if ([self.delegate respondsToSelector:@selector(interstitialDidFailToLoadAd:)]) {
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

+ (NSMutableArray *)sharedInterstitialAdControllers
{
    return [[self class] sharedInterstitials];
}

+ (void)removeSharedInterstitialAdController:(MPInterstitialAdController *)controller
{
    [[[self class] sharedInterstitials] removeObject:controller];
}

@end
