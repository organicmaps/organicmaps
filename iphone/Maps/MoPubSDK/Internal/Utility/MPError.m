//
//  MPAdRequestError.m
//  MoPub
//
//  Copyright (c) 2012 MoPub. All rights reserved.
//

#import "MPError.h"

NSString * const kMPErrorDomain = @"com.mopub.iossdk";

@implementation MPError

+ (MPError *)errorWithCode:(MPErrorCode)code
{
    return [self errorWithDomain:kMPErrorDomain code:code userInfo:nil];
}

@end
