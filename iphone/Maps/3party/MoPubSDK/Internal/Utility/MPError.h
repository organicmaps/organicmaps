//
//  MPError.h
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString * const kMOPUBErrorDomain;

typedef enum {
    MOPUBErrorUnknown = -1,
    MOPUBErrorNoInventory = 0,
    MOPUBErrorAdUnitWarmingUp = 1,
    MOPUBErrorNetworkTimedOut = 4,
    MOPUBErrorServerError = 8,
    MOPUBErrorAdapterNotFound = 16,
    MOPUBErrorAdapterInvalid = 17,
    MOPUBErrorAdapterHasNoInventory = 18
} MOPUBErrorCode;

@interface MOPUBError : NSError

+ (MOPUBError *)errorWithCode:(MOPUBErrorCode)code;

@end
