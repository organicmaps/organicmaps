//
//  MPVASTTrackingEvent.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTTrackingEvent.h"
#import "MPVASTDurationOffset.h"

NSString * const MPVASTTrackingEventTypeCreativeView = @"creativeView";
NSString * const MPVASTTrackingEventTypeStart = @"start";
NSString * const MPVASTTrackingEventTypeFirstQuartile = @"firstQuartile";
NSString * const MPVASTTrackingEventTypeMidpoint = @"midpoint";
NSString * const MPVASTTrackingEventTypeThirdQuartile = @"thirdQuartile";
NSString * const MPVASTTrackingEventTypeComplete = @"complete";
NSString * const MPVASTTrackingEventTypeMute = @"mute";
NSString * const MPVASTTrackingEventTypeUnmute = @"unmute";
NSString * const MPVASTTrackingEventTypePause = @"pause";
NSString * const MPVASTTrackingEventTypeRewind = @"rewind";
NSString * const MPVASTTrackingEventTypeResume = @"resume";
NSString * const MPVASTTrackingEventTypeFullscreen = @"fullscreen";
NSString * const MPVASTTrackingEventTypeExitFullscreen = @"exitFullscreen";
NSString * const MPVASTTrackingEventTypeExpand = @"expand";
NSString * const MPVASTTrackingEventTypeCollapse = @"collapse";
NSString * const MPVASTTrackingEventTypeAcceptInvitationLinear = @"acceptInvitationLinear";
NSString * const MPVASTTrackingEventTypeCloseLinear = @"closeLinear";
NSString * const MPVASTTrackingEventTypeSkip = @"skip";
NSString * const MPVASTTrackingEventTypeProgress = @"progress";

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
        _progressOffset = [self generateModelFromDictionaryValue:dictionary
                                                   modelProvider:^id(NSDictionary *dictionary) {
                                                       return [[MPVASTDurationOffset alloc] initWithDictionary:dictionary];
                                                   }];
    }
    return self;
}

@end
