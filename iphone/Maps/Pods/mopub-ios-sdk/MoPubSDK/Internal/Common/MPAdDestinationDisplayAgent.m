//
//  MPAdDestinationDisplayAgent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPLastResortDelegate.h"
#import "MPLogging.h"
#import "NSURL+MPAdditions.h"
#import "MPCoreInstanceProvider.h"
#import "MPAnalyticsTracker.h"
#import "MOPUBExperimentProvider.h"
#import "MoPub+Utility.h"
#import "SKStoreProductViewController+MPAdditions.h"
#import <SafariServices/SafariServices.h>

static NSString * const kDisplayAgentErrorDomain = @"com.mopub.displayagent";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdDestinationDisplayAgent () <SFSafariViewControllerDelegate, SKStoreProductViewControllerDelegate>

@property (nonatomic, strong) MPURLResolver *resolver;
@property (nonatomic, strong) MPURLResolver *enhancedDeeplinkFallbackResolver;
@property (nonatomic, strong) MPProgressOverlayView *overlayView;
@property (nonatomic, assign) BOOL isLoadingDestination;
@property (nonatomic) MOPUBDisplayAgentType displayAgentType;
@property (nonatomic, strong) SKStoreProductViewController *storeKitController;
@property (nonatomic, strong) SFSafariViewController *safariController;

@property (nonatomic, strong) MPActivityViewControllerHelper *activityViewControllerHelper;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdDestinationDisplayAgent

@synthesize delegate;

+ (MPAdDestinationDisplayAgent *)agentWithDelegate:(id<MPAdDestinationDisplayAgentDelegate>)delegate
{
    MPAdDestinationDisplayAgent *agent = [[MPAdDestinationDisplayAgent alloc] init];
    agent.delegate = delegate;
    agent.overlayView = [[MPProgressOverlayView alloc] initWithDelegate:agent];
    agent.activityViewControllerHelper = [[MPActivityViewControllerHelper alloc] initWithDelegate:agent];
    agent.displayAgentType = MOPUBExperimentProvider.sharedInstance.displayAgentType;
    return agent;
}

- (void)dealloc
{
    [self dismissAllModalContent];

    self.overlayView.delegate = nil;

    // XXX: If this display agent is deallocated while a StoreKit controller is still on-screen,
    // nil-ing out the controller's delegate would leave us with no way to dismiss the controller
    // in the future. Therefore, we change the controller's delegate to a singleton object which
    // implements SKStoreProductViewControllerDelegate and is always around.
    self.storeKitController.delegate = [MPLastResortDelegate sharedDelegate];
}

- (void)dismissAllModalContent
{
    [self.overlayView hide];
}

+ (BOOL)shouldDisplayContentInApp
{
    switch (MOPUBExperimentProvider.sharedInstance.displayAgentType) {
        case MOPUBDisplayAgentTypeInApp:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        case MOPUBDisplayAgentTypeSafariViewController:
#pragma clang diagnostic pop
            return YES;
        case MOPUBDisplayAgentTypeNativeSafari:
            return NO;
    }
}

- (void)displayDestinationForURL:(NSURL *)URL
{
    if (self.isLoadingDestination) return;
    self.isLoadingDestination = YES;

    [self.delegate displayAgentWillPresentModal];
    [self.overlayView show];

    [self.resolver cancel];
    [self.enhancedDeeplinkFallbackResolver cancel];

    __weak __typeof__(self) weakSelf = self;
    self.resolver = [MPURLResolver resolverWithURL:URL completion:^(MPURLActionInfo *suggestedAction, NSError *error) {
        __typeof__(self) strongSelf = weakSelf;
        if (error) {
            [strongSelf failedToResolveURLWithError:error];
        } else {
            [strongSelf handleSuggestedURLAction:suggestedAction isResolvingEnhancedDeeplink:NO];
        }
    }];

    [self.resolver start];
}

- (void)cancel
{
    if (self.isLoadingDestination) {
        [self.resolver cancel];
        [self.enhancedDeeplinkFallbackResolver cancel];
        [self hideOverlay];
        [self completeDestinationLoading];
    }
}

- (BOOL)handleSuggestedURLAction:(MPURLActionInfo *)actionInfo isResolvingEnhancedDeeplink:(BOOL)isResolvingEnhancedDeeplink
{
    if (actionInfo == nil) {
        [self failedToResolveURLWithError:[NSError errorWithDomain:kDisplayAgentErrorDomain code:-1 userInfo:@{NSLocalizedDescriptionKey: @"Invalid URL action"}]];
        return NO;
    }

    BOOL success = YES;

    switch (actionInfo.actionType) {
        case MPURLActionTypeStoreKit:
            [self showStoreKitWithAction:actionInfo];
            break;
        case MPURLActionTypeGenericDeeplink:
            [self openURLInApplication:actionInfo.deeplinkURL];
            break;
        case MPURLActionTypeEnhancedDeeplink:
            if (isResolvingEnhancedDeeplink) {
                // We end up here if we encounter a nested enhanced deeplink. We'll simply disallow
                // this to avoid getting into cycles.
                [self failedToResolveURLWithError:[NSError errorWithDomain:kDisplayAgentErrorDomain code:-1 userInfo:@{NSLocalizedDescriptionKey: @"Cannot resolve an enhanced deeplink that is nested within another enhanced deeplink."}]];
                success = NO;
            } else {
                [self handleEnhancedDeeplinkRequest:actionInfo.enhancedDeeplinkRequest];
            }
            break;
        case MPURLActionTypeOpenInSafari:
            [self openURLInApplication:actionInfo.safariDestinationURL];
            break;
        case MPURLActionTypeOpenInWebView:
            [self showWebViewWithHTMLString:actionInfo.HTTPResponseString baseURL:actionInfo.webViewBaseURL actionType:MPURLActionTypeOpenInWebView];
            break;
        case MPURLActionTypeOpenURLInWebView:
            [self showWebViewWithHTMLString:actionInfo.HTTPResponseString baseURL:actionInfo.originalURL actionType:MPURLActionTypeOpenInWebView];
            break;
        case MPURLActionTypeShare:
            [self openShareURL:actionInfo.shareURL];
            break;
        default:
            [self failedToResolveURLWithError:[NSError errorWithDomain:kDisplayAgentErrorDomain code:-2 userInfo:@{NSLocalizedDescriptionKey: @"Unrecognized URL action type."}]];
            success = NO;
            break;
    }

    return success;
}

- (void)handleEnhancedDeeplinkRequest:(MPEnhancedDeeplinkRequest *)request
{
    [MoPub openURL:request.primaryURL options:@{} completion:^(BOOL didOpenURLSuccessfully) {
        if (didOpenURLSuccessfully) {
            [self hideOverlay];
            [self.delegate displayAgentWillLeaveApplication];
            [self completeDestinationLoading];
            [[MPAnalyticsTracker sharedTracker] sendTrackingRequestForURLs:request.primaryTrackingURLs];
        } else if (request.fallbackURL) {
            [self handleEnhancedDeeplinkFallbackForRequest:request];
        } else {
            [self openURLInApplication:request.originalURL];
        }
    }];
}

- (void)handleEnhancedDeeplinkFallbackForRequest:(MPEnhancedDeeplinkRequest *)request
{
    __weak __typeof__(self) weakSelf = self;
    [self.enhancedDeeplinkFallbackResolver cancel];
    self.enhancedDeeplinkFallbackResolver = [MPURLResolver resolverWithURL:request.fallbackURL completion:^(MPURLActionInfo *actionInfo, NSError *error) {
        __typeof__(self) strongSelf = weakSelf;
        if (error) {
            // If the resolver fails, just treat the entire original URL as a regular deeplink.
            [strongSelf openURLInApplication:request.originalURL];
        }
        else {
            // Otherwise, the resolver will return us a URL action. We process that action
            // normally with one exception: we don't follow any nested enhanced deeplinks.
            BOOL success = [strongSelf handleSuggestedURLAction:actionInfo isResolvingEnhancedDeeplink:YES];
            if (success) {
                [[MPAnalyticsTracker sharedTracker] sendTrackingRequestForURLs:request.fallbackTrackingURLs];
            }
        }
    }];
    [self.enhancedDeeplinkFallbackResolver start];
}

- (void)showWebViewWithHTMLString:(NSString *)HTMLString baseURL:(NSURL *)URL actionType:(MPURLActionType)actionType {
    switch (self.displayAgentType) {
        case MOPUBDisplayAgentTypeInApp:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        case MOPUBDisplayAgentTypeSafariViewController:
#pragma clang diagnostic pop
            self.safariController = ({
                SFSafariViewController * controller = [[SFSafariViewController alloc] initWithURL:URL];
                controller.delegate = self;
                controller.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
                controller.modalPresentationStyle = UIModalPresentationFullScreen;
                controller;
            });

            [self showAdBrowserController];
            break;
        case MOPUBDisplayAgentTypeNativeSafari:
            [self openURLInApplication:URL];
            break;
    }
}

- (void)showAdBrowserController {
    [self hideOverlay];
    [[self.delegate viewControllerForPresentingModalView] presentViewController:self.safariController
                                                                       animated:MP_ANIMATED
                                                                     completion:nil];
}

- (void)showStoreKitProductWithParameters:(NSDictionary *)parameters fallbackURL:(NSURL *)URL
{
    if (!SKStoreProductViewController.canUseStoreProductViewController) {
        [self openURLInApplication:URL];
        return;
    }

    [self presentStoreKitControllerWithProductParameters:parameters fallbackURL:URL];
}

- (void)openURLInApplication:(NSURL *)URL
{
    [self hideOverlay];

    [MoPub openURL:URL options:@{} completion:^(BOOL didOpenURLSuccessfully) {
        if (didOpenURLSuccessfully) {
            [self.delegate displayAgentWillLeaveApplication];
        }
        [self completeDestinationLoading];
    }];
}

- (BOOL)openShareURL:(NSURL *)URL
{
    MPLogDebug(@"MPAdDestinationDisplayAgent - loading Share URL: %@", URL);
    MPMoPubShareHostCommand command = [URL mp_MoPubShareHostCommand];
    switch (command) {
        case MPMoPubShareHostCommandTweet:
            return [self.activityViewControllerHelper presentActivityViewControllerWithTweetShareURL:URL];
        default:
            MPLogInfo(@"MPAdDestinationDisplayAgent - unsupported Share URL: %@", [URL absoluteString]);
            return NO;
    }
}

- (void)failedToResolveURLWithError:(NSError *)error
{
    [self hideOverlay];
    [self completeDestinationLoading];
}

- (void)completeDestinationLoading
{
    self.isLoadingDestination = NO;
    [self.delegate displayAgentDidDismissModal];
}

- (void)presentStoreKitControllerWithProductParameters:(NSDictionary *)parameters fallbackURL:(NSURL *)URL
{
    self.storeKitController = [[SKStoreProductViewController alloc] init];
    self.storeKitController.modalPresentationStyle = UIModalPresentationFullScreen;
    self.storeKitController.delegate = self;
    [self.storeKitController loadProductWithParameters:parameters completionBlock:nil];

    [self hideOverlay];
    [[self.delegate viewControllerForPresentingModalView] presentViewController:self.storeKitController animated:MP_ANIMATED completion:nil];
}

#pragma mark - <SKStoreProductViewControllerDelegate>

- (void)productViewControllerDidFinish:(SKStoreProductViewController *)viewController
{
    self.isLoadingDestination = NO;
    [self hideModalAndNotifyDelegate];
}

#pragma mark - <SFSafariViewControllerDelegate>

- (void)safariViewControllerDidFinish:(SFSafariViewController *)controller {
    self.isLoadingDestination = NO;
    [self.delegate displayAgentDidDismissModal];
}

#pragma mark - <MPProgressOverlayViewDelegate>

- (void)overlayCancelButtonPressed
{
    [self cancel];
}

#pragma mark - Convenience Methods

- (void)hideModalAndNotifyDelegate
{
    [[self.delegate viewControllerForPresentingModalView] dismissViewControllerAnimated:MP_ANIMATED completion:^{
        [self.delegate displayAgentDidDismissModal];
    }];
}

- (void)hideOverlay
{
    [self.overlayView hide];
}

#pragma mark <MPActivityViewControllerHelperDelegate>

- (UIViewController *)viewControllerForPresentingActivityViewController
{
    return self.delegate.viewControllerForPresentingModalView;
}

- (void)activityViewControllerWillPresent
{
    [self hideOverlay];
    self.isLoadingDestination = NO;
    [self.delegate displayAgentWillPresentModal];
}

- (void)activityViewControllerDidDismiss
{
    [self.delegate displayAgentDidDismissModal];
}

#pragma mark - Experiment with 3 display agent types: 0 -> keep existing, 1 -> use native safari, 2 -> use SafariViewController

- (void)showStoreKitWithAction:(MPURLActionInfo *)actionInfo
{
    switch (self.displayAgentType) {
        case MOPUBDisplayAgentTypeInApp:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        case MOPUBDisplayAgentTypeSafariViewController: // It doesn't make sense to open store kit in SafariViewController so storeKitController is used here.
#pragma clang diagnostic pop
            [self showStoreKitProductWithParameters:actionInfo.iTunesStoreParameters
                                        fallbackURL:actionInfo.iTunesStoreFallbackURL];
            break;
        case MOPUBDisplayAgentTypeNativeSafari:
            [self openURLInApplication:actionInfo.iTunesStoreFallbackURL];
            break;
        default:
            break;
    }
}

@end
