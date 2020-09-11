//
//  MPEnhancedDeeplinkRequest.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPEnhancedDeeplinkRequest.h"
#import "NSURL+MPAdditions.h"

static NSString * const kRequiredHostname = @"navigate";
static NSString * const kPrimaryURLKey = @"primaryUrl";
static NSString * const kPrimaryTrackingURLKey = @"primaryTrackingUrl";
static NSString * const kFallbackURLKey = @"fallbackUrl";
static NSString * const kFallbackTrackingURLKey = @"fallbackTrackingUrl";

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPEnhancedDeeplinkRequest

- (instancetype)initWithURL:(NSURL *)URL
{
    self = [super init];
    if (self) {
        if (![[[URL host] lowercaseString] isEqualToString:kRequiredHostname]) {
            return nil;
        }

        NSString *primaryURLString = [URL mp_queryParameterForKey:kPrimaryURLKey];
        if (![primaryURLString length]) {
            return nil;
        }
        _primaryURL = [NSURL URLWithString:primaryURLString];
        _originalURL = [URL copy];

        NSMutableArray *primaryTrackingURLs = [NSMutableArray array];
        NSArray *primaryTrackingURLStrings = [URL mp_queryParametersForKey:kPrimaryTrackingURLKey];
        for (NSString *URLString in primaryTrackingURLStrings) {
            [primaryTrackingURLs addObject:[NSURL URLWithString:URLString]];
        }
        _primaryTrackingURLs = [NSArray arrayWithArray:primaryTrackingURLs];

        NSString *fallbackURLString = [URL mp_queryParameterForKey:kFallbackURLKey];
        _fallbackURL = [NSURL URLWithString:fallbackURLString];

        NSMutableArray *fallbackTrackingURLs = [NSMutableArray array];
        NSArray *fallbackTrackingURLStrings = [URL mp_queryParametersForKey:kFallbackTrackingURLKey];
        for (NSString *URLString in fallbackTrackingURLStrings) {
            [fallbackTrackingURLs addObject:[NSURL URLWithString:URLString]];
        }
        _fallbackTrackingURLs = [NSArray arrayWithArray:fallbackTrackingURLs];
    }
    return self;
}

@end
