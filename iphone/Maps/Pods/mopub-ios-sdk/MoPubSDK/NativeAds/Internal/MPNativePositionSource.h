//
//  MPNativePositionSource.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@class MPAdPositioning;

typedef enum : NSUInteger {
    MPNativePositionSourceInvalidAdUnitIdentifier,
    MPNativePositionSourceEmptyResponse,
    MPNativePositionSourceDeserializationFailed,
    MPNativePositionSourceConnectionFailed,
} MPNativePositionSourceErrorCode;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNativePositionSource : NSObject

- (void)loadPositionsWithAdUnitIdentifier:(NSString *)identifier completionHandler:(void (^)(MPAdPositioning *positioning, NSError *error))completionHandler;
- (void)cancel;

@end
