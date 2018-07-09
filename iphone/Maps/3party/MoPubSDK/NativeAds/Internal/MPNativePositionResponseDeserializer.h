//
//  MPNativePositionResponseDeserializer.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class MPClientAdPositioning;

typedef enum : NSUInteger {
    MPNativePositionResponseDataIsEmpty,
    MPNativePositionResponseIsNotValidJSON,
    MPNativePositionResponseJSONHasInvalidPositionData,
} MPNativePositionResponseDeserializationErrorCode;

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The `MPNativePositionResponseDeserializer` class is used to convert HTTP response data
 * containing positioning information into ad positioning objects that may be used by various
 * native ad placers.
 */
@interface MPNativePositionResponseDeserializer : NSObject

/**
 * Creates and returns an object that can deserialize HTTP response data into ad positioning
 * objects.
 *
 * @return The newly created deserializer.
 */
+ (instancetype)deserializer;

/**
 * Returns an ad positioning object given a data object.
 *
 * If an error occurs during the data conversion, this method will return an empty positioning
 * object containing no desired ad positions.
 *
 * @param data A data object containing positioning information.
 * @param error A pointer to an error object. If an error occurs, this pointer will be set to an
 * actual error object containing the error information.
 *
 * @return An `MPClientAdPositioning` object. This is guaranteed to be non-nil; if an error occurs
 * during deserialization, the return value will still be a positioning object with no ad positions.
 */
- (MPClientAdPositioning *)clientPositioningForData:(NSData *)data error:(NSError **)error;

@end
