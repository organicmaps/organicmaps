//
//  MPVASTTrackingEvent.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTTrackingEvent.h"
#import "MPVASTDurationOffset.h"

#pragma mark - MPVideoEvent

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
