//
//  MPFacebookAttributionIdProvider.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPFacebookKeywordProvider.h"
#import <UIKit/UIKit.h>

static NSString *kFacebookAttributionIdPasteboardKey = @"fb_app_attribution";
static NSString *kFacebookAttributionIdPrefix = @"FBATTRID:";

@implementation MPFacebookKeywordProvider

#pragma mark - MPKeywordProvider

+ (NSString *)keyword {
    NSString *facebookAttributionId = [[UIPasteboard pasteboardWithName:kFacebookAttributionIdPasteboardKey
                                                                 create:NO] string];
    if (!facebookAttributionId) {
        return nil;
    }

    return [NSString stringWithFormat:@"%@%@", kFacebookAttributionIdPrefix, facebookAttributionId];
}

@end
