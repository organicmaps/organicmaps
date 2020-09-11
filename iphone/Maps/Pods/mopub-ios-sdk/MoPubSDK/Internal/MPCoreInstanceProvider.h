//
//  MPCoreInstanceProvider.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface MPCoreInstanceProvider : NSObject

+ (instancetype)sharedProvider;

- (void)keepObjectAliveForCurrentRunLoopIteration:(id)anObject;

@end
