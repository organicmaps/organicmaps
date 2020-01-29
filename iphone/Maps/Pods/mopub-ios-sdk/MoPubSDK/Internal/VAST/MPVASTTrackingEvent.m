//
//  MPVASTTrackingEvent.m
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTTrackingEvent.h"
#import "MPVASTDurationOffset.h"

#pragma mark - MPVideoEvent

// keep this list sorted alphabetically
MPVideoEvent const MPVideoEventClick = @"click";
MPVideoEvent const MPVideoEventCloseLinear = @"closeLinear";
MPVideoEvent const MPVideoEventCollapse = @"collapse";
MPVideoEvent const MPVideoEventComplete = @"complete";
MPVideoEvent const MPVideoEventCreativeView = @"creativeView";
MPVideoEvent const MPVideoEventError = @"error";
MPVideoEvent const MPVideoEventExitFullScreen = @"exitFullscreen";
MPVideoEvent const MPVideoEventExpand = @"expand";
MPVideoEvent const MPVideoEventFirstQuartile = @"firstQuartile";
MPVideoEvent const MPVideoEventFullScreen = @"fullscreen";
MPVideoEvent const MPVideoEventImpression = @"impression";
MPVideoEvent const MPVideoEventMidpoint = @"midpoint";
MPVideoEvent const MPVideoEventMute = @"mute";
MPVideoEvent const MPVideoEventPause = @"pause";
MPVideoEvent const MPVideoEventProgress = @"progress";
MPVideoEvent const MPVideoEventResume = @"resume";
MPVideoEvent const MPVideoEventSkip = @"skip";
MPVideoEvent const MPVideoEventStart = @"start";
MPVideoEvent const MPVideoEventThirdQuartile = @"thirdQuartile";
MPVideoEvent const MPVideoEventUnmute = @"unmute";

@implementation MPVASTTrackingEvent

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
    self = [super initWithDictionary:dictionary];
    if (self) {
        _eventType = dictionary[@"event"];

        _URL = [self generateModelFromDictionaryValue:dictionary
                                        modelProvider:^id(NSDictionary *dictionary) {
                                            return [NSURL URLWithString:[dictionary[@"text"] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
                                        }];
        // a tracker that does not specify a URL is not valid
        if (_URL == nil) {
            return nil;
        }

        _progressOffset = [self generateModelFromDictionaryValue:dictionary
                                                   modelProvider:^id(NSDictionary *dictionary) {
                                                       return [[MPVASTDurationOffset alloc] initWithDictionary:dictionary];
                                                   }];
    }
    return self;
}

- (instancetype)initWithEventType:(MPVideoEvent)eventType
                              url:(NSURL *)url
                   progressOffset:(MPVASTDurationOffset *)progressOffset {
    self = [super init];
    if (self) {
        _eventType = eventType;
        _URL = url;
        _progressOffset = progressOffset;
    }
    return self;
}

@end
