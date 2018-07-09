//
//  MPAdDestinationDisplayAgent.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MPAdDestinationDisplayAgent.h"
#import "MPCoreInstanceProvider.h"
#import "MPLastResortDelegate.h"
#import "MPLogging.h"
#import "NSURL+MPAdditions.h"
#import "MPCoreInstanceProvider.h"
#import "MPAnalyticsTracker.h"
#import "MOPUBExperimentProvider.h"
#import <SafariServices/SafariServices.h>

static NSString * const kDisplayAgentErrorDomain = @"com.mopub.displayagent";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdDestinationDisplayAgent () <SFSafariViewControllerDelegate>

@property (nonatomic, strong) MPURLResolver *resolver;
@property (nonatomic, strong) MPURLResolver *enhancedDeeplinkFallbackResolver;
@property (nonatomic, strong) MPProgressOverlayView *overlayView;
@property (nonatomic, assign) BOOL isLoadingDestination;
@property (nonatomic) MOPUBDisplayAgentType displayAgentType;
@property (nonatomic, strong) SKStoreProductViewController *storeKitController;

@property (nonatomic, strong) MPAdBrowserController *browserController;
@property (nonatomic, strong) SFSafariViewController *safariController;

@property (nonatomic, strong) MPTelephoneConfirmationController *telephoneConfirmationController;
@property (nonatomic, strong) MPActivityViewControllerHelper *activityViewControllerHelper;

- (void)presentStoreKitControllerWithItemIdentifier:(NSString *)identifier fallbackURL:(NSURL *)URL;
- (void)hideOverlay;
- (void)hideModalAndNotifyDelegate;
- (void)dismissAllModalContent;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdDestinationDisplayAgent

@synthesize delegate = _delegate;
@synthesize resolver = _resolver;
@synthesize isLoadingDestination = _isLoadingDestination;

+ (MPAdDestinationDisplayAgent *)agentWithDelegate:(id<MPAdDestinationDisplayAgentDelegate>)delegate
{
    MPAdDestinationDisplayAgent *agent = [[MPAdDestinationDisplayAgent alloc] init];
    agent.delegate = delegate;
    agent.overlayView = [[MPProgressOverlayView alloc] initWithDelegate:agent];
    agent.activityViewControllerHelper = [[MPActivityViewControllerHelper alloc] initWithDelegate:agent];
    agent.displayAgentType = [MOPUBExperimentProvider displayAgentType];
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

    self.browserController.delegate = nil;

}

- (void)dismissAllModalContent
{
    [self.overlayView hide];
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
    BOOL didOpenSuccessfully = [[UIApplication sharedApplication] openURL:request.primaryURL];
    if (didOpenSuccessfully) {
        [self hideOverlay];
        [self.delegate displayAgentWillLeaveApplication];
        [self completeDestinationLoading];
        [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] sendTrackingRequestForURLs:request.primaryTrackingURLs];
    } else if (request.fallbackURL) {
        [self handleEnhancedDeeplinkFallbackForRequest:request];
    } else {
        [self openURLInApplication:request.originalURL];
    }
}

- (void)handleEnhancedDeeplinkFallbackForRequest:(MPEnhancedDeeplinkRequest *)request;
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
                [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] sendTrackingRequestForURLs:request.fallbackTrackingURLs];
            }
        }
    }];
    [self.enhancedDeeplinkFallbackResolver start];
}

- (void)showWebViewWithHTMLString:(NSString *)HTMLString baseURL:(NSURL *)URL actionType:(MPURLActionType)actionType {
    switch (self.displayAgentType) {
        case MOPUBDisplayAgentTypeInApp:
        case MOPUBDisplayAgentTypeSafariViewController:
            if ([MPAdDestinationDisplayAgent shouldUseSafariViewController]) {
                if (@available(iOS 9.0, *)) {
                    self.safariController = [[SFSafariViewController alloc] initWithURL:URL];
                    self.safariController.delegate = self;
                }
            } else {
                if (actionType == MPURLActionTypeOpenInWebView) {
                    self.browserController = [[MPAdBrowserController alloc] initWithURL:URL
                                                                         HTMLString:HTMLString
                                                                           delegate:self];
                } else {
                    self.browserController = [[MPAdBrowserController alloc] initWithURL:URL
                                                                               delegate:self];
                }
            }
            [self showAdBrowserController];
            break;
        case MOPUBDisplayAgentTypeNativeSafari:
            [self openURLInApplication:URL];
            break;
    }
}

- (void)showAdBrowserController {
    [self hideOverlay];

    UIViewController *browserViewController = self.safariController ? self.safariController : self.browserController;

    browserViewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
    [[self.delegate viewControllerForPresentingModalView] presentViewController:browserViewController
                                                                       animated:MP_ANIMATED
                                                                     completion:nil];
}

- (void)showStoreKitProductWithParameter:(NSString *)parameter fallbackURL:(NSURL *)URL
{
    if ([MPStoreKitProvider deviceHasStoreKit]) {
        [self presentStoreKitControllerWithItemIdentifier:parameter fallbackURL:URL];
    } else {
        [self openURLInApplication:URL];
    }
}

- (void)openURLInApplication:(NSURL *)URL
{
    [self hideOverlay];

    if ([URL mp_hasTelephoneScheme] || [URL mp_hasTelephonePromptScheme]) {
        [self interceptTelephoneURL:URL];
    } else {
        BOOL didOpenSuccessfully = [[UIApplication sharedApplication] openURL:URL];
        if (didOpenSuccessfully) {
            [self.delegate displayAgentWillLeaveApplication];
        }
        [self completeDestinationLoading];
    }
}

- (BOOL)openShareURL:(NSURL *)URL
{
    MPLogDebug(@"MPAdDestinationDisplayAgent - loading Share URL: %@", URL);
    MPMoPubShareHostCommand command = [URL mp_MoPubShareHostCommand];
    switch (command) {
        case MPMoPubShareHostCommandTweet:
            return [self.activityViewControllerHelper presentActivityViewControllerWithTweetShareURL:URL];
        default:
            MPLogWarn(@"MPAdDestinationDisplayAgent - unsupported Share URL: %@", [URL absoluteString]);
            return NO;
    }
}

- (void)interceptTelephoneURL:(NSURL *)URL
{
    __weak MPAdDestinationDisplayAgent *weakSelf = self;
    self.telephoneConfirmationController = [[MPTelephoneConfirmationController alloc] initWithURL:URL clickHandler:^(NSURL *targetTelephoneURL, BOOL confirmed) {
        MPAdDestinationDisplayAgent *strongSelf = weakSelf;
        if (strongSelf) {
            if (confirmed) {
                [strongSelf.delegate displayAgentWillLeaveApplication];
                [[UIApplication sharedApplication] openURL:targetTelephoneURL];
            }
            [strongSelf completeDestinationLoading];
        }
    }];

    [self.telephoneConfirmationController show];
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

- (void)presentStoreKitControllerWithItemIdentifier:(NSString *)identifier fallbackURL:(NSURL *)URL
{
    self.storeKitController = [MPStoreKitProvider buildController];
    self.storeKitController.delegate = self;

    NSDictionary *parameters = [NSDictionary dictionaryWithObject:identifier
                                                           forKey:SKStoreProductParameterITunesItemIdentifier];
    [self.storeKitController loadProductWithParameters:parameters completionBlock:nil];

    [self hideOverlay];
    [[self.delegate viewControllerForPresentingModalView] presentViewController:self.storeKitController animated:MP_ANIMATED completion:nil];
}

#pragma mark - <MPSKStoreProductViewControllerDelegate>

- (void)productViewControllerDidFinish:(SKStoreProductViewController *)viewController
{
    self.isLoadingDestination = NO;
    [self hideModalAndNotifyDelegate];
}

#pragma mark - <MPAdBrowserControllerDelegate>

- (void)dismissBrowserController:(MPAdBrowserController *)browserController animated:(BOOL)animated
{
    self.isLoadingDestination = NO;
    [self hideModalAndNotifyDelegate];
}

- (MPAdConfiguration *)adConfiguration
{
    if ([self.delegate respondsToSelector:@selector(adConfiguration)]) {
        return [self.delegate adConfiguration];
    }

    return nil;
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

+ (BOOL)shouldUseSafariViewController
{
    MOPUBDisplayAgentType displayAgentType = [MOPUBExperimentProvider displayAgentType];
    if (@available(iOS 9.0, *)) {
        return (displayAgentType == MOPUBDisplayAgentTypeSafariViewController);
    }

    return NO;
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
        case MOPUBDisplayAgentTypeSafariViewController: // It doesn't make sense to open store kit in SafariViewController so storeKitController is used here.
            [self showStoreKitProductWithParameter:actionInfo.iTunesItemIdentifier
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
