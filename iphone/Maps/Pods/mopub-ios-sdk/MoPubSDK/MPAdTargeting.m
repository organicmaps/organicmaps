//
//  MPAdTargeting.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdTargeting.h"

@implementation MPAdTargeting

- (instancetype)initWithCreativeSafeSize:(CGSize)size {
    if (self = [super init]) {
        self.creativeSafeSize = size;
    }

    return self;
}

+ (instancetype)targetingWithCreativeSafeSize:(CGSize)size {
    return [[MPAdTargeting alloc] initWithCreativeSafeSize:size];
}

@end
