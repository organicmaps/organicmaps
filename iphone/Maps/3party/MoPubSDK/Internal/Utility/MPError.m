//
//  MPError.m
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import "MPError.h"

NSString * const kMOPUBErrorDomain = @"com.mopub.iossdk";

@implementation MOPUBError

+ (MOPUBError *)errorWithCode:(MOPUBErrorCode)code
{
    return [self errorWithDomain:kMOPUBErrorDomain code:code userInfo:nil];
}

@end
