//
//  MRBundleManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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
