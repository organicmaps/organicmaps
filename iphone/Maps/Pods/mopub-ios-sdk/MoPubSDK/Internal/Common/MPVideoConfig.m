//
//  MPVideoConfig.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVideoConfig.h"
#import "MPLogging.h"
#import "MPVASTStringUtilities.h"
#import "MPVASTCompanionAd.h"
#import "MPVASTConstant.h"
#import "MPVASTTracking.h"

/**
 This is a private data object that represents an ad candidate for display.
 */
@interface MPVideoPlaybackCandidate : NSObject

@property (nonatomic, strong) MPVASTLinearAd *linearAd;
@property (nonatomic, strong) NSArray<NSURL *> *errorURLs;
@property (nonatomic, strong) NSArray<NSURL *> *impressionURLs;
@property (nonatomic, strong) MPVASTDurationOffset *skipOffset;
@property (nonatomic, strong) NSString *callToActionButtonTitle;
@property (nonatomic, strong) NSArray<MPVASTCompanionAd *> *companionAds;

@end

@implementation MPVideoPlaybackCandidate
@end // this data object should have empty implementation

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPVASTLinearAd (MPVideoConfig)

@property (nonatomic, strong) NSArray *clickTrackingURLs;
@property (nonatomic, strong) NSArray *customClickURLs;
@property (nonatomic, strong) NSArray *industryIcons;
@property (nonatomic, strong) NSDictionary *trackingEvents;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPVideoConfig ()
@property (nonatomic, strong) MPVASTDurationOffset *skipOffset;
@property (nonatomic, strong) NSArray<MPVASTCompanionAd *> *companionAds;
@property (nonatomic, strong) NSDictionary<MPVideoEvent, NSArray<MPVASTTrackingEvent *> *> *trackingEventTable;
@end

@implementation MPVideoConfig

#pragma mark - Public

- (MPVASTDurationOffset *)skipOffset {
    // If the video is rewarded, do not use the skip offset for countdown timer purposes
    if (self.isRewarded) {
        return nil;
    } else {
        return _skipOffset;
    }
}

- (instancetype)initWithVASTResponse:(MPVASTResponse *)response additionalTrackers:(NSDictionary *)additionalTrackers
{
    self = [super init];
    if (self) {
        [self commonInit:response additionalTrackers:additionalTrackers];
    }
    return self;
}

- (NSArray<MPVASTTrackingEvent *> *)trackingEventsForKey:(MPVideoEvent)key {
    return self.trackingEventTable[key];
}

#pragma mark - Private

- (void)commonInit:(MPVASTResponse *)response additionalTrackers:(NSDictionary *)additionalTrackers
{
    NSArray<MPVideoPlaybackCandidate *> *candidates = [self playbackCandidatesFromVASTResponse:response];

    if (candidates.count == 0) {
        return;
    }

    MPVideoPlaybackCandidate *candidate = candidates[0];

    // obtain from linear ad
    _mediaFiles = candidate.linearAd.mediaFiles;
    _clickThroughURL = candidate.linearAd.clickThroughURL;
    _industryIcons = candidate.linearAd.industryIcons;

    _skipOffset = candidate.skipOffset;
    _companionAds = candidate.companionAds;

    if (candidate.callToActionButtonTitle.length > 0) {
        _callToActionButtonTitle = candidate.callToActionButtonTitle;
    } else {
        _callToActionButtonTitle = kVASTDefaultCallToActionButtonTitle;
    }

    // set up the tracking event table
    NSMutableDictionary<MPVideoEvent, NSArray<MPVASTTrackingEvent *> *> *table
    = [NSMutableDictionary dictionaryWithDictionary:candidate.linearAd.trackingEvents];
    for (MPVideoEvent name in @[MPVideoEventStart,
                                MPVideoEventFirstQuartile,
                                MPVideoEventMidpoint,
                                MPVideoEventThirdQuartile,
                                MPVideoEventComplete]) {
        table[name] = [self mergeTrackersOfName:name
                               originalTrackers:table
                             additionalTrackers:additionalTrackers];
    }

    NSMutableDictionary<MPVideoEvent, NSArray<NSURL *> *> *eventVsURLs = [NSMutableDictionary new];
    if (candidate.linearAd.clickTrackingURLs.count > 0) {
        eventVsURLs[MPVideoEventClick] = candidate.linearAd.clickTrackingURLs;
    }
    if (candidate.errorURLs.count > 0) {
        eventVsURLs[MPVideoEventError] = candidate.errorURLs;
    }
    if (candidate.impressionURLs.count > 0) {
        eventVsURLs[MPVideoEventImpression] = candidate.impressionURLs;
    }

    for (MPVideoEvent event in eventVsURLs.allKeys) {
        NSMutableArray<MPVASTTrackingEvent *> *trackingEvents = [NSMutableArray new];
        for (NSURL *url in eventVsURLs[event]) {
            [trackingEvents addObject:[[MPVASTTrackingEvent alloc] initWithEventType:event
                                                                                 url:url
                                                                      progressOffset:nil]];
        }
        table[event] = trackingEvents;
    }

    self.trackingEventTable = [NSDictionary dictionaryWithDictionary:table];
}

- (NSArray<MPVideoPlaybackCandidate *> *)playbackCandidatesFromVASTResponse:(MPVASTResponse *)response
{
    NSMutableArray<MPVideoPlaybackCandidate *> *candidates = [NSMutableArray array];

    for (MPVASTAd *ad in response.ads) {
        if (ad.inlineAd) {
            MPVASTInline *inlineAd = ad.inlineAd;
            MPVideoPlaybackCandidate *candidate = [[MPVideoPlaybackCandidate alloc] init];
            candidate.callToActionButtonTitle = [self extensionFromInlineAd:inlineAd forKey:kVASTMoPubCTATextKey][kVASTAdTextKey];

            for (MPVASTCreative *creative in inlineAd.creatives) {
                if (creative.linearAd && [creative.linearAd.mediaFiles count]) {
                    candidate.linearAd = creative.linearAd;
                    candidate.skipOffset = creative.linearAd.skipOffset;
                    candidate.errorURLs = inlineAd.errorURLs;
                    candidate.impressionURLs = inlineAd.impressionURLs;
                    [candidates addObject:candidate];
                } else if (creative.companionAds.count > 0) {
                    NSMutableArray<MPVASTCompanionAd *> *companionAds = [NSMutableArray new];
                    for (MPVASTCompanionAd *companionAd in creative.companionAds) {
                        if (companionAd.resourceToDisplay != nil) { // cannot display ad without any resource
                            [companionAds addObject:companionAd];
                        }
                    }
                    candidate.companionAds = [NSArray arrayWithArray:companionAds];
                }
            }
        } else if (ad.wrapper) {
            NSArray<MPVideoPlaybackCandidate *> *candidatesFromWrapper = [self playbackCandidatesFromVASTResponse:ad.wrapper.wrappedVASTResponse];

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

- (NSArray<MPVASTTrackingEvent *> *)mergeTrackersOfName:(NSString *)trackerName
                                       originalTrackers:(NSDictionary<NSString *, NSArray<MPVASTTrackingEvent *> *> *)originalTrackers
                                     additionalTrackers:(NSDictionary<NSString *, NSArray<MPVASTTrackingEvent *> *> *)additionalTrackers {
    NSArray<MPVASTTrackingEvent *> *original = originalTrackers[trackerName];
    NSArray<MPVASTTrackingEvent *> *additional = additionalTrackers[trackerName];
    if (original == nil || [original isKindOfClass:[NSArray class]] == false) {
        original = @[];
    }
    if ([additional isKindOfClass:[NSArray class]] == false) {
        return original;
    }
    return [original arrayByAddingObjectsFromArray:additional];
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
                MPLogInfo(@"TrackingEvents dictionary expected an array object for key '%@' "
                           @"but got an instance of %@ instead.",
                           key, NSStringFromClass([dictionary[key] class]));
            }
        }
    }
    return mergedDictionary;
}

@end

#pragma mark - MPVASTCompanionAdProvider

@implementation MPVideoConfig (MPVASTCompanionAdProvider)

- (BOOL)hasCompanionAd {
    return self.companionAds.count > 0;
}

- (MPVASTCompanionAd *)companionAdForContainerSize:(CGSize)containerSize {
    return [MPVASTCompanionAd bestCompanionAdForCandidates:self.companionAds
                                             containerSize:containerSize];
}

@end
