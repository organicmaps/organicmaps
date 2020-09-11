//
//  MPCoreInstanceProvider+MRAID.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPCoreInstanceProvider+MRAID.h"
#import "MRBundleManager.h"

@implementation MPCoreInstanceProvider (MRAID)

- (NSString *)mraidJavascript {
    static NSString * mraidJavascript = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSString * mraidJavascriptFilePath = [[MRBundleManager sharedManager] mraidPath];
        mraidJavascript = [NSString stringWithContentsOfFile:mraidJavascriptFilePath encoding:NSUTF8StringEncoding error:nil];
    });

    // @c stringWithContentsOfFile will return @c nil when the file cannot be opened or parsed. Therefore, if
    // mraid.js is missing, this method will return @c nil.
    return mraidJavascript;
}

- (BOOL)isMraidJavascriptAvailable {
    return [self mraidJavascript] != nil;
}

@end
