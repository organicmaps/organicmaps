//
//  MPEngineInfo.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 SDK Engine information specifying the wrapper name and version for the MoPub SDK.
 */
@interface MPEngineInfo : NSObject <NSCoding>
/**
 Name of the engine using the MoPub SDK. This field is @c nil for platform native integrations.
 */
@property (nonatomic, copy, readonly, nullable) NSString * name;

/**
 Version of the engine using the MoPub SDK. This field is @c nil for platform native integrations.
 */
@property (nonatomic, copy, readonly, nullable) NSString * version;

/**
 Required initializer
 @param name Name of the engine using the MoPub SDK.
 @param version Version of the engine using the MoPub SDK.
 @return An initialized instance of the engine information.
 */
- (instancetype)initWithName:(NSString *)name version:(NSString *)version;

/**
 Required initializer
 @param name Name of the engine using the MoPub SDK.
 @param version Version of the engine using the MoPub SDK.
 @return An initialized instance of the engine information.
 */
+ (instancetype)named:(NSString *)name version:(NSString *)version;

#pragma mark - Disallowed Initializers

- (instancetype)init NS_UNAVAILABLE;

+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
