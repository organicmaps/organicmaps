//
//  MPCountdownTimerView.m
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import "MPCountdownTimerView.h"
#import "MPLogging.h"
#import "MPTimer.h"
#import "NSBundle+MPAdditions.h"

// The frequency at which the internal timer is fired in seconds.
// This value matches the step size of the jQuery knob in MPCountdownTimer.html.
static const NSTimeInterval kCountdownTimerInterval = 0.05;

@interface MPCountdownTimerView() <UIWebViewDelegate>
@property (nonatomic, assign, readwrite) BOOL isPaused;

@property (nonatomic, copy) void(^completionBlock)(BOOL);
@property (nonatomic, assign) NSTimeInterval currentSeconds;
@property (nonatomic, strong) MPTimer * timer;
@property (nonatomic, strong) UIWebView * timerView;
@end

@implementation MPCountdownTimerView

#pragma mark - Initialization

- (instancetype)initWithFrame:(CGRect)frame duration:(NSTimeInterval)seconds {
    if (self = [super initWithFrame:frame]) {
        // Duration should be non-negative.
        if (seconds < 0) {
            MPLogDebug(@"Attempted to initialize MPCountdownTimerView with a negative duration. No timer will be created.");
            return nil;
        }

        _completionBlock = nil;
        _currentSeconds = seconds;
        _isPaused = NO;
        _timer = nil;
        _timerView = ({
            UIWebView * view = [[UIWebView alloc] initWithFrame:self.bounds];
            view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            view.backgroundColor = [UIColor clearColor];
            view.delegate = self;
            view.opaque = NO;
            view.userInteractionEnabled = NO;
            view;
        });
        self.userInteractionEnabled = NO;

        [self addSubview:_timerView];
        [_timerView loadHTMLString:self.timerHtml baseURL:self.timerBaseUrl];
    }

    return self;
}

- (void)dealloc {
    // Stop the timer if the view is going away, but do not notify the completion block
    // if there is one.
    _completionBlock = nil;
    [self stopAndSignalCompletion:NO];
}

#pragma mark - Timer

- (BOOL)isActive {
    return (self.timer != nil);
}

- (void)startWithTimerCompletion:(void(^)(BOOL hasElapsed))completion {
    if (self.isActive) {
        return;
    }

    // Reset any internal state that may be dirty from a previous timer run.
    self.isPaused = NO;

    // Copy the completion block and set up the initial state of the timer.
    self.completionBlock = completion;

    // Start the timer now.
    self.timer = [MPTimer timerWithTimeInterval:kCountdownTimerInterval
                                         target:self
                                       selector:@selector(onTimerUpdate:)
                                        repeats:YES];
    [self.timer scheduleNow];

    MPLogInfo(@"MPCountdownTimerView started");
}

- (void)stopAndSignalCompletion:(BOOL)shouldSignalCompletion {
    if (!self.isActive) {
        return;
    }

    // Invalidate and clear the timer to stop it completely.
    [self.timer invalidate];
    self.timer = nil;

    MPLogInfo(@"MPCountdownTimerView stopped");

    // Notify the completion block and clear it out once it's handling has finished.
    if (shouldSignalCompletion && self.completionBlock != nil) {
        BOOL hasElapsed = (self.currentSeconds <= 0);
        self.completionBlock(hasElapsed);

        MPLogInfo(@"MPCountdownTimerView completion block notified");
    }

    // Clear out the completion block since the timer has stopped and it is
    // no longer needed for this instance.
    self.completionBlock = nil;
}

- (void)pause {
    if (!self.isActive) {
        return;
    }

    self.isPaused = [self.timer pause];
    if (self.isPaused) {
        MPLogInfo(@"MPCountdownTimerView paused");
    }
}

- (void)resume {
    if (!self.isActive) {
        return;
    }

    if ([self.timer resume]) {
        self.isPaused = NO;
        MPLogInfo(@"MPCountdownTimerView resumed");
    }
}

#pragma mark - Resources

- (NSString *)timerHtml {
    // Save the contents of the HTML into a static string since it will not change
    // across instances.
    static NSString * html = nil;

    if (html == nil) {
        NSBundle * parentBundle = [NSBundle resourceBundleForClass:self.class];
        NSString * filepath = [parentBundle pathForResource:@"MPCountdownTimer" ofType:@"html"];
        html = [NSString stringWithContentsOfFile:filepath encoding:NSUTF8StringEncoding error:nil];

        if (html == nil) {
            MPLogError(@"Could not find MPCountdownTimer.html in bundle %@", parentBundle.bundlePath);
        }
    }

    return html;
}

- (NSURL *)timerBaseUrl {
    // Save the base URL into a static string since it will not change
    // across instances.
    static NSURL * baseUrl = nil;

    if (baseUrl == nil) {
        NSBundle * parentBundle = [NSBundle resourceBundleForClass:self.class];
        baseUrl = [NSURL fileURLWithPath:parentBundle.bundlePath];
    }

    return baseUrl;
}

#pragma mark - MPTimer

- (void)onTimerUpdate:(MPTimer *)sender {
    // Update the count.
    self.currentSeconds -= kCountdownTimerInterval;

    NSString * jsToEvaluate = [NSString stringWithFormat:@"setCounterValue(%f);", self.currentSeconds];
    [self.timerView stringByEvaluatingJavaScriptFromString:jsToEvaluate];

    // Stop the timer and notify the completion block.
    if (self.currentSeconds <= 0) {
        [self stopAndSignalCompletion:YES];
    }
}

#pragma mark - UIWebViewDelegate

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    NSString * jsToEvaluate = [NSString stringWithFormat:@"setMaxCounterValue(%f); setCounterValue(%f);", self.currentSeconds, self.currentSeconds];
    [self.timerView stringByEvaluatingJavaScriptFromString:jsToEvaluate];
}

@end
