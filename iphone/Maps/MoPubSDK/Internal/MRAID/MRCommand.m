//
//  MRCommand.m
//  MoPub
//
//  Created by Andrew He on 12/19/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import "MRCommand.h"
#import "MRAdView.h"
#import "MRAdViewDisplayController.h"
#import "MPGlobal.h"
#import "MPLogging.h"

@implementation MRCommand

@synthesize delegate = _delegate;
@synthesize view = _view;
@synthesize parameters = _parameters;

+ (NSMutableDictionary *)sharedCommandClassMap {
    static NSMutableDictionary *sharedMap = nil;
    @synchronized(self) {
        if (!sharedMap) sharedMap = [[NSMutableDictionary alloc] init];
    }
    return sharedMap;
}

+ (void)registerCommand:(Class)commandClass {
    NSMutableDictionary *map = [self sharedCommandClassMap];
    @synchronized(self) {
        [map setValue:commandClass forKey:[commandClass commandType]];
    }
}

+ (NSString *)commandType {
    return @"BASE_CMD_TYPE";
}

+ (Class)commandClassForString:(NSString *)string {
    NSMutableDictionary *map = [self sharedCommandClassMap];
    @synchronized(self) {
        return [map objectForKey:string];
    }
}

+ (id)commandForString:(NSString *)string {
    Class commandClass = [self commandClassForString:string];
    return [[[commandClass alloc] init] autorelease];
}

- (void)dealloc {
    [_parameters release];
    [super dealloc];
}

// return YES by default for user safety
- (BOOL)requiresUserInteraction {
    return YES;
}

- (BOOL)execute {
    return YES;
}

- (CGFloat)floatFromParametersForKey:(NSString *)key {
    return [self floatFromParametersForKey:key withDefault:0.0];
}

- (CGFloat)floatFromParametersForKey:(NSString *)key withDefault:(CGFloat)defaultValue {
    NSString *stringValue = [self.parameters valueForKey:key];
    return stringValue ? [stringValue floatValue] : defaultValue;
}

- (BOOL)boolFromParametersForKey:(NSString *)key {
    NSString *stringValue = [self.parameters valueForKey:key];
    return [stringValue isEqualToString:@"true"];
}

- (int)intFromParametersForKey:(NSString *)key {
    NSString *stringValue = [self.parameters valueForKey:key];
    return stringValue ? [stringValue intValue] : -1;
}

- (NSString *)stringFromParametersForKey:(NSString *)key {
    NSString *value = [self.parameters objectForKey:key];
    if (!value || [value isEqual:[NSNull null]]) return nil;

    value = [value stringByTrimmingCharactersInSet:
             [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (!value || [value isEqual:[NSNull null]] || value.length == 0) return nil;

    return value;
}

- (NSURL *)urlFromParametersForKey:(NSString *)key {
    NSString *value = [[self stringFromParametersForKey:key] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    return [NSURL URLWithString:value];
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRCloseCommand

+ (void)load {
    [MRCommand registerCommand:self];
}

+ (NSString *)commandType {
    return @"close";
}

- (BOOL)execute {
    [self.view.displayController close];
    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRExpandCommand

+ (void)load {
    [MRCommand registerCommand:self];
}

+ (NSString *)commandType {
    return @"expand";
}

- (BOOL)execute {
    CGRect applicationFrame = MPApplicationFrame();
    CGFloat afWidth = CGRectGetWidth(applicationFrame);
    CGFloat afHeight = CGRectGetHeight(applicationFrame);

    // If the ad has expandProperties, we should use the width and height values specified there.
    CGFloat w = [self floatFromParametersForKey:@"w" withDefault:afWidth];
    CGFloat h = [self floatFromParametersForKey:@"h" withDefault:afHeight];

    // Constrain the ad to the application frame size.
    if (w > afWidth) w = afWidth;
    if (h > afHeight) h = afHeight;

    // Center the ad within the application frame.
    CGFloat x = applicationFrame.origin.x + floor((afWidth - w) / 2);
    CGFloat y = applicationFrame.origin.y + floor((afHeight - h) / 2);

    NSString *urlString = [self stringFromParametersForKey:@"url"];
    NSURL *url = [NSURL URLWithString:urlString];

    MPLogDebug(@"Expanding to (%.1f, %.1f, %.1f, %.1f); displaying %@.", x, y, w, h, url);

    CGRect newFrame = CGRectMake(x, y, w, h);

    [self.view.displayController expandToFrame:newFrame
                           withURL:url
                    useCustomClose:[self boolFromParametersForKey:@"shouldUseCustomClose"]
                           isModal:NO
             shouldLockOrientation:[self boolFromParametersForKey:@"lockOrientation"]];

    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRUseCustomCloseCommand

+ (void)load {
    [MRCommand registerCommand:self];
}

- (BOOL)requiresUserInteraction {
    return NO;
}

+ (NSString *)commandType {
    return @"usecustomclose";
}

- (BOOL)execute {
    BOOL shouldUseCustomClose = [[self.parameters valueForKey:@"shouldUseCustomClose"] boolValue];
    [self.view.displayController useCustomClose:shouldUseCustomClose];
    return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MROpenCommand

+ (void)load {
    [MRCommand registerCommand:self];
}

+ (NSString *)commandType {
    return @"open";
}

- (BOOL)execute {
    [self.view handleMRAIDOpenCallForURL:[self urlFromParametersForKey:@"url"]];
    return YES;
}

@end
