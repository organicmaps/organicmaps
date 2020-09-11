//
//  MPViewabilityTracker.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MoPub.h"
#import "MPLogging.h"
#import "MPViewabilityAdapter.h"
#import "MPViewabilityTracker.h"
#import "MPWebView+Viewability.h"

/**
 * Macro that queries if a bitmask value is currently in the given bitmask
 * Examples:
 *   bitmask = 0x0110
 *   value_1 = 0x0001
 *   value_2 = 0x0100
 *   value_3 = 0x0111
 *
 *   OptionsHasValue(bitmask, value_1)   // returns NO
 *   OptionsHasValue(bitmask, value_2)   // returns YES
 *   OptionsHasValue(bitmask, value_3)   // returns NO
 */
#define OptionsHasValue(options, value) (((options) & (value)) == (value))

static MPViewabilityOption sEnabledViewabilityVendors = 0;
static NSDictionary * sSupportedAdapters = nil;
NSString *const kDisableViewabilityTrackerNotification = @"com.mopub.mopub-ios-sdk.viewability.disabletracking";
NSString *const kDisabledViewabilityTrackers = @"disableViewabilityTrackers";

@interface MPViewabilityTracker()
// Map of `MPViewabilityOption`: `id<MPViewabilityAdapter>`
@property (nonatomic, strong) NSDictionary<NSNumber *, id<MPViewabilityAdapter>> * trackers;
@end

@implementation MPViewabilityTracker

+ (void)initialize {
    if (self == [MPViewabilityTracker class]) {
        // Initialize the current mapping of viewability vendors to their
        // associated adapter class names.
        // This map should be updated when there are changes to `MPViewabilityOption`.
        sSupportedAdapters = @{ @(MPViewabilityOptionMoat): @"MPViewabilityAdapterMoat",
                                @(MPViewabilityOptionIAS): @"MPViewabilityAdapterAvid",
                              };

        // Initial population of the enabled viewability vendors.
        for (NSInteger index = 1; index < MPViewabilityOptionAll; index = index << 1) {
            NSString * adapterClassName = sSupportedAdapters[@(index)];
            if (NSClassFromString(adapterClassName)) {
                sEnabledViewabilityVendors |= index;
                MPLogInfo(@"%@ was found.", adapterClassName);
            }
        }
    }
}

- (instancetype)initWithWebView:(MPWebView *)webView
                        isVideo:(BOOL)isVideo
       startTrackingImmediately:(BOOL)startTracking {
    if (self = [super init]) {
        // While the viewability SDKs have features that allow the developer to pass in a container view, WKWebView is
        // not always in MPWebView's view hierarchy. Pass in the contained web view to be safe, as we don't know for
        // sure *how* or *when* MPWebView is traversed.
        UIView *view = webView.containedWebView;

        // Invalid ad view
        if (view == nil) {
            MPLogInfo(@"nil ad view passed into %s", __PRETTY_FUNCTION__);
            return nil;
        }

        // Register handler for disabling of viewability tracking.
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onDisableViewabilityTrackingForNotification:) name:kDisableViewabilityTrackerNotification object:nil];

        // Initialize all known and enabled viewability trackers.
        NSMutableDictionary<NSNumber *, id<MPViewabilityAdapter>> * trackers = [NSMutableDictionary dictionary];
        for (NSInteger index = 1; index < MPViewabilityOptionAll; index = index << 1) {
            NSString * className = sSupportedAdapters[@(index)];
            id<MPViewabilityAdapter> tracker = [self initializeTrackerWithClassName:className
                                                               forViewabilityOption:index
                                                                         withAdView:view
                                                                            isVideo:isVideo
                                                           startTrackingImmediately:startTracking];
            if (tracker != nil) {
                trackers[@(index)] = tracker;
            }
        }

        _trackers = trackers;
    }

    return self;
}

- (id<MPViewabilityAdapter>)initializeTrackerWithClassName:(NSString *)className
                                      forViewabilityOption:(MPViewabilityOption)option
                                                withAdView:(UIView *)webView
                                                   isVideo:(BOOL)isVideo
                                  startTrackingImmediately:(BOOL)startTracking {
    // Ignore invalid options and empty class name
    if (option == MPViewabilityOptionNone || option == MPViewabilityOptionAll || className.length == 0) {
        return nil;
    }

    // Check if the tracker class exists in the runtime and if it is enabled before
    // attempting to initialize it.
    Class adapterClass = NSClassFromString(className);
    if (adapterClass
        && [adapterClass conformsToProtocol:@protocol(MPViewabilityAdapterForWebView)]
        && OptionsHasValue(sEnabledViewabilityVendors, option)) {
        id<MPViewabilityAdapter> tracker = [[adapterClass alloc]
                                            initWithWebView:webView
                                            isVideo:isVideo
                                            startTrackingImmediately:startTracking];
        return tracker;
    }

    return nil;
}

- (instancetype)initWithNativeVideoView:(UIView *)nativeVideoView
               startTrackingImmediately:(BOOL)startTracking {
    if (self = [super init]) {
        if (nativeVideoView == nil) {
            MPLogInfo(@"nil ad view passed into %s", __PRETTY_FUNCTION__);
            return nil;
        }

        // Register handler for disabling of viewability tracking.
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(onDisableViewabilityTrackingForNotification:)
                                                     name:kDisableViewabilityTrackerNotification
                                                   object:nil];

        // Initialize all known and enabled viewability trackers.
        NSMutableDictionary<NSNumber *, id<MPViewabilityAdapter>> * trackers = [NSMutableDictionary dictionary];
        for (NSInteger index = 1; index < MPViewabilityOptionAll; index = index << 1) {
            NSString *className = sSupportedAdapters[@(index)];
            id<MPViewabilityAdapter> tracker = [self initializeTrackerWithClassName:className
                                                               forViewabilityOption:index
                                                                withNativeVideoView:nativeVideoView
                                                           startTrackingImmediately:startTracking];
            if (tracker != nil) {
                trackers[@(index)] = tracker;
            }
        }

        _trackers = trackers;
    }

    return self;
}

- (id<MPViewabilityAdapter>)initializeTrackerWithClassName:(NSString *)className
                                      forViewabilityOption:(MPViewabilityOption)option
                                       withNativeVideoView:(UIView *)nativeVideoView
                                  startTrackingImmediately:(BOOL)startTracking {
    // Ignore invalid options and empty class name
    if (option == MPViewabilityOptionNone || option == MPViewabilityOptionAll || className.length == 0) {
        return nil;
    }

    // Check if the tracker class exists in the runtime and if it is enabled before init attempt
    Class adapterClass = NSClassFromString(className);
    if (adapterClass
        && [adapterClass conformsToProtocol:@protocol(MPViewabilityAdapterForNativeVideoView)]
        && OptionsHasValue(sEnabledViewabilityVendors, option)) {
        id<MPViewabilityAdapter> tracker = [[adapterClass alloc]
                                            initWithNativeVideoView:nativeVideoView
                                            startTrackingImmediately:startTracking];
        return tracker;
    }

    return nil;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self stopTracking];
}

- (void)startTracking {
    [self.trackers.allValues makeObjectsPerformSelector:@selector(startTracking)];
}

- (void)stopTracking:(MPViewabilityOption)vendors {
    for (NSInteger index = 1; index < MPViewabilityOptionAll; index = index << 1) {
        if (OptionsHasValue(vendors, index) && self.trackers[@(index)] != nil) {
            id<MPViewabilityAdapter> tracker = self.trackers[@(index)];
            [tracker stopTracking];
        }
    }
}

- (void)stopTracking {
    [self stopTracking:MPViewabilityOptionAll];
}

- (void)registerFriendlyObstructionView:(UIView *)view {
    [self.trackers.allValues makeObjectsPerformSelector:@selector(registerFriendlyObstructionView:) withObject:view];
}

- (void)trackNativeVideoEvent:(MPVideoEvent)event eventInfo:(NSDictionary<NSString *, id> *)eventInfo {
    for (id<MPViewabilityAdapter> tracker in self.trackers.allValues) {
        if ([tracker conformsToProtocol:@protocol(MPViewabilityAdapterForNativeVideoView)]) {
            [((id<MPViewabilityAdapterForNativeVideoView>)tracker) trackNativeVideoEvent:event
                                                                               eventInfo:eventInfo];
        }
    }
}

+ (MPViewabilityOption)enabledViewabilityVendors {
    return sEnabledViewabilityVendors;
}

+ (void)disableViewability:(MPViewabilityOption)vendors {
    // Keep around the old viewability bitmask for comparing if the
    // state has changed.
    MPViewabilityOption oldEnabledVendors = sEnabledViewabilityVendors;

    // Disable specified vendors
    for (NSInteger index = 1; index < MPViewabilityOptionAll; index = index << 1) {
        if (OptionsHasValue(vendors, index)) {
            sEnabledViewabilityVendors &= ~index;
        }
    }

    // Broadcast that some viewability tracking has been disabled.
    if (vendors != MPViewabilityOptionNone && oldEnabledVendors != sEnabledViewabilityVendors) {
        [[NSNotificationCenter defaultCenter] postNotificationName:kDisableViewabilityTrackerNotification object:nil userInfo:@{kDisabledViewabilityTrackers: @(vendors)}];
    }
}

#pragma mark - Notification Handlers

- (void)onDisableViewabilityTrackingForNotification:(NSNotification *)notification {
    MPViewabilityOption disabledTrackers = MPViewabilityOptionNone;
    if (notification.userInfo != nil && [notification.userInfo objectForKey:kDisabledViewabilityTrackers] != nil) {
        disabledTrackers = (MPViewabilityOption)[[notification.userInfo objectForKey:kDisabledViewabilityTrackers] integerValue];
    }

    // Immediately stop all tracking for the disabled viewability vendors.
    [self stopTracking:disabledTrackers];

    // Remove the disabled trackers
    NSMutableDictionary<NSNumber *, id<MPViewabilityAdapter>> * updatedTrackers = [self.trackers mutableCopy];
    for (NSInteger index = 1; index < MPViewabilityOptionAll; index = index << 1) {
        if (OptionsHasValue(disabledTrackers, index) && self.trackers[@(index)] != nil) {
            [updatedTrackers removeObjectForKey:@(index)];
        }
    }
    self.trackers = updatedTrackers;
}

@end
