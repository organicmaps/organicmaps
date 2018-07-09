//
//  MPVideoConfig.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVideoConfig.h"
#import "MPLogging.h"
#import "MPVASTStringUtilities.h"

@interface MPVideoPlaybackCandidate : NSObject

@property (nonatomic, readwrite) MPVASTLinearAd *linearAd;
@property (nonatomic, readwrite) NSArray *errorURLs;
@property (nonatomic, readwrite) NSArray *impressionURLs;
@property (nonatomic, readwrite) NSTimeInterval minimumViewabilityTimeInterval;
@property (nonatomic, readwrite) double minimumFractionOfVideoVisible;
@property (nonatomic, readwrite) NSURL *viewabilityTrackingURL;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPVideoPlaybackCandidate

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPVideoConfig ()

@property (nonatomic, readwrite) NSURL *mediaURL;
@property (nonatomic, readwrite) NSURL *clickThroughURL;
@property (nonatomic, readwrite) NSArray *clickTrackingURLs;
@property (nonatomic, readwrite) NSArray *errorURLs;
@property (nonatomic, readwrite) NSArray *impressionURLs;
@property (nonatomic, readwrite) NSArray *startTrackers;
@property (nonatomic, readwrite) NSArray *firstQuartileTrackers;
@property (nonatomic, readwrite) NSArray *midpointTrackers;
@property (nonatomic, readwrite) NSArray *thirdQuartileTrackers;
@property (nonatomic, readwrite) NSArray *completionTrackers;
@property (nonatomic, readwrite) NSArray *muteTrackers;
@property (nonatomic, readwrite) NSArray *unmuteTrackers;
@property (nonatomic, readwrite) NSArray *pauseTrackers;
@property (nonatomic, readwrite) NSArray *rewindTrackers;
@property (nonatomic, readwrite) NSArray *resumeTrackers;
@property (nonatomic, readwrite) NSArray *fullscreenTrackers;
@property (nonatomic, readwrite) NSArray *exitFullscreenTrackers;
@property (nonatomic, readwrite) NSArray *expandTrackers;
@property (nonatomic, readwrite) NSArray *collapseTrackers;
@property (nonatomic, readwrite) NSArray *acceptInvitationLinearTrackers;
@property (nonatomic, readwrite) NSArray *closeLinearTrackers;
@property (nonatomic, readwrite) NSArray *skipTrackers;
@property (nonatomic, readwrite) NSArray *otherProgressTrackers;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPVASTLinearAd (MPVideoConfig)

@property (nonatomic, readwrite) NSArray *clickTrackingURLs;
@property (nonatomic, readwrite) NSArray *customClickURLs;
@property (nonatomic, readwrite) NSArray *industryIcons;
@property (nonatomic, readwrite) NSDictionary *trackingEvents;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPVideoConfig

- (instancetype)initWithVASTResponse:(MPVASTResponse *)response additionalTrackers:(NSDictionary *)additionalTrackers
{
    self = [super init];
    if (self) {
        [self commonInit:response additionalTrackers:additionalTrackers];
    }
    return self;
}

- (void)commonInit:(MPVASTResponse *)response additionalTrackers:(NSDictionary *)additionalTrackers
{
    NSArray *candidates = [self playbackCandidatesFromVASTResponse:response];

    if (candidates.count == 0) {
        return;
    }

    MPVideoPlaybackCandidate *candidate = candidates[0];
    MPVASTMediaFile *mediaFile = candidate.linearAd.highestBitrateMediaFile;

    _mediaURL = mediaFile.URL;
    _clickThroughURL = candidate.linearAd.clickThroughURL;
    _clickTrackingURLs = candidate.linearAd.clickTrackingURLs;
    _errorURLs = candidate.errorURLs;
    _impressionURLs = candidate.impressionURLs;

    NSDictionary *trackingEvents = candidate.linearAd.trackingEvents;
    _creativeViewTrackers = trackingEvents[MPVASTTrackingEventTypeCreativeView];
    _startTrackers = [self trackersByMergingOriginalTrackers:trackingEvents additionalTrackers:additionalTrackers name:MPVASTTrackingEventTypeStart];
    _firstQuartileTrackers = [self trackersByMergingOriginalTrackers:trackingEvents additionalTrackers:additionalTrackers name:MPVASTTrackingEventTypeFirstQuartile];
    _midpointTrackers = [self trackersByMergingOriginalTrackers:trackingEvents additionalTrackers:additionalTrackers name:MPVASTTrackingEventTypeMidpoint];
    _thirdQuartileTrackers = [self trackersByMergingOriginalTrackers:trackingEvents additionalTrackers:additionalTrackers name:MPVASTTrackingEventTypeThirdQuartile];
    _completionTrackers = [self trackersByMergingOriginalTrackers:trackingEvents additionalTrackers:additionalTrackers name:MPVASTTrackingEventTypeComplete];
    _muteTrackers = trackingEvents[MPVASTTrackingEventTypeMute];
    _unmuteTrackers = trackingEvents[MPVASTTrackingEventTypeUnmute];
    _pauseTrackers = trackingEvents[MPVASTTrackingEventTypePause];
    _rewindTrackers = trackingEvents[MPVASTTrackingEventTypeRewind];
    _resumeTrackers = trackingEvents[MPVASTTrackingEventTypeResume];
    _fullscreenTrackers = trackingEvents[MPVASTTrackingEventTypeFullscreen];
    _exitFullscreenTrackers = trackingEvents[MPVASTTrackingEventTypeExitFullscreen];
    _expandTrackers = trackingEvents[MPVASTTrackingEventTypeExpand];
    _collapseTrackers = trackingEvents[MPVASTTrackingEventTypeCollapse];
    _acceptInvitationLinearTrackers = trackingEvents[MPVASTTrackingEventTypeAcceptInvitationLinear];
    _closeLinearTrackers = trackingEvents[MPVASTTrackingEventTypeCloseLinear];
    _skipTrackers = trackingEvents[MPVASTTrackingEventTypeSkip];
    _otherProgressTrackers = trackingEvents[MPVASTTrackingEventTypeProgress];

    _minimumViewabilityTimeInterval = candidate.minimumViewabilityTimeInterval;
    _minimumFractionOfVideoVisible = candidate.minimumFractionOfVideoVisible;
    _viewabilityTrackingURL = candidate.viewabilityTrackingURL;
}

- (NSArray *)playbackCandidatesFromVASTResponse:(MPVASTResponse *)response
{
    NSMutableArray *candidates = [NSMutableArray array];

    for (MPVASTAd *ad in response.ads) {
        if (ad.inlineAd) {
            MPVASTInline *inlineAd = ad.inlineAd;
            NSArray *creatives = inlineAd.creatives;
            for (MPVASTCreative *creative in creatives) {
                if (creative.linearAd && [creative.linearAd.mediaFiles count]) {
                    MPVideoPlaybackCandidate *candidate = [[MPVideoPlaybackCandidate alloc] init];
                    candidate.linearAd = creative.linearAd;
                    candidate.errorURLs = inlineAd.errorURLs;
                    candidate.impressionURLs = inlineAd.impressionURLs;

                    NSDictionary *viewabilityExt = [self extensionFromInlineAd:inlineAd forKey:@"MoPubViewabilityTracker"];
                    if (viewabilityExt) {
                        NSURL *viewabilityTrackingURL = [NSURL URLWithString:viewabilityExt[@"text"]];
                        BOOL valid = [MPVASTStringUtilities stringRepresentsNonNegativeDuration:viewabilityExt[@"viewablePlaytime"]]&&
                            [MPVASTStringUtilities stringRepresentsNonNegativePercentage:viewabilityExt[@"percentViewable"]] &&
                            viewabilityTrackingURL;

                        if (valid) {
                            candidate.minimumViewabilityTimeInterval = [MPVASTStringUtilities timeIntervalFromString:viewabilityExt[@"viewablePlaytime"]];
                            candidate.minimumFractionOfVideoVisible = [MPVASTStringUtilities percentageFromString:viewabilityExt[@"percentViewable"]] / 100.0;
                            candidate.viewabilityTrackingURL = viewabilityTrackingURL;
                        }
                    }

                    [candidates addObject:candidate];
                }
            }
        } else if (ad.wrapper) {
            NSArray *candidatesFromWrapper = [self playbackCandidatesFromVASTResponse:ad.wrapper.wrappedVASTResponse];

            // Merge any wrapper-level tracking URLs into each of the candidates.
            for (MPVideoPlaybackCandidate *candidate in candidatesFromWrapper) {
                candidate.errorURLs = [candidate.errorURLs arrayByAddingObjectsFromArray:ad.wrapper.errorURLs];
                candidate.impressionURLs = [candidate.impressionURLs arrayByAddingObjectsFromArray:ad.wrapper.impressionURLs];

                candidate.linearAd.trackingEvents = [self dictionaryByMergingTrackingDictionaries:@[candidate.linearAd.trackingEvents, [self trackingEventsFromWrapper:ad.wrapper]]];
                candidate.linearAd.clickTrackingURLs = [candidate.linearAd.clickTrackingURLs arrayByAddingObjectsFromArray:[self clickTrackingURLsFromWrapper:ad.wrapper]];
                candidate.linearAd.customClickURLs = [candidate.linearAd.customClickURLs arrayByAddingObjectsFromArray:[self customClickURLsFromWrapper:ad.wrapper]];
                candidate.linearAd.industryIcons = [candidate.linearAd.industryIcons arrayByAddingObjectsFromArray:[self industryIconsFromWrapper:ad.wrapper]];
            }

            [candidates addObjectsFromArray:candidatesFromWrapper];
        }
    }

    return candidates;
}

- (NSDictionary *)trackingEventsFromWrapper:(MPVASTWrapper *)wrapper
{
    NSMutableArray *trackingEventDictionaries = [NSMutableArray array];

    for (MPVASTCreative *creative in wrapper.creatives) {
        [trackingEventDictionaries addObject:creative.linearAd.trackingEvents];
    }

    return [self dictionaryByMergingTrackingDictionaries:trackingEventDictionaries];
}

- (NSArray *)clickTrackingURLsFromWrapper:(MPVASTWrapper *)wrapper
{
    NSMutableArray *clickTrackingURLs = [NSMutableArray array];
    for (MPVASTCreative *creative in wrapper.creatives) {
        [clickTrackingURLs addObjectsFromArray:creative.linearAd.clickTrackingURLs];
    }

    return clickTrackingURLs;
}

- (NSArray *)customClickURLsFromWrapper:(MPVASTWrapper *)wrapper
{
    NSMutableArray *customClickURLs = [NSMutableArray array];
    for (MPVASTCreative *creative in wrapper.creatives) {
        [customClickURLs addObjectsFromArray:creative.linearAd.customClickURLs];
    }

    return customClickURLs;
}

- (NSArray *)industryIconsFromWrapper:(MPVASTWrapper *)wrapper
{
    NSMutableArray *industryIcons = [NSMutableArray array];
    for (MPVASTCreative *creative in wrapper.creatives) {
        [industryIcons addObjectsFromArray:creative.linearAd.industryIcons];
    }

    return industryIcons;
}

- (NSDictionary *)extensionFromInlineAd:(MPVASTInline *)inlineAd forKey:(NSString *)key
{
    NSDictionary *extensions = inlineAd.extensions;
    id extensionObject = [extensions objectForKey:@"Extension"];

    if ([extensionObject isKindOfClass:[NSDictionary class]]) {
        // Case 1: "Extensions" element with only one "Extension" child.
        NSDictionary *extensionChildNode = extensionObject;
        id extension = [self firstObjectForKey:key inDictionary:extensionChildNode];
        if ([extension isKindOfClass:[NSDictionary class]]) {
            return extension;
        }
    } else if ([extensionObject isKindOfClass:[NSArray class]]) {
        // Case 2: "Extensions" element with multiple "Extension" children.
        NSArray *extensionChildNodes = extensionObject;
        for (id node in extensionChildNodes) {
            if (![node isKindOfClass:[NSDictionary class]]) {
                continue;
            }

            id extension = [self firstObjectForKey:key inDictionary:node];
            if ([extension isKindOfClass:[NSDictionary class]]) {
                return extension;
            }
        }
    }

    return nil;
}

// When dealing with VAST, we will often have dictionaries where a key can map either to a single
// value or an array of values. For example, the dictionary containing VAST extensions might contain
// one or more <Extension> nodes. This method is useful when we simply want the first value matching
// a given key. It is equivalent to calling [dictionary objectForKey:key] when the key maps to a
// single value. When the key maps to an NSArray, it returns the first value in the array.
- (id)firstObjectForKey:(NSString *)key inDictionary:(NSDictionary *)dictionary
{
    id value = [dictionary objectForKey:key];
    if ([value isKindOfClass:[NSArray class]]) {
        return [value firstObject];
    } else {
        return value;
    }
}

- (NSArray *)trackersByMergingOriginalTrackers:(NSDictionary *)originalTrackers additionalTrackers:(NSDictionary *)additionalTrackers name:(NSString *)trackerName
{
    if (![originalTrackers[trackerName] isKindOfClass:[NSArray class]]) {
        return additionalTrackers[trackerName];
    }
    if (![additionalTrackers[trackerName] isKindOfClass:[NSArray class]]) {
        return originalTrackers[trackerName];
    }
    NSMutableArray *mergedTrackers = [NSMutableArray new];
    [mergedTrackers addObjectsFromArray:originalTrackers[trackerName]];
    [mergedTrackers addObjectsFromArray:additionalTrackers[trackerName]];
    return mergedTrackers;
}

- (NSDictionary *)dictionaryByMergingTrackingDictionaries:(NSArray *)dictionaries
{
    NSMutableDictionary *mergedDictionary = [NSMutableDictionary dictionary];
    for (NSDictionary *dictionary in dictionaries) {
        for (NSString *key in [dictionary allKeys]) {
            if ([dictionary[key] isKindOfClass:[NSArray class]]) {
                if (!mergedDictionary[key]) {
                    mergedDictionary[key] = [NSMutableArray array];
                }

                [mergedDictionary[key] addObjectsFromArray:dictionary[key]];
            } else {
                MPLogError(@"TrackingEvents dictionary expected an array object for key '%@' "
                           @"but got an instance of %@ instead.",
                           key, NSStringFromClass([dictionary[key] class]));
            }
        }
    }
    return mergedDictionary;
}

@end
