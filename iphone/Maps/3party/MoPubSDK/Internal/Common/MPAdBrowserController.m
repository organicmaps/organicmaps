//
//  MPAdBrowserController.m
//  MoPub
//
//  Created by Nafis Jamal on 1/19/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPAdBrowserController.h"
#import "MPLogging.h"
#import "MPAdConfiguration.h"
#import "MPAPIEndpoints.h"
#import "NSBundle+MPAdditions.h"
#import "MPURLRequest.h"

static NSString * const kAdBrowserControllerNibName = @"MPAdBrowserController";

@interface MPAdBrowserController ()

@property (weak, nonatomic) IBOutlet UINavigationBar *navigationBar;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *navigationBarYConstraint;

@property (weak, nonatomic) IBOutlet UIToolbar *browserControlToolbar;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *browserControlToolbarBottomConstraint;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint *webViewTopConstraint;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *webViewLeadingConstraint;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *webViewTrailingConstraint;

@property (nonatomic, strong) UIActionSheet *actionSheet;
@property (nonatomic, strong) NSString *HTMLString;
@property (nonatomic, assign) int webViewLoadCount;

- (void)dismissActionSheet;

@end

@implementation MPAdBrowserController

#pragma mark -
#pragma mark Lifecycle

- (instancetype)initWithURL:(NSURL *)URL HTMLString:(NSString *)HTMLString delegate:(id<MPAdBrowserControllerDelegate>)delegate
{
    if (self = [super initWithNibName:kAdBrowserControllerNibName bundle:[NSBundle resourceBundleForClass:self.class]])
    {
        self.delegate = delegate;
        self.URL = URL;
        self.HTMLString = HTMLString;

        MPLogDebug(@"Ad browser (%p) initialized with URL: %@", self, self.URL);

        self.spinner = [[UIActivityIndicatorView alloc] initWithFrame:CGRectZero];
        [self.spinner sizeToFit];
        self.spinner.hidesWhenStopped = YES;

        self.webViewLoadCount = 0;
    }
    return self;
}

- (instancetype)initWithURL:(NSURL *)URL delegate:(id<MPAdBrowserControllerDelegate>)delegate {
    return [self initWithURL:URL
                  HTMLString:nil
                    delegate:delegate];
}

- (void)dealloc
{
    self.webView.delegate = nil;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Set web view delegate
    self.webView.delegate = self;
    self.webView.scalesPageToFit = YES;

    // Set up toolbar buttons
    self.backButton.image = [self backArrowImage];
    self.backButton.title = nil;
    self.forwardButton.image = [self forwardArrowImage];
    self.forwardButton.title = nil;
    self.spinnerItem.customView = self.spinner;
    self.spinnerItem.title = nil;

    // If iOS 11, set up autolayout constraints so that the toolbar and web view stay within the safe area
    // Note: The web view has to be constrained to the safe area on top for the notch in Portait and leading/trailing
    // for the notch in Landscape. Only the bottom of the toolbar needs to be constrained because Apple will move
    // the buttons into the safe area automatically in Landscape, and otherwise it's preferable for the toolbar to
    // stretch the length of the unsafe area as well.
    if (@available(iOS 11, *)) {
        // Disable the old constraints
        self.navigationBarYConstraint.active = NO;
        self.browserControlToolbarBottomConstraint.active = NO;
        self.webViewTopConstraint.active = NO;
        self.webViewLeadingConstraint.active = NO;
        self.webViewTrailingConstraint.active = NO;

        // Set new constraints based on the safe area layout guide
        self.navigationBarYConstraint = [self.navigationBar.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor]; // put nav bar just above safe area
        self.browserControlToolbarBottomConstraint = [self.browserControlToolbar.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor];
        self.webViewTopConstraint = [self.webView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor];
        self.webViewLeadingConstraint = [self.webView.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor];
        self.webViewTrailingConstraint = [self.webView.trailingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor];

        // Enable the new constraints
        [NSLayoutConstraint activateConstraints:@[
                                                  self.navigationBarYConstraint,
                                                  self.browserControlToolbarBottomConstraint,
                                                  self.webViewTopConstraint,
                                                  self.webViewLeadingConstraint,
                                                  self.webViewTrailingConstraint,
                                                  ]];
    }

    // Set web view background color to white so scrolling at extremes won't have a gray background
    self.webView.backgroundColor = [UIColor whiteColor];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];

    // Set button enabled status.
    self.backButton.enabled = self.webView.canGoBack;
    self.forwardButton.enabled = self.webView.canGoForward;
    self.refreshButton.enabled = NO;
    self.safariButton.enabled = NO;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];

    NSURL *baseURL = (self.URL != nil) ? self.URL : [NSURL URLWithString:[MPAPIEndpoints baseURL]];

    if (self.HTMLString) {
        [self.webView loadHTMLString:self.HTMLString baseURL:baseURL];
    } else {
        [self.webView loadRequest:[MPURLRequest requestWithURL:self.URL]];
    }
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self.webView stopLoading];
    [super viewWillDisappear:animated];
}

#pragma mark - Hidding status bar (iOS 7 and above)

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

#pragma mark -
#pragma mark Navigation

- (IBAction)refresh
{
    [self dismissActionSheet];
    [self.webView reload];
}

- (IBAction)done
{
    [self dismissActionSheet];
    if (self.delegate) {
        [self.delegate dismissBrowserController:self animated:MP_ANIMATED];
    } else {
        [self dismissViewControllerAnimated:MP_ANIMATED completion:nil];
    }
}

- (IBAction)back
{
    [self dismissActionSheet];
    [self.webView goBack];
    self.backButton.enabled = self.webView.canGoBack;
    self.forwardButton.enabled = self.webView.canGoForward;
}

- (IBAction)forward
{
    [self dismissActionSheet];
    [self.webView goForward];
    self.backButton.enabled = self.webView.canGoBack;
    self.forwardButton.enabled = self.webView.canGoForward;
}

- (IBAction)safari
{
    if (self.actionSheet) {
        [self dismissActionSheet];
    } else {
        self.actionSheet = [[UIActionSheet alloc] initWithTitle:nil
                                                       delegate:self
                                              cancelButtonTitle:@"Cancel"
                                         destructiveButtonTitle:nil
                                              otherButtonTitles:@"Open in Safari", nil];

        if ([UIActionSheet instancesRespondToSelector:@selector(showFromBarButtonItem:animated:)]) {
            [self.actionSheet showFromBarButtonItem:self.safariButton animated:YES];
        } else {
            [self.actionSheet showInView:self.webView];
        }
    }
}

- (void)dismissActionSheet
{
    [self.actionSheet dismissWithClickedButtonIndex:0 animated:YES];

}

#pragma mark -
#pragma mark UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
    self.actionSheet = nil;
    if (buttonIndex == 0) {
        // Open in Safari.
        [[UIApplication sharedApplication] openURL:self.URL];
    }
}

#pragma mark -
#pragma mark MPWebViewDelegate

- (BOOL)webView:(MPWebView *)webView
shouldStartLoadWithRequest:(NSURLRequest *)request
 navigationType:(UIWebViewNavigationType)navigationType
{
    MPLogDebug(@"Ad browser (%p) starting to load URL: %@", self, request.URL);
    self.URL = request.URL;

    BOOL appShouldOpenURL = ![self.URL.scheme isEqualToString:@"http"] && ![self.URL.scheme isEqualToString:@"https"];

    if (appShouldOpenURL) {
        [[UIApplication sharedApplication] openURL:self.URL];
    }

    return !appShouldOpenURL;
}

- (void)webViewDidStartLoad:(MPWebView *)webView
{
    self.refreshButton.enabled = YES;
    self.safariButton.enabled = YES;
    [self.spinner startAnimating];

    self.webViewLoadCount++;
}

- (void)webViewDidFinishLoad:(MPWebView *)webView
{
    self.webViewLoadCount--;
    if (self.webViewLoadCount > 0) return;

    self.refreshButton.enabled = YES;
    self.safariButton.enabled = YES;
    self.backButton.enabled = self.webView.canGoBack;
    self.forwardButton.enabled = self.webView.canGoForward;
    [self.spinner stopAnimating];
}

- (void)webView:(MPWebView *)webView didFailLoadWithError:(NSError *)error
{
    self.webViewLoadCount--;

    self.refreshButton.enabled = YES;
    self.safariButton.enabled = YES;
    self.backButton.enabled = self.webView.canGoBack;
    self.forwardButton.enabled = self.webView.canGoForward;
    [self.spinner stopAnimating];

    // Ignore NSURLErrorDomain error (-999).
    if (error.code == NSURLErrorCancelled) return;

    // Ignore "Frame Load Interrupted" errors after navigating to iTunes or the App Store.
    if (error.code == 102 && [error.domain isEqual:@"WebKitErrorDomain"]) return;

    MPLogError(@"Ad browser (%p) experienced an error: %@.", self, [error localizedDescription]);
}

#pragma mark -
#pragma mark Drawing

- (CGContextRef)createContext
{
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(nil,27,27,8,0,
                                                 colorSpace,(CGBitmapInfo)kCGImageAlphaPremultipliedLast);
    CFRelease(colorSpace);
    return context;
}

- (UIImage *)backArrowImage
{
    CGContextRef context = [self createContext];
    CGColorRef fillColor = [[UIColor blackColor] CGColor];
    CGContextSetFillColor(context, CGColorGetComponents(fillColor));

    CGContextBeginPath(context);
    CGContextMoveToPoint(context, 8.0f, 13.0f);
    CGContextAddLineToPoint(context, 24.0f, 4.0f);
    CGContextAddLineToPoint(context, 24.0f, 22.0f);
    CGContextClosePath(context);
    CGContextFillPath(context);

    CGImageRef imageRef = CGBitmapContextCreateImage(context);
    CGContextRelease(context);

    UIImage *image = [[UIImage alloc] initWithCGImage:imageRef];
    CGImageRelease(imageRef);
    return image;
}

- (UIImage *)forwardArrowImage
{
    CGContextRef context = [self createContext];
    CGColorRef fillColor = [[UIColor blackColor] CGColor];
    CGContextSetFillColor(context, CGColorGetComponents(fillColor));

    CGContextBeginPath(context);
    CGContextMoveToPoint(context, 24.0f, 13.0f);
    CGContextAddLineToPoint(context, 8.0f, 4.0f);
    CGContextAddLineToPoint(context, 8.0f, 22.0f);
    CGContextClosePath(context);
    CGContextFillPath(context);

    CGImageRef imageRef = CGBitmapContextCreateImage(context);
    CGContextRelease(context);

    UIImage *image = [[UIImage alloc] initWithCGImage:imageRef];
    CGImageRelease(imageRef);
    return image;
}

@end
