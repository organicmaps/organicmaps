//
//  MRCommand.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MRCommand.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MRConstants.h"

@implementation MRCommand

+ (void)initialize {
    if (self == [MRCommand self]) {
        // Register command classes
        [self registerCommand:[MRCloseCommand self]];
        [self registerCommand:[MRExpandCommand self]];
        [self registerCommand:[MRResizeCommand self]];
        [self registerCommand:[MRUseCustomCloseCommand self]];
        [self registerCommand:[MRSetOrientationPropertiesCommand self]];
        [self registerCommand:[MROpenCommand self]];
        [self registerCommand:[MRPlayVideoCommand self]];
    }
}

+ (NSMutableDictionary *)sharedCommandClassMap
{
    static NSMutableDictionary *sharedMap = nil;

    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sharedMap = [[NSMutableDictionary alloc] init];
    });

    return sharedMap;
}

+ (void)registerCommand:(Class)commandClass
{
    NSMutableDictionary *map = [self sharedCommandClassMap];
    @synchronized(self) {
        [map setValue:commandClass forKey:[commandClass commandType]];
    }
}

+ (NSString *)commandType
{
    return @"BASE_CMD_TYPE";
}

+ (Class)commandClassForString:(NSString *)string
{
    NSMutableDictionary *map = [self sharedCommandClassMap];
    @synchronized(self) {
        return [map objectForKey:string];
    }
}

+ (id)commandForString:(NSString *)string
{
    Class commandClass = [self commandClassForString:string];
    return [[commandClass alloc] init];
}

// return YES by default for user safety
- (BOOL)requiresUserInteractionForPlacementType:(NSUInteger)placementType
{
    return YES;
}

// Default to NO to avoid race conditions.
- (BOOL)executableWhileBlockingRequests
{
    return NO;
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    return YES;
}

- (CGFloat)floatFromParameters:(NSDictionary *)parameters forKey:(NSString *)key
{
    return [self floatFromParameters:parameters forKey:key withDefault:0.0];
}

- (CGFloat)floatFromParameters:(NSDictionary *)parameters forKey:(NSString *)key withDefault:(CGFloat)defaultValue
{
    NSString *stringValue = [parameters valueForKey:key];
    return stringValue ? [stringValue floatValue] : defaultValue;
}

- (BOOL)boolFromParameters:(NSDictionary *)parameters forKey:(NSString *)key
{
    NSString *stringValue = [parameters valueForKey:key];
    return [stringValue isEqualToString:@"true"] || [stringValue isEqualToString:@"1"];
}

- (int)intFromParameters:(NSDictionary *)parameters forKey:(NSString *)key
{
    NSString *stringValue = [parameters valueForKey:key];
    return stringValue ? [stringValue intValue] : -1;
}

- (NSString *)stringFromParameters:(NSDictionary *)parameters forKey:(NSString *)key
{
    NSString *value = [parameters objectForKey:key];
    if (!value || [value isEqual:[NSNull null]]) return nil;

    value = [value stringByTrimmingCharactersInSet:
             [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (!value || [value isEqual:[NSNull null]] || value.length == 0) return nil;

    return value;
}

- (NSURL *)urlFromParameters:(NSDictionary *)parameters forKey:(NSString *)key
{
    NSString *value = [self stringFromParameters:parameters forKey:key];
    return [NSURL URLWithString:value];
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRCloseCommand

+ (NSString *)commandType
{
    return @"close";
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    [self.delegate mrCommandClose:self];
    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRExpandCommand

+ (NSString *)commandType
{
    return @"expand";
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    NSURL *url = [self urlFromParameters:params forKey:@"url"];

    NSDictionary *expandParams = [NSDictionary dictionaryWithObjectsAndKeys:
                                  (url == nil) ? [NSNull null] : url , @"url",
                                  [NSNumber numberWithBool:[self boolFromParameters:params forKey:@"shouldUseCustomClose"]], @"useCustomClose",
                                  nil];

    [self.delegate mrCommand:self expandWithParams:expandParams];

    return YES;
}
@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRResizeCommand

+ (NSString *)commandType
{
    return @"resize";
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    [self.delegate mrCommand:self resizeWithParams:params];

    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRUseCustomCloseCommand

// We allow useCustomClose to run while we're blocking requests because it only controls how we present a UIButton.
// It can't present/dismiss any view or view controllers. It also doesn't affect any mraid ad/screen metrics.
- (BOOL)executableWhileBlockingRequests
{
    return YES;
}

- (BOOL)requiresUserInteractionForPlacementType:(NSUInteger)placementType
{
    return NO;
}

+ (NSString *)commandType
{
    return @"usecustomclose";
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    [self.delegate mrCommand:self shouldUseCustomClose:[self boolFromParameters:params forKey:@"shouldUseCustomClose"]];

    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRSetOrientationPropertiesCommand

+ (NSString *)commandType
{
    return @"setOrientationProperties";
}

- (BOOL)requiresUserInteractionForPlacementType:(NSUInteger)placementType
{
    return NO;
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    // We can take the forceOrientation and allowOrientationChange values and boil them down to an orientation mask
    // that will represent the intention of the ad.
    UIInterfaceOrientationMask forceOrientationMaskValue;

    NSString *forceOrientationString = params[@"forceOrientation"];

    // Give a default value of "none" for forceOrientationString if it didn't come in through the params.
    if (!forceOrientationString) {
        forceOrientationString = kOrientationPropertyForceOrientationNoneKey;
    }

    // Do not allow orientation changing if we're given a force orientation other than none. Based on the spec,
    // we believe that forceOrientation takes precedence over allowOrientationChange and should not allow
    // orientation changes when a forceOrientation other than 'none' is given.
    if ([forceOrientationString isEqualToString:kOrientationPropertyForceOrientationPortraitKey]) {
        forceOrientationMaskValue = UIInterfaceOrientationMaskPortrait;
    } else if ([forceOrientationString isEqualToString:kOrientationPropertyForceOrientationLandscapeKey]) {
        forceOrientationMaskValue = UIInterfaceOrientationMaskLandscape;
    } else {
        // Default allowing orientation change to YES. We will change this only if we received a value for this in our params.
        BOOL allowOrientationChangeValue = YES;

        // If we end up allowing orientation change, then we're going to allow any orientation.
        forceOrientationMaskValue = UIInterfaceOrientationMaskAll;

        NSObject *allowOrientationChangeObj = params[@"allowOrientationChange"];

        if (allowOrientationChangeObj) {
            allowOrientationChangeValue = [self boolFromParameters:params forKey:@"allowOrientationChange"];
        }

        // If we don't allow orientation change, we're locking the user into the current orientation.
        if (!allowOrientationChangeValue) {
            UIInterfaceOrientation currentOrientation = MPInterfaceOrientation();

            if (UIInterfaceOrientationIsLandscape(currentOrientation)) {
                forceOrientationMaskValue = UIInterfaceOrientationMaskLandscape;
            } else if (currentOrientation == UIInterfaceOrientationPortrait) {
                forceOrientationMaskValue = UIInterfaceOrientationMaskPortrait;
            } else if (currentOrientation == UIInterfaceOrientationPortraitUpsideDown) {
                forceOrientationMaskValue = UIInterfaceOrientationMaskPortraitUpsideDown;
            }
        }
    }

    [self.delegate mrCommand:self setOrientationPropertiesWithForceOrientation:forceOrientationMaskValue];

    return YES;
}
/*
 * We allow setOrientationProperties to run while we're blocking requests because this command can occur during the presentation
 * animation of an interstitial, and has a strong effect on how an ad is presented so we want to make sure it's executed.
 *
 * Even though we return YES here, updating orientation while blocking requests is not safe. MRController receives the appropriate
 * delegate call, and caches the intended call in a block, which it executes when request-blocking is disabled.
 */
- (BOOL)executableWhileBlockingRequests
{
    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MROpenCommand

+ (NSString *)commandType
{
    return @"open";
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    [self.delegate mrCommand:self openURL:[self urlFromParameters:params forKey:@"url"]];

    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRPlayVideoCommand

+ (NSString *)commandType
{
    return @"playVideo";
}

- (BOOL)requiresUserInteractionForPlacementType:(NSUInteger)placementType
{
    // allow interstitials to auto-play video
    return placementType != MRAdViewPlacementTypeInterstitial;
}

- (BOOL)executeWithParams:(NSDictionary *)params
{
    [self.delegate mrCommand:self playVideoWithURL:[self urlFromParameters:params forKey:@"uri"]];

    return YES;
}

@end
