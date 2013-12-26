//
//  MPiAdBannerCustomEvent.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPiAdBannerCustomEvent.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import <iAd/iAd.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPADBannerViewManagerObserver <NSObject>

- (void)bannerDidLoad;
- (void)bannerDidFail;
- (void)bannerActionWillBeginAndWillLeaveApplication:(BOOL)willLeave;
- (void)bannerActionDidFinish;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPADBannerViewManager : NSObject <ADBannerViewDelegate>

@property (nonatomic, retain) ADBannerView *bannerView;
@property (nonatomic, retain) NSMutableSet *observers;
@property (nonatomic, assign) BOOL hasTrackedImpression;
@property (nonatomic, assign) BOOL hasTrackedClick;

+ (MPADBannerViewManager *)sharedManager;

- (void)registerObserver:(id<MPADBannerViewManagerObserver>)observer;
- (void)unregisterObserver:(id<MPADBannerViewManagerObserver>)observer;
- (BOOL)shouldTrackImpression;
- (void)didTrackImpression;
- (BOOL)shouldTrackClick;
- (void)didTrackClick;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////


@interface MPInstanceProvider (iAdBanners)

- (ADBannerView *)buildADBannerView;
- (MPADBannerViewManager *)sharedMPAdBannerViewManager;

@end

@implementation MPInstanceProvider (iAdBanners)

- (ADBannerView *)buildADBannerView
{
    return [[[ADBannerView alloc] init] autorelease];
}

- (MPADBannerViewManager *)sharedMPAdBannerViewManager
{
    return [self singletonForClass:[MPADBannerViewManager class]
                          provider:^id{
                              return [[[MPADBannerViewManager alloc] init] autorelease];
                          }];
}

@end


/////////////////////////////////////////////////////////////////////////////////////

@interface MPiAdBannerCustomEvent () <MPADBannerViewManagerObserver>

@property (nonatomic, assign) BOOL onScreen;

@end

@implementation MPiAdBannerCustomEvent

@synthesize onScreen = _onScreen;

- (BOOL)enableAutomaticImpressionAndClickTracking
{
    return NO;
}

- (ADBannerView *)bannerView
{
    return [MPADBannerViewManager sharedManager].bannerView;
}

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info
{
    MPLogInfo(@"Requesting iAd banner");

    [[MPADBannerViewManager sharedManager] registerObserver:self];

    if (self.bannerView.isBannerLoaded) {
        [self bannerDidLoad];
    }
}

- (void)invalidate
{
    self.onScreen = NO;
    [[MPADBannerViewManager sharedManager] unregisterObserver:self];
}

- (void)rotateToOrientation:(UIInterfaceOrientation)orientation
{
    self.bannerView.currentContentSizeIdentifier = UIInterfaceOrientationIsPortrait(orientation) ? ADBannerContentSizeIdentifierPortrait : ADBannerContentSizeIdentifierLandscape;
}

- (void)didDisplayAd
{
    self.onScreen = YES;
    [self trackImpressionIfNecessary];
}

- (void)trackImpressionIfNecessary
{
    if (self.onScreen && [[MPADBannerViewManager sharedManager] shouldTrackImpression]) {
        [self.delegate trackImpression];
        [[MPADBannerViewManager sharedManager] didTrackImpression];
    }
}

- (void)trackClickIfNecessary
{
    if ([[MPADBannerViewManager sharedManager] shouldTrackClick]) {
        [self.delegate trackClick];
        [[MPADBannerViewManager sharedManager] didTrackClick];
    }
}

#pragma mark - <MPADBannerViewManagerObserver>

- (void)bannerDidLoad
{
    [self trackImpressionIfNecessary];
    [self.delegate bannerCustomEvent:self didLoadAd:self.bannerView];
}

- (void)bannerDidFail
{
    [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:nil];
}

- (void)bannerActionWillBeginAndWillLeaveApplication:(BOOL)willLeave
{
    [self trackClickIfNecessary];
    if (willLeave) {
        [self.delegate bannerCustomEventWillLeaveApplication:self];
    } else {
        [self.delegate bannerCustomEventWillBeginAction:self];
    }
}

- (void)bannerActionDidFinish
{
    [self.delegate bannerCustomEventDidFinishAction:self];
}

@end

/////////////////////////////////////////////////////////////////////////////////////

@implementation MPADBannerViewManager

@synthesize bannerView = _bannerView;
@synthesize observers = _observers;
@synthesize hasTrackedImpression = _hasTrackedImpression;
@synthesize hasTrackedClick = _hasTrackedClick;

+ (MPADBannerViewManager *)sharedManager
{
    return [[MPInstanceProvider sharedProvider] sharedMPAdBannerViewManager];
}

- (id)init
{
    self = [super init];
    if (self) {
        self.bannerView = [[MPInstanceProvider sharedProvider] buildADBannerView];
        self.bannerView.delegate = self;
        self.observers = [NSMutableSet set];
    }
    return self;
}

- (void)dealloc
{
    self.bannerView.delegate = nil;
    self.bannerView = nil;
    self.observers = nil;
    [super dealloc];
}

- (void)registerObserver:(id<MPADBannerViewManagerObserver>)observer;
{
    [self.observers addObject:observer];
}

- (void)unregisterObserver:(id<MPADBannerViewManagerObserver>)observer;
{
    [self.observers removeObject:observer];
}

- (BOOL)shouldTrackImpression
{
    return !self.hasTrackedImpression;
}

- (void)didTrackImpression
{
    self.hasTrackedImpression = YES;
}

- (BOOL)shouldTrackClick
{
    return !self.hasTrackedClick;
}

- (void)didTrackClick
{
    self.hasTrackedClick = YES;
}

#pragma mark - <ADBannerViewDelegate>

- (void)bannerViewDidLoadAd:(ADBannerView *)banner
{
    MPLogInfo(@"iAd banner did load");
    self.hasTrackedImpression = NO;
    self.hasTrackedClick = NO;

    for (id<MPADBannerViewManagerObserver> observer in [[self.observers copy] autorelease]) {
        [observer bannerDidLoad];
    }
}

- (void)bannerView:(ADBannerView *)banner didFailToReceiveAdWithError:(NSError *)error
{
    MPLogInfo(@"iAd banner did fail with error %@", error.localizedDescription);
    for (id<MPADBannerViewManagerObserver> observer in [[self.observers copy] autorelease]) {
        [observer bannerDidFail];
    }
}

- (BOOL)bannerViewActionShouldBegin:(ADBannerView *)banner willLeaveApplication:(BOOL)willLeave
{
    MPLogInfo(@"iAd banner action will begin");
    for (id<MPADBannerViewManagerObserver> observer in [[self.observers copy] autorelease]) {
        [observer bannerActionWillBeginAndWillLeaveApplication:willLeave];
    }
    return YES;
}

- (void)bannerViewActionDidFinish:(ADBannerView *)banner
{
    MPLogInfo(@"iAd banner action did finish");
    for (id<MPADBannerViewManagerObserver> observer in [[self.observers copy] autorelease]) {
        [observer bannerActionDidFinish];
    }
}

@end
