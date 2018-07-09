//
//  MRBundleManager.m
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MRBundleManager.h"
#import "NSBundle+MPAdditions.h"

@implementation MRBundleManager

static MRBundleManager *sharedManager = nil;

+ (MRBundleManager *)sharedManager
{
    if (!sharedManager) {
        sharedManager = [[MRBundleManager alloc] init];
    }
    return sharedManager;
}

- (NSString *)mraidPath
{
    NSBundle *parentBundle = [NSBundle resourceBundleForClass:self.class];

    NSString *mraidBundlePath = [parentBundle pathForResource:@"MRAID" ofType:@"bundle"];
    NSBundle *mraidBundle = [NSBundle bundleWithPath:mraidBundlePath];
    return [mraidBundle pathForResource:@"mraid" ofType:@"js"];
}

@end
