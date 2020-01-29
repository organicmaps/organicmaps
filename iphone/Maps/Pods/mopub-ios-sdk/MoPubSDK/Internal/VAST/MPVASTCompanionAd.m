//
//  MPVASTCompanionAd.m
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTCompanionAd.h"
#import "MPVASTResource.h"
#import "MPVASTStringUtilities.h"
#import "MPVASTTrackingEvent.h"
#import "MPVASTTracking.h"

@implementation MPVASTCompanionAd

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
    self = [super initWithDictionary:dictionary];
    if (self) {
        NSArray *trackingEvents = [self generateModelsFromDictionaryValue:dictionary[@"TrackingEvents"][@"Tracking"]
                                                            modelProvider:^id(NSDictionary *dictionary) {
                                                                return [[MPVASTTrackingEvent alloc] initWithDictionary:dictionary];
                                                            }];
        NSMutableDictionary *eventsDictionary = [NSMutableDictionary dictionary];
        for (MPVASTTrackingEvent *event in trackingEvents) {
            NSMutableArray *events = [eventsDictionary objectForKey:event.eventType];
            if (!events) {
                [eventsDictionary setObject:[NSMutableArray array] forKey:event.eventType];
                events = [eventsDictionary objectForKey:event.eventType];
            }
            [events addObject:event];
        }
        _creativeViewTrackers = eventsDictionary[MPVideoEventCreativeView];
    }
    return self;
}

+ (NSDictionary *)modelMap
{
    return @{@"assetHeight":        @[@"assetHeight", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"assetWidth":         @[@"assetWidth", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"height":             @[@"height", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"width":              @[@"width", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"clickThroughURL":    @[@"CompanionClickThrough.text", MPParseURLFromString()],
             @"clickTrackingURLs":  @[@"CompanionClickTracking.text", MPParseArrayOf(MPParseURLFromString())],
             @"viewTrackingURLs":   @[@"IconViewTracking.text", MPParseArrayOf(MPParseURLFromString())],
             @"identifier":         @"id",
             @"HTMLResources":      @[@"HTMLResource", MPParseArrayOf(MPParseClass([MPVASTResource class]))],
             @"iframeResources":    @[@"IFrameResource", MPParseArrayOf(MPParseClass([MPVASTResource class]))],
             @"staticResources":    @[@"StaticResource", MPParseArrayOf(MPParseClass([MPVASTResource class]))]};
}

- (BOOL)hasResources {
    return (self.staticResources.count > 0
            || self.HTMLResources.count > 0
            || self.iframeResources.count > 0);
}

@end
