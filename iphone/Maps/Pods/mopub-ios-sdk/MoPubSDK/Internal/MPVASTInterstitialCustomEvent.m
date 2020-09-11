//
//  MPVASTInterstitialCustomEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdConfiguration.h"
#import "MPAdDestinationDisplayAgent.h"
#import "MPDiskLRUCache.h"
#import "MPHTTPNetworkSession.h"
#import "MPLogging.h"
#import "MPURLRequest.h"
#import "MPVASTInterstitialCustomEvent.h"
#import "MPVASTMacroProcessor.h"
#import "MPVASTManager.h"
#import "MPVASTTracking.h"
#import "MPVideoConfig.h"
#import "MPVideoPlayerViewController.h"

static NSString * const kMPVASTInterstitialCustomEventErrorDomain = @"com.mopub.MPVASTInterstitialCustomEvent";

@interface MPVASTInterstitialCustomEvent ()

@property (nonatomic, strong) id<MPAdDestinationDisplayAgent> adDestinationDisplayAgent;
@property (nonatomic, strong) id<MPVASTTracking> vastTracking;
@property (nonatomic, strong) id<MPMediaFileCache> mediaFileCache;
@property (nonatomic, strong) MPVASTMediaFile *remoteMediaFileToPlay;
@property (nonatomic, strong) MPVideoConfig *videoConfig;
@property (nonatomic, strong) MPVideoPlayerViewController *playerViewController; // the interstitial
@property (nonatomic, assign) BOOL hasAdAvailable; // Rewarded experience related (MPRewardedVideoCustomEvent)

@end

@interface MPVASTInterstitialCustomEvent (MPAdDestinationDisplayAgentDelegate) <MPAdDestinationDisplayAgentDelegate>
@end

@interface MPVASTInterstitialCustomEvent (MPVideoPlayerViewControllerDelegate) <MPVideoPlayerViewControllerDelegate>
@end

@implementation MPVASTInterstitialCustomEvent

@synthesize delegate;

- (instancetype)init {
    self = [super init];
    if (self) {
        _adDestinationDisplayAgent = [MPAdDestinationDisplayAgent agentWithDelegate:self];
        _mediaFileCache = [MPDiskLRUCache sharedDiskCache];
    }
    return self;
}

- (void)dealloc {
    [self.adDestinationDisplayAgent cancel];
}

- (MPAdConfiguration *)adConfig {
    return self.delegate.configuration;
}

#pragma mark - MPInterstitialCustomEvent Override

- (void)requestInterstitialWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup {
    [self loadAd];
}

- (void)showInterstitialFromRootViewController:(UIViewController *)rootViewController {
    if (self.playerViewController.presentingViewController) {
        MPLogInfo(@"View controller has been presented");
    } else if (rootViewController.presentedViewController) {
        MPLogInfo(@"Root view controller has presented a view controller");
    } else {
        [rootViewController presentViewController:self.playerViewController animated:MP_ANIMATED completion:^{
            [self.playerViewController play];
        }];
    }
}

#pragma mark - Private

- (void)loadAd {
    void (^fetchAdCompletion)(NSError *) = ^(NSError *fetchAdError) {
        if (fetchAdError == nil) {
            // Fetch successfully does not mean load successfully. Use `MPVideoPlayerContainerViewDelegate`
            // to determine the ad load result.
            return;
        }

        if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
            [self.delegate interstitialCustomEvent:self didFailToLoadAdWithError:fetchAdError];
        } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
            [self.delegate rewardedVideoDidFailToLoadAdForCustomEvent:self error:fetchAdError];
        } else {
            MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
        }
    };

    [self fetchAndLoadAdWithConfiguration:self.adConfig fetchAdCompletion:fetchAdCompletion];
}

/**
 Parse the VAST XML and resolve potential wrapper chain, and then precache the media file if needed,
 and finally create the view controller with the media file automatially loaded into it.

 Note: `MPAdConfiguration.precacheRequired` is ignored because video precaching is always enforced
 for VAST. See MoPub documentation: https://developers.mopub.com/dsps/ad-formats/video/

 Note: For the completion handler `fetchAdCompletion`, if the `NSError` is nil, it only means the VAST
 ad has been fetched successfully for the load procedure. Use `MPVideoPlayerContainerViewDelegate` to
 determine the ad load result.
 */
- (void)fetchAndLoadAdWithConfiguration:(MPAdConfiguration *)configuration fetchAdCompletion:(void(^)(NSError *))fetchAdCompletion {
    // Safely fire completion block with error.
    void (^fireComplete)(NSError *) = ^(NSError *error) {
        if (fetchAdCompletion != nil) {
            fetchAdCompletion(error);
        }
    };

    [MPVASTManager fetchVASTWithData:configuration.adResponseData completion:^(MPVASTResponse *response, NSError *error) {
        if (error) {
            fireComplete(error);
            return;
        }

        MPVideoConfig *videoConfig = [[MPVideoConfig alloc] initWithVASTResponse:response additionalTrackers:configuration.vastVideoTrackers];
        videoConfig.isRewarded = configuration.hasValidReward;
        videoConfig.enableEarlyClickthroughForNonRewardedVideo = configuration.enableEarlyClickthroughForNonRewardedVideo;

        if (videoConfig == nil || videoConfig.mediaFiles == nil) {
            fireComplete([NSError errorWithDomain:kMPVASTInterstitialCustomEventErrorDomain
                                             code:MPVASTErrorUnableToFindLinearAdOrMediaFileFromURI
                                         userInfo:nil]);
            return;
        }

        self.videoConfig = videoConfig;
        CGSize windowSize = [UIApplication sharedApplication].keyWindow.bounds.size;
        MPVASTMediaFile *remoteMediaFile = [MPVASTMediaFile bestMediaFileFromCandidates:videoConfig.mediaFiles
                                                                       forContainerSize:windowSize
                                                                   containerScaleFactor:[UIScreen mainScreen].scale];
        if (remoteMediaFile == nil) {
            fireComplete([NSError errorWithDomain:kMPVASTInterstitialCustomEventErrorDomain
                                             code:MPVASTErrorUnableToFindLinearAdOrMediaFileFromURI
                                         userInfo:nil]);
            return;
        }

        self.remoteMediaFileToPlay = remoteMediaFile;
        self.vastTracking = [[MPVASTTracking alloc] initWithVideoConfig:videoConfig
                                                               videoURL:remoteMediaFile.URL];
        NSURL *cacheFileURL = [self.mediaFileCache cachedFileURLForRemoteFileURL:remoteMediaFile.URL];

        // `MPVideoPlayerViewController.init` automatically loads the video and triggers delegate callback
        if ([self.mediaFileCache isRemoteFileCached:remoteMediaFile.URL]) {
            [self.mediaFileCache touchCachedFileForRemoteFile:remoteMediaFile.URL]; // for LRU
            self.playerViewController = [[MPVideoPlayerViewController alloc] initWithVideoURL:cacheFileURL
                                                                                  videoConfig:videoConfig];
            self.playerViewController.delegate = self;
            [self.vastTracking registerVideoViewForViewabilityTracking:self.playerViewController.view];
            [self.playerViewController loadVideo];

            // No error
            fireComplete(nil);
        } else {
            MPURLRequest *request = [MPURLRequest requestWithURL:remoteMediaFile.URL];
            [MPHTTPNetworkSession startTaskWithHttpRequest:request responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
                [self.mediaFileCache storeData:data forRemoteSourceFileURL:remoteMediaFile.URL];
                dispatch_async(dispatch_get_main_queue(), ^{
                    self.playerViewController = [[MPVideoPlayerViewController alloc] initWithVideoURL:cacheFileURL videoConfig:videoConfig];
                    self.playerViewController.delegate = self;
                    [self.vastTracking registerVideoViewForViewabilityTracking:self.playerViewController.view];
                    [self.playerViewController loadVideo];
                });
            } errorHandler:fireComplete];
        }
    }];
}

- (void)dismissPlayerViewController {
    [self.playerViewController dismissViewControllerAnimated:MP_ANIMATED completion:nil];
    self.playerViewController = nil;
}

@end

#pragma mark - MPRewardedVideoCustomEvent

@implementation MPVASTInterstitialCustomEvent (MPRewardedVideoCustomEvent)

- (BOOL)enableAutomaticImpressionAndClickTracking {
    // Although `enableAutomaticImpressionAndClickTracking` is NO, for historic reasons such as ILRD
    // impression tracking we still need to relay on the adapter (the delegate) to track impression.
    return NO;
}

- (void)handleAdPlayedForCustomEventNetwork {
    // no-op
}

- (void)handleCustomEventInvalidated {
    // no-op
}

- (void)presentRewardedVideoFromViewController:(UIViewController *)viewController {
    [self showInterstitialFromRootViewController:viewController];
}

- (void)requestRewardedVideoWithCustomEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup {
    [self loadAd];
}

@end

#pragma mark - MPAdDestinationDisplayAgentDelegate

@implementation MPVASTInterstitialCustomEvent (MPAdDestinationDisplayAgentDelegate)

- (void)displayAgentDidDismissModal {
    [self.playerViewController enableAppLifeCycleEventObservationForAutoPlayPause];
    [self.playerViewController play]; // continue playing after click-through
}

- (void)displayAgentWillLeaveApplication {
    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEventWillLeaveApplication:self];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoWillLeaveApplicationForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)displayAgentWillPresentModal {
    [self.playerViewController pause];
}

- (UIViewController *)viewControllerForPresentingModalView {
    return self.playerViewController;
}

@end

#pragma mark - MPVideoPlayerViewControllerDelegate

@implementation MPVASTInterstitialCustomEvent (MPVideoPlayerViewControllerDelegate)

#pragma mark - MPInterstitialViewControllerAppearanceDelegate

- (void)interstitialWillAppear:(id<MPInterstitialViewController>)interstitial {
    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEventWillAppear:self];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoWillAppearForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)interstitialDidAppear:(id<MPInterstitialViewController>)interstitial {
    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEventDidAppear:self];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoDidAppearForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)interstitialWillDisappear:(id<MPInterstitialViewController>)interstitial {
    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEventWillDisappear:self];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoWillDisappearForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)interstitialDidDisappear:(id<MPInterstitialViewController>)interstitial {
    self.hasAdAvailable = NO;

    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEventDidDisappear:self];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoDidDisappearForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

#pragma mark - MPVideoPlayerContainerViewDelegate

- (UIViewController *)viewControllerForPresentingModalMRAIDExpandedView {
    return self.playerViewController;
}

- (void)videoPlayerContainerViewDidLoadVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView {
    self.hasAdAvailable = YES;

    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEvent:self didLoadAd:nil];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoDidLoadAdForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)videoPlayerContainerViewDidFailToLoadVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView error:(NSError *)error {
    self.hasAdAvailable = NO;

    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEvent:self didFailToLoadAdWithError:error];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoDidFailToPlayForCustomEvent:self error:error];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)videoPlayerContainerViewDidStartVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView duration:(NSTimeInterval)duration {
    // We only support one creative in one ad response, so we trigger all of Start, Impression and
    // CreativeView events at the same time.

    // VAST level impression
    [self.vastTracking handleVideoEvent:MPVideoEventStart videoTimeOffset:0];
    [self.vastTracking handleVideoEvent:MPVideoEventCreativeView videoTimeOffset:0];
    [self.vastTracking handleVideoEvent:MPVideoEventImpression videoTimeOffset:0];

    // ad level impression
    // Although `enableAutomaticImpressionAndClickTracking` is NO, for historic reasons such as ILRD
    // impression tracking we still need to relay on the adapter (the delegate) to track impression.
    // Do not call `[self.vastTracking uniquelySendURLs:self.adConfig.impressionTrackingURLs];` until
    // `enableAutomaticImpressionAndClickTracking` is removed or refactored.
    [self.delegate trackImpression];
}

- (void)videoPlayerContainerViewDidCompleteVideo:(MPVideoPlayerContainerView *)videoPlayerContainerView duration:(NSTimeInterval)duration {
    [self.vastTracking handleVideoEvent:MPVideoEventComplete videoTimeOffset:duration];

    if ([self.delegate conformsToProtocol:@protocol(MPPrivateRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoShouldRewardUserForCustomEvent:self reward:self.delegate.configuration.selectedReward];
    }
}

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
       videoDidReachProgressTime:(NSTimeInterval)videoProgress
                        duration:(NSTimeInterval)duration {
    [self.vastTracking handleVideoProgressEvent:videoProgress videoDuration:duration];
}

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
                 didTriggerEvent:(MPVideoPlayerEvent)event
                   videoProgress:(NSTimeInterval)videoProgress {
    switch (event) {
        case MPVideoPlayerEvent_ClickThrough: {
            [self.adDestinationDisplayAgent displayDestinationForURL:self.videoConfig.clickThroughURL];

            // need to take care of both VAST level and ad level click tracking
            [self.vastTracking handleVideoEvent:MPVideoEventClick videoTimeOffset:videoProgress];
            if (self.adConfig.clickTrackingURL != nil) { // prevent crash: nil element in set
                [self.vastTracking uniquelySendURLs:@[self.adConfig.clickTrackingURL]];
            }

            // No need for call `[self.delegate trackClick]` since `enableAutomaticImpressionAndClickTracking` is NO
            if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
                [self.delegate interstitialCustomEventDidReceiveTapEvent:self];
            } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
                [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self];
            } else {
                MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
            }
            break;
        }
        case MPVideoPlayerEvent_Close: {
            // Typically the creative only has one of the "close" tracker and the "closeLinear"
            // tracker. If it has both trackers, we send both as it asks for.
            [self.vastTracking handleVideoEvent:MPVideoEventClose videoTimeOffset:videoProgress];
            [self.vastTracking handleVideoEvent:MPVideoEventCloseLinear videoTimeOffset:videoProgress];
            [self.vastTracking stopViewabilityTracking];
            [self dismissPlayerViewController];
            break;
        }
        case MPVideoPlayerEvent_Skip: {
            // Typically the creative only has one of the "close" tracker and the "closeLinear"
            // tracker. If it has both trackers, we send both as it asks for.
            [self.vastTracking handleVideoEvent:MPVideoEventSkip videoTimeOffset:videoProgress];
            [self.vastTracking handleVideoEvent:MPVideoEventClose videoTimeOffset:videoProgress];
            [self.vastTracking handleVideoEvent:MPVideoEventCloseLinear videoTimeOffset:videoProgress];
            [self.vastTracking stopViewabilityTracking];
            [self dismissPlayerViewController];
            break;
        }
    }
}

#pragma mark - industry icon view

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
         didShowIndustryIconView:(MPVASTIndustryIconView *)iconView {
    [self.vastTracking uniquelySendURLs:iconView.icon.viewTrackingURLs];
}

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerView
        didClickIndustryIconView:(MPVASTIndustryIconView *)iconView
        overridingClickThroughURL:(NSURL * _Nullable)url {
    if (url.absoluteString.length > 0) {
        [self.adDestinationDisplayAgent displayDestinationForURL:url];
    } else {
        [self.adDestinationDisplayAgent displayDestinationForURL:iconView.icon.clickThroughURL];
    }

    [self.playerViewController disableAppLifeCycleEventObservationForAutoPlayPause];
    [self.vastTracking uniquelySendURLs:iconView.icon.clickTrackingURLs];
}

#pragma mark - companion ad view

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
          didShowCompanionAdView:(MPVASTCompanionAdView *)companionAdView {
    // Aggregate trackers
    NSMutableSet<NSURL *> *urls = [NSMutableSet new];
    for (MPVASTTrackingEvent *event in companionAdView.ad.creativeViewTrackers) {
        [urls addObject:event.URL];
    }

    // Additional trackers
    NSArray<MPVASTTrackingEvent *> *additionalTrackingUrls = self.adConfig.vastVideoTrackers[MPVideoEventCompanionAdView];
    [additionalTrackingUrls enumerateObjectsUsingBlock:^(MPVASTTrackingEvent * _Nonnull event, NSUInteger idx, BOOL * _Nonnull stop) {
        [urls addObject:event.URL];
    }];

    [self.vastTracking uniquelySendURLs:urls.allObjects];
}

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
         didClickCompanionAdView:(MPVASTCompanionAdView *)companionAdView
       overridingClickThroughURL:(NSURL * _Nullable)url {
    // Navigation to destination
    if (url.absoluteString.length > 0) {
        [self.adDestinationDisplayAgent displayDestinationForURL:url];
    } else {
        [self.adDestinationDisplayAgent displayDestinationForURL:companionAdView.ad.clickThroughURL];
    }

    [self.playerViewController disableAppLifeCycleEventObservationForAutoPlayPause];

    // Aggregate trackers with additional trackers
    NSMutableSet<NSURL *> *urls = [NSMutableSet set];
    if (companionAdView.ad.clickTrackingURLs != nil) {
        [urls addObjectsFromArray:companionAdView.ad.clickTrackingURLs];
    }
    if (self.adConfig.clickTrackingURL != nil) {
        [urls addObject:self.adConfig.clickTrackingURL];
    }

    NSArray<MPVASTTrackingEvent *> *additionalTrackingUrls = self.adConfig.vastVideoTrackers[MPVideoEventCompanionAdClick];
    [additionalTrackingUrls enumerateObjectsUsingBlock:^(MPVASTTrackingEvent * _Nonnull event, NSUInteger idx, BOOL * _Nonnull stop) {
        [urls addObject:event.URL];
    }];

    [self.vastTracking uniquelySendURLs:urls.allObjects];

    // Notify delegates
    if ([self.delegate conformsToProtocol:@protocol(MPInterstitialCustomEventDelegate)]) {
        [self.delegate interstitialCustomEventDidReceiveTapEvent:self];
    } else if ([self.delegate conformsToProtocol:@protocol(MPRewardedVideoCustomEventDelegate)]) {
        [self.delegate rewardedVideoDidReceiveTapEventForCustomEvent:self];
    } else {
        MPLogInfo(@"%s unexpected delegate: %@", __FUNCTION__, self.delegate);
    }
}

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
    didFailToLoadCompanionAdView:(MPVASTCompanionAdView *)companionAdView {
    [self.vastTracking handleVASTError:MPVASTErrorGeneralCompanionAdsError
                       videoTimeOffset:kMPVASTMacroProcessorUnknownTimeOffset];
}

- (void)videoPlayerContainerView:(MPVideoPlayerContainerView *)videoPlayerContainerView
   companionAdViewRequestDismiss:(MPVASTCompanionAdView *)companionAdView {
    [self dismissPlayerViewController];
}

@end
