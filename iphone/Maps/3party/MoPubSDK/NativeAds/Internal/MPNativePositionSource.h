//
//  MPNativePositionSource.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
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
