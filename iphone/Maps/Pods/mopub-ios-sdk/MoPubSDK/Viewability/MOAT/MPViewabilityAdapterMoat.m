//
//  MPViewabilityAdapterMoat.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#if __has_include("MoPub.h")
#import "MPLogging.h"
#endif

#import "MPViewabilityAdapterMoat.h"
#import <WebKit/WebKit.h>

#if __has_include(<MPUBMoatMobileAppKit/MPUBMoatMobileAppKit.h>)
#import <MPUBMoatMobileAppKit/MPUBMoatMobileAppKit.h>
#define __HAS_MOAT_FRAMEWORK_
#endif

#ifdef __HAS_MOAT_FRAMEWORK_
static NSString *const kMOATSendAdStoppedJavascript = @"MoTracker.sendMoatAdStoppedEvent()";
#endif

@interface MPViewabilityAdapterMoat()
@property (nonatomic, readwrite) BOOL isTracking;

#ifdef __HAS_MOAT_FRAMEWORK_
@property (nonatomic, strong) MPUBMoatWebTracker * moatWebTracker;
@property (nonatomic, strong) UIView *webView;
@property (nonatomic, assign) BOOL isVideo;
#endif
@end

@implementation MPViewabilityAdapterMoat

#pragma mark - MPViewabilityAdapter

- (void)startTracking {
#ifdef __HAS_MOAT_FRAMEWORK_
    // Only start tracking if:
    // 1. Moat is not already tracking
    // 2. Moat is allocated
    if (!self.isTracking && self.moatWebTracker != nil) {
        [self.moatWebTracker startTracking];
        self.isTracking = YES;
        MPLogInfo(@"MOAT tracking started");
    }
#endif
}

- (void)stopTracking {
#ifdef __HAS_MOAT_FRAMEWORK_
    // Only stop tracking if:
    // 1. Moat is currently tracking
    if (self.isTracking) {
        void (^moatEndTrackingBlock)(void) = ^{
            [self.moatWebTracker stopTracking];
            if (self.moatWebTracker) {
                MPLogInfo(@"MOAT tracking stopped");
            }
        };
        // If video, as a safeguard, dispatch `AdStopped` event before we stop tracking.
        // (MoTracker makes sure AdStopped is only dispatched once no matter how many times
        // this function is called)
        if (self.isVideo) {
            if ([self.webView isKindOfClass:[WKWebView class]]) {
                WKWebView *typedWebView = (WKWebView *)self.webView;
                [typedWebView evaluateJavaScript:kMOATSendAdStoppedJavascript
                               completionHandler:^(id result, NSError *error){
                                   moatEndTrackingBlock();
                               }];
            } else {
                MPLogInfo(@"Unexpected web view class: %@", self.webView.class);
                moatEndTrackingBlock();
            }
        } else {
            moatEndTrackingBlock();
        }

        // Mark Moat as not tracking
        self.isTracking = NO;
    }
#endif
}

- (void)registerFriendlyObstructionView:(UIView *)view {
    // Nothing to do
}

#pragma mark - MPViewabilityAdapterForWebView

- (instancetype)initWithWebView:(UIView *)webView isVideo:(BOOL)isVideo startTrackingImmediately:(BOOL)startTracking {
    if (self = [super init]) {
        _isTracking = NO;

#ifdef __HAS_MOAT_FRAMEWORK_
        static dispatch_once_t sMoatSharedInstanceStarted;
        dispatch_once(&sMoatSharedInstanceStarted, ^{
            // explicitly disable location tracking and IDFA tracking
            MPUBMoatOptions *options = [[MPUBMoatOptions alloc] init];
            options.locationServicesEnabled = NO;
            options.IDFACollectionEnabled = NO;
            options.debugLoggingEnabled = NO;

            // start with options
            [[MPUBMoatAnalytics sharedInstance] startWithOptions:options];
        });

        _moatWebTracker = [MPUBMoatWebTracker trackerWithWebComponent:webView];
        _webView = webView;
        _isVideo = isVideo;
        if (_moatWebTracker == nil) {
            NSString * adViewClassName = NSStringFromClass([webView class]);
            MPLogError(@"Couldn't attach Moat to %@.", adViewClassName);
        }

        if (startTracking) {
            [_moatWebTracker startTracking];
            _isTracking = YES;
            MPLogInfo(@"MOAT tracking started");
        }
#endif
    }

    return self;
}

@end
