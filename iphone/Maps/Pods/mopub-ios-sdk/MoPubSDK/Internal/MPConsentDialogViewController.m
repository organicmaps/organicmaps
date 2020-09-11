//
//  MPConsentDialogViewController.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAPIEndpoints.h"
#import "MPConsentDialogViewController.h"
#import "MPGlobal.h"
#import "MPWebView.h"
#import "MoPub+Utility.h"

typedef void(^MPConsentDialogViewControllerCompletion)(BOOL success, NSError *error);

static NSString * const kMoPubScheme = @"mopub";
static NSString * const kMoPubConsentURLHost = @"consent";
static NSString * const kMoPubCloseURLHost = @"close";

static NSString * const kMoPubAffirmativeConsentQueryString = @"yes";
static NSString * const kMoPubNegativeConsentQueryString = @"no";

static CGFloat const kCloseButtonDimension = 40.0;
static CGFloat const kCloseButtonSpacing = 7.0;
static NSTimeInterval const kCloseButtonFadeInAnimationDuration = 0.2;
static NSTimeInterval const kCloseButtonFadeInAfterSeconds = 10.0;

@interface MPConsentDialogViewController () <MPWebViewDelegate>

@property (nonatomic, strong) UIButton *closeButton;
@property (nonatomic, strong) MPWebView *webView;
@property (nonatomic, assign) BOOL finishedInitialLoad;
@property (nonatomic, assign) BOOL closeButtonHasBeenShown;
@property (nonatomic, copy) MPConsentDialogViewControllerCompletion didLoadCompletionBlock;

@property (nonatomic, copy) NSString *dialogHTML;

@end

@implementation MPConsentDialogViewController

#pragma mark - Initialization

- (instancetype)initWithDialogHTML:(NSString *)dialogHTML {
    if (self = [super initWithNibName:nil bundle:nil]) {
        // Set internal variables
        _dialogHTML = dialogHTML;

        // Initialize web view
        [self setUpWebView];

        // Ensure fullscreen presentation
        self.modalPresentationStyle = UIModalPresentationFullScreen;
    }

    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Make background color black
    self.view.backgroundColor = [UIColor blackColor];

    // It is important to layout the web view first, then set up the close button so that the button appears on top.
    [self layoutWebView];
    [self setUpCloseButton];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];

    // Fade in close button
    __weak __typeof__(self) weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(kCloseButtonFadeInAfterSeconds * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [weakSelf fadeInCloseButton];
    });
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];

    if ([self.delegate respondsToSelector:@selector(consentDialogViewControllerWillDisappear:)]) {
        [self.delegate consentDialogViewControllerWillDisappear:self];
    }
}

- (void)closeConsentDialog {
    [self dismissViewControllerAnimated:YES completion:^{
        // Intentionally holding a strong reference to @c self here so that the view controller doesn't deallocate
        // before the delegate method is called.
        if ([self.delegate respondsToSelector:@selector(consentDialogViewControllerDidDismiss:)]) {
            [self.delegate consentDialogViewControllerDidDismiss:self];
        }
    }];
}

- (void)setUpWebView {
    self.webView = [[MPWebView alloc] initWithFrame:CGRectZero];
    self.webView.delegate = self;
    self.webView.scrollView.bounces = NO;
    self.webView.backgroundColor = [UIColor whiteColor];
    self.webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
}

- (void)layoutWebView {
    self.webView.frame = self.view.bounds;
    [self.view addSubview:self.webView];

    // Set up autolayout constraints on iOS 11+. This web view should always stay within the safe area.
    if (@available(iOS 11, *)) {
        self.webView.translatesAutoresizingMaskIntoConstraints = NO;
        [NSLayoutConstraint activateConstraints:@[
                                                  [self.webView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
                                                  [self.webView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor],
                                                  [self.webView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor],
                                                  [self.webView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
                                                  ]];
    }
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)setUpCloseButton {
    self.closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
    self.closeButton.frame = CGRectMake(CGRectGetMaxX(self.view.bounds) - kCloseButtonDimension - kCloseButtonSpacing,
                                        CGRectGetMinY(self.view.bounds) + kCloseButtonSpacing,
                                        kCloseButtonDimension,
                                        kCloseButtonDimension);
    self.closeButton.backgroundColor = [UIColor clearColor];
    [self.closeButton setImage:[UIImage imageWithContentsOfFile:MPResourcePathForResource(@"MPCloseButtonX.png")] forState:UIControlStateNormal];
    [self.closeButton addTarget:self
                         action:@selector(closeButtonAction:)
               forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.closeButton];

    if (@available(iOS 11, *)) {
        self.closeButton.translatesAutoresizingMaskIntoConstraints = NO;
        [NSLayoutConstraint activateConstraints:@[
                                                  [self.closeButton.widthAnchor constraintEqualToConstant:kCloseButtonDimension],
                                                  [self.closeButton.heightAnchor constraintEqualToConstant:kCloseButtonDimension],
                                                  [self.closeButton.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:kCloseButtonSpacing],
                                                  [self.closeButton.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor constant:-kCloseButtonSpacing],
                                                  ]];
    }

    // Set close button alpha to zero to fade in later
    self.closeButton.alpha = 0.0;
}

- (IBAction)closeButtonAction:(id)sender {
    [self closeConsentDialog];
}

- (void)fadeInCloseButton {
    if (!self.closeButtonHasBeenShown) {
        self.closeButtonHasBeenShown = YES;
        [UIView animateWithDuration:kCloseButtonFadeInAnimationDuration animations:^{
            self.closeButton.alpha = 1.0;
        }];
    }
}

#pragma mark - Load Consent Page

- (void)loadConsentPageWithCompletion:(MPConsentDialogViewControllerCompletion)completion {
    // Set `finishedInitialLoad` to `NO` because this method is (re)doing the initial load
    self.finishedInitialLoad = NO;

    // Copy the completion block to `didLoadCompletionBlock` to run when the page loads or fails to load
    self.didLoadCompletionBlock = completion;

    // Load consent dialog HTML markup
    [self.webView loadHTMLString:self.dialogHTML
                         baseURL:[NSURL URLWithString:[MPAPIEndpoints baseURL]]];
}

#pragma mark - MPWebViewDelegate

- (void)webView:(MPWebView *)webView didFailLoadWithError:(NSError *)error {
    if (!self.finishedInitialLoad) {
        self.finishedInitialLoad = YES;

        if (self.didLoadCompletionBlock) {
            self.didLoadCompletionBlock(NO, error);
            self.didLoadCompletionBlock = nil;
        }
    }
}

- (void)webViewDidFinishLoad:(MPWebView *)webView {
    if (!self.finishedInitialLoad) {
        self.finishedInitialLoad = YES;

        if (self.didLoadCompletionBlock) {
            self.didLoadCompletionBlock(YES, nil);
            self.didLoadCompletionBlock = nil;
        }
    }
}

- (BOOL)webView:(MPWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(WKNavigationType)navigationType {
    BOOL requestIsMoPubScheme = [request.URL.scheme isEqualToString:kMoPubScheme];
    BOOL requestIsMoPubHost = [request.URL.host isEqualToString:MPAPIEndpoints.baseHostname];

    // Kick to Safari if the URL is not of MoPub scheme or hostname
    if (!requestIsMoPubScheme && !requestIsMoPubHost) {
        [MoPub openURL:request.URL];
        return NO;
    }

    // Allow load if request came from ads.mopub.com
    if (requestIsMoPubHost) {
        return YES;
    }

    // Sanity check: do nothing if we get to this point and it is not a MoPub scheme
    if (!requestIsMoPubScheme) {
        return NO;
    }

    // MoPub Scheme

    // Received close command; send dismiss and do not allow load.
    if ([request.URL.host isEqualToString:kMoPubCloseURLHost]) {
        [self closeConsentDialog];
        return NO;
    }

    // Break out if any command other than consent (we do not know any commands besides "close" and "consent"
    if (![request.URL.host isEqualToString:kMoPubConsentURLHost]) {
        return NO;
    }

    // Received consent command; validate query string
    BOOL receivedAffirmativeAnswer = [request.URL.query isEqualToString:kMoPubAffirmativeConsentQueryString];
    BOOL receivedNegativeAnswer = [request.URL.query isEqualToString:kMoPubNegativeConsentQueryString];
    // We should have received one answer or the other (i.e., one of these should be marked as `YES`). If neither
    // was marked as `YES` (i.e., the query string was malformed), break out immediately.
    if (!receivedAffirmativeAnswer && !receivedNegativeAnswer) {
        return NO;
    }
    // Given that we did receive an answer, the receivedAffirmativeAnswer variable is what we expect consentAnswer to be.
    // If consent was given, receivedAffirmativeAnswer will be `YES`.
    // If consent was not given, receivedAffirmativeAnswer will be `NO` (and the above `if` will filter out unknowns).
    BOOL consentAnswer = receivedAffirmativeAnswer;

    // Inform delegate
    if ([self.delegate respondsToSelector:@selector(consentDialogViewControllerDidReceiveConsentResponse:consentDialogViewController:)]) {
        [self.delegate consentDialogViewControllerDidReceiveConsentResponse:consentAnswer
                                                consentDialogViewController:self];
    }

    // Show close button (if it hasn't already been shown)
    [self fadeInCloseButton];

    // Do not load
    return NO;
}

@end
