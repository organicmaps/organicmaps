//
//  NSError+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface NSError (MPAdditions)

/**
 Queries if the error is a MoPub ad request timeout error.
 */
@property (nonatomic, readonly) BOOL isAdRequestTimedOutError;

@end

@interface NSError (Networking)
/**
 Networking error from an HTTP status code.
 @param statusCode HTTP status code.
 @return Error.
 */
+ (NSError *)networkErrorWithHTTPStatusCode:(NSInteger)statusCode;
@end
