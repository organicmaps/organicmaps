//
//  MRProperty.m
//  MoPub
//
//  Created by Andrew He on 12/13/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import "MRProperty.h"
#import <EventKit/EventKit.h>

@implementation MRProperty

- (NSString *)description {
    return @"";
}

- (NSString *)jsonString {
    return @"{}";
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRPlacementTypeProperty : MRProperty

@synthesize placementType = _placementType;

+ (MRPlacementTypeProperty *)propertyWithType:(MRAdViewPlacementType)type {
    MRPlacementTypeProperty *property = [[[self alloc] init] autorelease];
    property.placementType = type;
    return property;
}

- (NSString *)description {
    NSString *placementTypeString = @"unknown";
    switch (_placementType) {
        case MRAdViewPlacementTypeInline: placementTypeString = @"inline"; break;
        case MRAdViewPlacementTypeInterstitial: placementTypeString = @"interstitial"; break;
        default: break;
    }

    return [NSString stringWithFormat:@"placementType: '%@'", placementTypeString];
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRStateProperty

@synthesize state = _state;

+ (MRStateProperty *)propertyWithState:(MRAdViewState)state {
    MRStateProperty *property = [[[self alloc] init] autorelease];
    property.state = state;
    return property;
}

- (NSString *)description {
    NSString *stateString;
    switch (_state) {
        case MRAdViewStateHidden:      stateString = @"hidden"; break;
        case MRAdViewStateDefault:     stateString = @"default"; break;
        case MRAdViewStateExpanded:    stateString = @"expanded"; break;
        default:                       stateString = @"loading"; break;
    }
    return [NSString stringWithFormat:@"state: '%@'", stateString];
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRScreenSizeProperty : MRProperty

@synthesize screenSize = _screenSize;

+ (MRScreenSizeProperty *)propertyWithSize:(CGSize)size {
    MRScreenSizeProperty *property = [[[self alloc] init] autorelease];
    property.screenSize = size;
    return property;
}

- (NSString *)description {
    return [NSString stringWithFormat:@"screenSize: {width: %f, height: %f}",
            _screenSize.width,
            _screenSize.height];
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRSupportsProperty : MRProperty

+ (NSDictionary *)supportedFeatures
{
    BOOL supportsSms = [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"sms://"]];
    BOOL supportsTel = [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"tel://"]];
    BOOL supportsCal = YES;

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
    if ([EKEventStore respondsToSelector:@selector(authorizationStatusForEntityType:)]) {
        supportsCal = [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent] == EKAuthorizationStatusNotDetermined ||
                [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent] == EKAuthorizationStatusAuthorized;
    }
#endif

    return [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:supportsSms], @"sms",
            [NSNumber numberWithBool:supportsTel], @"tel",
            [NSNumber numberWithBool:supportsCal], @"calendar",
            [NSNumber numberWithBool:YES], @"storePicture",
            [NSNumber numberWithBool:YES], @"inlineVideo",
            nil];
}

+ (MRSupportsProperty *)defaultProperty
{
    return [self propertyWithSupportedFeaturesDictionary:[self supportedFeatures]];
}

+ (MRSupportsProperty *)propertyWithSupportedFeaturesDictionary:(NSDictionary *)dictionary
{
    MRSupportsProperty *property = [[[self alloc] init] autorelease];
    property.supportsSms = [[dictionary objectForKey:@"sms"] boolValue];
    property.supportsTel = [[dictionary objectForKey:@"tel"] boolValue];
    property.supportsCalendar = [[dictionary objectForKey:@"calendar"] boolValue];
    property.supportsStorePicture = [[dictionary objectForKey:@"storePicture"] boolValue];
    property.supportsInlineVideo = [[dictionary objectForKey:@"inlineVideo"] boolValue];
    return property;
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"supports: {sms: %@, tel: %@, calendar: %@, storePicture: %@, inlineVideo: %@}",
            [self javascriptBooleanStringFromBoolValue:self.supportsSms],
            [self javascriptBooleanStringFromBoolValue:self.supportsTel],
            [self javascriptBooleanStringFromBoolValue:self.supportsCalendar],
            [self javascriptBooleanStringFromBoolValue:self.supportsStorePicture],
            [self javascriptBooleanStringFromBoolValue:self.supportsInlineVideo]];
}

- (NSString *)javascriptBooleanStringFromBoolValue:(BOOL)value
{
    return value ? @"true" : @"false";
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRViewableProperty : MRProperty

@synthesize isViewable = _isViewable;

+ (MRViewableProperty *)propertyWithViewable:(BOOL)viewable {
    MRViewableProperty *property = [[[self alloc] init] autorelease];
    property.isViewable = viewable;
    return property;
}

- (NSString *)description {
    return [NSString stringWithFormat:@"viewable: '%@'", _isViewable ? @"true" : @"false"];
}

@end
