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

static NSString * const kDisplayAgentErrorDomain = @"com.mopub.displayagent";

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPAdDestinationDisplayAgent ()

@property (nonatomic, strong) MPURLResolver *resolver;
@property (nonatomic, strong) MPURLResolver *enhancedDeeplinkFallbackResolver;
@property (nonatomic, strong) MPProgressOverlayView *overlayView;
@property (nonatomic, assign) BOOL isLoadingDestination;

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
@property (nonatomic, strong) SKStoreProductViewController *storeKitController;
#endif

@property (nonatomic, strong) MPAdBrowserController *browserController;
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
    return agent;
}

- (void)dealloc
{
    [self dismissAllModalContent];

    self.overlayView.delegate = nil;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
    // XXX: If this display agent is deallocated while a StoreKit controller is still on-screen,
    // nil-ing out the controller's delegate would leave us with no way to dismiss the controller
    // in the future. Therefore, we change the controller's delegate to a singleton object which
    // implements SKStoreProductViewControllerDelegate and is always around.
    self.storeKitController.delegate = [MPLastResortDelegate sharedDelegate];
#endif
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

    __weak typeof(self) weakSelf = self;
    self.resolver = [[MPCoreInstanceProvider sharedProvider] buildMPURLResolverWithURL:URL completion:^(MPURLActionInfo *suggestedAction, NSError *error) {
        typeof(self) strongSelf = weakSelf;
        if (strongSelf) {
            if (error) {
                [strongSelf failedToResolveURLWithError:error];
            } else {
                [strongSelf handleSuggestedURLAction:suggestedAction isResolvingEnhancedDeeplink:NO];
            }
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
            [self showStoreKitProductWithParameter:actionInfo.iTunesItemIdentifier
                                       fallbackURL:actionInfo.iTunesStoreFallbackURL];
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
            [self showWebViewWithHTMLString:actionInfo.HTTPResponseString
                                    baseURL:actionInfo.webViewBaseURL];
            break;
        case MPURLActionTypeOpenURLInWebView:
            [self showWebViewWithURL:actionInfo.originalURL];
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
    __weak typeof(self) weakSelf = self;
    [self.enhancedDeeplinkFallbackResolver cancel];
    self.enhancedDeeplinkFallbackResolver = [[MPCoreInstanceProvider sharedProvider] buildMPURLResolverWithURL:request.fallbackURL completion:^(MPURLActionInfo *actionInfo, NSError *error) {
        typeof(self) strongSelf = weakSelf;
        if (strongSelf) {
            if (error) {
                // If the resolver fails, just treat the entire original URL as a regular deeplink.
                [strongSelf openURLInApplication:request.originalURL];
            } else {
                // Otherwise, the resolver will return us a URL action. We process that action
                // normally with one exception: we don't follow any nested enhanced deeplinks.
                BOOL success = [strongSelf handleSuggestedURLAction:actionInfo isResolvingEnhancedDeeplink:YES];
                if (success) {
                    [[[MPCoreInstanceProvider sharedProvider] sharedMPAnalyticsTracker] sendTrackingRequestForURLs:request.fallbackTrackingURLs];
                }
            }
        }
    }];
    [self.enhancedDeeplinkFallbackResolver start];
}

- (void)showWebViewWithHTMLString:(NSString *)HTMLString baseURL:(NSURL *)URL
{
    self.browserController = [[MPAdBrowserController alloc] initWithURL:URL
                                                             HTMLString:HTMLString
                                                               delegate:self];
    [self showAdBrowserController:self.browserController];
}

- (void)showWebViewWithURL:(NSURL *)URL {
    self.browserController = [[MPAdBrowserController alloc] initWithURL:URL
                                                               delegate:self];
    [self showAdBrowserController:self.browserController];
}

- (void)showAdBrowserController:(MPAdBrowserController *)adBrowserController {
    [self hideOverlay];

    self.browserController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
    [[self.delegate viewControllerForPresentingModalView] presentViewController:self.browserController
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
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_6_0
    self.storeKitController = [MPStoreKitProvider buildController];
    self.storeKitController.delegate = self;

    NSDictionary *parameters = [NSDictionary dictionaryWithObject:identifier
                                                           forKey:SKStoreProductParameterITunesItemIdentifier];
    [self.storeKitController loadProductWithParameters:parameters completionBlock:nil];

    [self hideOverlay];
    [[self.delegate viewControllerForPresentingModalView] presentViewController:self.storeKitController animated:MP_ANIMATED completion:nil];
#endif
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

@end
