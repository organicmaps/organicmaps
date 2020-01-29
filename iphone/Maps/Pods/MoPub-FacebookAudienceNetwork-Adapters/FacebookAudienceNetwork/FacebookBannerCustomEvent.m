//
//  FacebookBannerCustomEvent.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <FBAudienceNetwork/FBAudienceNetwork.h>
#import "FacebookBannerCustomEvent.h"
#import "FacebookAdapterConfiguration.h"

#if __has_include("MoPub.h")
    #import "MoPub.h"
    #import "MPLogging.h"
#endif

@interface FacebookBannerCustomEvent () <FBAdViewDelegate>

@property (nonatomic, strong) FBAdView *fbAdView;
@property (nonatomic, copy) NSString *fbPlacementId;

@end

@implementation FacebookBannerCustomEvent

- (BOOL)enableAutomaticImpressionAndClickTracking
{
    return NO;
}

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info
{
    [self requestAdWithSize:size customEventInfo:info adMarkup:nil];
}

- (void)requestAdWithSize:(CGSize)size customEventInfo:(NSDictionary *)info adMarkup:(NSString *)adMarkup
{
    // Determine if the inline ad format is a banner or medium rectangle since
    // Facebook makes this distinction with their placements. Explicitly look
    // for medium rectangle since the default case is `kFBAdSizeHeight50Banner`.
    NSString * format = [info objectForKey:@"adunit_format"];
    BOOL isMediumRectangleFormat = (format != nil ? ![[format lowercaseString] containsString:@"banner"] : NO);
    
    /**
     * Facebook Banner ads can accept arbitrary widths for given heights of 50 and 90. We convert these sizes
     * to Facebook's constants and set the fbAdView's size to the intended size ("size" passed to this method).
     */
    self.fbPlacementId = [info objectForKey:@"placement_id"];
    
    // Default to standard banner size
    FBAdSize fbAdSize = kFBAdSizeHeight50Banner;
    
    // Size can contain a Facebook 90 height banner
    if (size.height >= kFBAdSizeHeight90Banner.size.height) {
        fbAdSize = kFBAdSizeHeight90Banner;
    }
    
    // Size can contain a Facebook medium rectangle
    if (isMediumRectangleFormat && size.height >= kFBAdSizeHeight250Rectangle.size.height) {
        fbAdSize = kFBAdSizeHeight250Rectangle;
    }
    
    if (self.fbPlacementId == nil) {
        NSError *error = [self createErrorWith:@"Invalid Facebook placement ID"
                                     andReason:@""
                                 andSuggestion:@""];
        
        MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], nil);
        [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:error];

        return;
    }

    self.fbAdView = [[FBAdView alloc] initWithPlacementID:self.fbPlacementId
                                                   adSize:fbAdSize
                                       rootViewController:[self.delegate viewControllerForPresentingModalView]];
    self.fbAdView.delegate = self;

    if (!self.fbAdView) {
        NSError *error = [self createErrorWith:@"Facebook failed to load an ad"
                                     andReason:@""
                                 andSuggestion:@""];
        
        [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:error];
        MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], self.fbPlacementId);
        return;
    }

    /*
     * Manually resize the frame of the FBAdView due to a bug in the Facebook SDK that sets the ad's width
     * to the width of the device instead of the width of the container it's placed in.
     * (Confirmed in email with a FB Solutions Engineer)
     */
    CGRect fbAdFrame = self.fbAdView.frame;
    fbAdFrame.size.width = size.width;              // Since FB ad sizes are flexible width, use the container width
    fbAdFrame.size.height = fbAdSize.size.height;   // Use the FB ad size height
    self.fbAdView.frame = fbAdFrame;
    [FBAdSettings setMediationService:[FacebookAdapterConfiguration mediationString]];

    // Load the advanced bid payload.
    if (adMarkup != nil) {
        MPLogInfo(@"Loading Facebook banner ad markup for Advanced Bidding");
        [self.fbAdView loadAdWithBidPayload:adMarkup];

        MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(self.class) dspCreativeId:nil dspName:nil], self.fbPlacementId);
    }
    // Request a banner ad.
    else {
        MPLogInfo(@"Loading Facebook banner ad");
        [self.fbAdView loadAd];

        MPLogAdEvent([MPLogEvent adLoadAttemptForAdapter:NSStringFromClass(self.class) dspCreativeId:nil dspName:nil], self.fbPlacementId);
    }
}

- (NSError *)createErrorWith:(NSString *)description andReason:(NSString *)reaason andSuggestion:(NSString *)suggestion {
    NSDictionary *userInfo = @{
                               NSLocalizedDescriptionKey: NSLocalizedString(description, nil),
                               NSLocalizedFailureReasonErrorKey: NSLocalizedString(reaason, nil),
                               NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(suggestion, nil)
                               };

    return [NSError errorWithDomain:NSStringFromClass([self class]) code:0 userInfo:userInfo];
}

- (void)dealloc
{
    self.fbAdView.delegate = nil;
}

#pragma mark FBAdViewDelegate methods

- (void)adView:(FBAdView *)adView didFailWithError:(NSError *)error
{
    MPLogAdEvent([MPLogEvent adLoadFailedForAdapter:NSStringFromClass(self.class) error:error], nil);

    [self.delegate bannerCustomEvent:self didFailToLoadAdWithError:error];
}

- (void)adViewDidLoad:(FBAdView *)adView
{
    MPLogAdEvent([MPLogEvent adLoadSuccessForAdapter:NSStringFromClass(self.class)], self.fbPlacementId);
    MPLogAdEvent([MPLogEvent adShowAttemptForAdapter:NSStringFromClass(self.class)], self.fbPlacementId);
    MPLogAdEvent([MPLogEvent adShowSuccessForAdapter:NSStringFromClass(self.class)], self.fbPlacementId);

    [self.delegate bannerCustomEvent:self didLoadAd:adView];
}

- (void)adViewWillLogImpression:(FBAdView *)adView
{
    MPLogInfo(@"Facebook banner ad did log impression");
    [self.delegate trackImpression];
}

- (void)adViewDidClick:(FBAdView *)adView
{
    MPLogAdEvent([MPLogEvent adTappedForAdapter:NSStringFromClass(self.class)], self.fbPlacementId);

    [self.delegate trackClick];
    [self.delegate bannerCustomEventWillBeginAction:self];
}

- (void)adViewDidFinishHandlingClick:(FBAdView *)adView
{
    MPLogInfo(@"Facebook banner ad did finish handling click");
    [self.delegate bannerCustomEventDidFinishAction:self];
}

- (UIViewController *)viewControllerForPresentingModalView
{
    return [self.delegate viewControllerForPresentingModalView];
}

@end
