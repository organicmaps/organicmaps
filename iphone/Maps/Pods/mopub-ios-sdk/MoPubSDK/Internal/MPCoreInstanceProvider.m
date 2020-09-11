//
//  MPCoreInstanceProvider.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPCoreInstanceProvider.h"

@implementation MPCoreInstanceProvider

static MPCoreInstanceProvider *sharedProvider = nil;

+ (instancetype)sharedProvider
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        sharedProvider = [[self alloc] init];
    });

    return sharedProvider;
}

- (id)init
{
    self = [super init];
    if (self) {
    }
    return self;
}

// This method ensures that "anObject" is retained until the next runloop iteration when
// performNoOp: is executed.
//
// This is useful in situations where, potentially due to a callback chain reaction, an object
// is synchronously deallocated as it's trying to do more work, especially invoking self, after
// the callback.
- (void)keepObjectAliveForCurrentRunLoopIteration:(id)anObject
{
    [self performSelector:@selector(performNoOp:) withObject:anObject afterDelay:0];
}

- (void)performNoOp:(id)anObject
{
    ; // noop
}

@end
