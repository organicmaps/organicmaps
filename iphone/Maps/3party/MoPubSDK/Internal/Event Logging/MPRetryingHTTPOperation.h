//
//  MPRetryingHTTPOperation.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "MPQRunLoopOperation.h"

extern NSString * const MPRetryingHTTPOperationErrorDomain;

typedef enum {
    MPRetryingHTTPOperationReceivedNonRetryResponse = -1000,
    MPRetryingHTTPOperationExceededRetryLimit = -1001
} MPRetryingHTTPOperationErrorCode;

////////////////////////////////////////////////////////////////////////////////////////////////////

// Adapted from QHTTPOperation / RetryingHTTPOperation in Apple's MVCNetworking sample code.

@interface MPRetryingHTTPOperation : MPQRunLoopOperation

// Things that are configured via the init method and can't be changed.
@property (copy, readonly) NSURLRequest *request;

// Things that are only meaningful after the operation is finished.
@property (copy, readonly) NSHTTPURLResponse *lastResponse;
@property (strong, readonly) NSMutableData *lastReceivedData;

- (instancetype)initWithRequest:(NSURLRequest *)request;

@end
