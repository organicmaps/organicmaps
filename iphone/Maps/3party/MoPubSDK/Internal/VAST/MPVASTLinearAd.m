//
//  MPVASTLinearAd.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTLinearAd.h"
#import "MPVASTDurationOffset.h"
#import "MPVASTIndustryIcon.h"
#import "MPVASTMediaFile.h"
#import "MPVASTStringUtilities.h"
#import "MPVASTTrackingEvent.h"

@interface MPVASTLinearAd ()

@property (nonatomic, readwrite) NSArray *clickTrackingURLs;
@property (nonatomic, readwrite) NSArray *customClickURLs;
@property (nonatomic, readwrite) NSArray *industryIcons;
@property (nonatomic, readwrite) NSDictionary *trackingEvents;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPVASTLinearAd

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
        _trackingEvents = eventsDictionary;
    }
    return self;
}

+ (NSDictionary *)modelMap
{
    return @{@"clickThroughURL":    @[@"VideoClicks.ClickThrough.text", MPParseURLFromString()],
             @"clickTrackingURLs":  @[@"VideoClicks.ClickTracking.text", MPParseArrayOf(MPParseURLFromString())],
             @"customClickURLs":    @[@"VideoClicks.CustomClick.text", MPParseArrayOf(MPParseURLFromString())],
             @"duration":           @[@"Duration.text", MPParseTimeIntervalFromDurationString()],
             @"industryIcons":      @[@"Icons.Icon", MPParseArrayOf(MPParseClass([MPVASTIndustryIcon class]))],
             @"mediaFiles":         @[@"MediaFiles.MediaFile", MPParseArrayOf(MPParseClass([MPVASTMediaFile class]))],
             @"skipOffset":         @[@"@self", MPParseClass([MPVASTDurationOffset class])]};
}

@end

@implementation MPVASTLinearAd (Media)

// Static set of supported MIME types for native video.
- (NSSet *)validVideoMimeTypes {
    static dispatch_once_t onceToken;
    static NSSet * validVideoMimeTypes = nil;
    dispatch_once(&onceToken, ^{
        validVideoMimeTypes = [NSSet setWithObjects:@"video/quicktime", @"video/mp4", @"video/3gpp", @"video/3gpp2", @"video/x-m4v", nil];
    });

    return validVideoMimeTypes;
}

// Filters out unsupported media files and selects the highest bitrate video
- (MPVASTMediaFile *)highestBitrateMediaFile {
    NSPredicate * predicate = [NSPredicate predicateWithFormat:@"mimeType IN %@", self.validVideoMimeTypes];
    NSArray * filteredMediaFiles = [self.mediaFiles filteredArrayUsingPredicate:predicate];
    NSArray * sortedMediaFiles = [filteredMediaFiles sortedArrayUsingComparator:^NSComparisonResult(MPVASTMediaFile * a, MPVASTMediaFile * b) {
        return a.bitrate < b.bitrate;
    }];

    return (sortedMediaFiles.count > 0 ? sortedMediaFiles[0] : nil);
}

@end
