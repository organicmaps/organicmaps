//
//  MPAdBrowserController.m
//  MoPub
//
//  Created by Nafis Jamal on 1/19/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import "MPAdBrowserController.h"
#import "MPLogging.h"
#import "MPLogEvent.h"
#import "MPLogEventRecorder.h"
#import "MPAdConfiguration.h"
#import "MPAPIEndPoints.h"
#import "NSBundle+MPAdditions.h"

static NSString * const kAdBrowserControllerNibName = @"MPAdBrowserController";

@interface MPAdBrowserController ()

@property (nonatomic, strong) UIActionSheet *actionSheet;
@property (nonatomic, strong) NSString *HTMLString;
@property (nonatomic, assign) int webViewLoadCount;
@property (nonatomic) MPLogEvent *dwellEvent;
@property (nonatomic) BOOL hasAppeared;

- (void)dismissActionSheet;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdBrowserController

@synthesize webView = _webView;
@synthesize backButton = _backButton;
@synthesize forwardButton = _forwardButton;
@synthesize refreshButton = _refreshButton;
@synthesize safariButton = _safariButton;
@synthesize doneButton = _doneButton;
@synthesize spinnerItem = _spinnerItem;
@synthesize spinner = _spinner;
@synthesize actionSheet = _actionSheet;
@synthesize delegate = _delegate;
@synthesize URL = _URL;
@synthesize webViewLoadCount = _webViewLoadCount;
@synthesize HTMLString = _HTMLString;

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

        self.webView = [[MPWebView alloc] initWithFrame:CGRectZero];
        self.webView.autoresizingMask = UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight;

        self.spinner = [[UIActivityIndicatorView alloc] initWithFrame:CGRectZero];
        [self.spinner sizeToFit];
        self.spinner.hidesWhenStopped = YES;

        self.webViewLoadCount = 0;

        _hasAppeared = NO;
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

    // Track when this view first appears so we can log the time the user stays in the view controller. Creating the event will mark the start of the dwell time.
    // Make sure we don't create the event twice.
    if (!self.hasAppeared) {
        self.dwellEvent = [[MPLogEvent alloc] initWithEventCategory:MPLogEventCategoryAdInteractions eventName:MPLogEventNameClickthroughDwellTime];
    }

    self.hasAppeared = YES;

    NSURL *baseURL = (self.URL != nil) ? self.URL : [NSURL URLWithString:[MPAPIEndpoints baseURL]];

    if (self.HTMLString) {
        [self.webView loadHTMLString:self.HTMLString baseURL:baseURL];
    } else {
        [self.webView loadRequest:[NSURLRequest requestWithURL:self.URL]];
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

    if ([self.delegate respondsToSelector:@selector(adConfiguration)]) {
        MPAdConfiguration *configuration = [self.delegate adConfiguration];

        if (configuration) {
            MPAdConfigurationLogEventProperties *logProperties = [[MPAdConfigurationLogEventProperties alloc] initWithConfiguration:configuration];
            [self.dwellEvent setLogEventProperties:logProperties];
            [self.dwellEvent recordEndTime];

            MPAddLogEvent(self.dwellEvent);
        }
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

#pragma mark -

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

@end
