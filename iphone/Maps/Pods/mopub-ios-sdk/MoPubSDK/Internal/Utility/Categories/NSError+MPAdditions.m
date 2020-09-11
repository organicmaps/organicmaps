//
//  NSError+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "NSError+MPAdditions.h"
#import "MPError.h"

@implementation NSError (MPAdditions)

- (BOOL)isAdRequestTimedOutError {
    return ([self.domain isEqualToString:kNSErrorDomain] && self.code == MOPUBErrorAdRequestTimedOut);
}

@end

@implementation NSError (Networking)

+ (NSError *)networkErrorWithHTTPStatusCode:(NSInteger)statusCode {
    // `localizedStringForStatusCode:` will always give back a valid string even if the
    // status code is 200 (not an error). It is up to the caller of this method to
    // determine if the status code is really an error or not.
    NSString * message = [NSHTTPURLResponse localizedStringForStatusCode:statusCode];
    NSError * error = [NSError errorWithDomain:kNSErrorDomain code:MOPUBErrorHTTPResponseNot200 userInfo:@{ NSLocalizedDescriptionKey: message }];

    return error;
}

@end
