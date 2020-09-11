//
//  MPURL.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN

/**
 MoPub-specific URL object to include passing along a POST request payload.
 */
@interface MPURL : NSURL
/**
 Dictionary of data that will be represented as JSON in the POST request body.
 @c NSJSONSerialization will be used to perform the serialization of the data
 into JSON; as such the only objects that are supported are: @c NSString,
 @c NSNumber, @c NSArray, @c NSDictionary, and @c NSNull.
 @note Keys and values will not be URL encoded.
 */
@property (nonatomic, strong, readonly) NSMutableDictionary<NSString *, NSObject *> * postData;

/**
 Initialize with a valid URL string with the string arguments to contain any percent
 escape codes that are necessary. It is an error for URLString to be nil.
 @param URLString Valid URL string with percent escaped arguments.
 @return MoPub object representation of the URL.
 */
- (instancetype _Nullable)initWithString:(NSString * _Nullable)URLString;

/**
 Initialize with a valid URL string with the string arguments to contain any percent
 escape codes that are necessary. It is an error for URLString to be nil.
 @param URLString Valid URL string with percent escaped arguments.
 @return MoPub object representation of the URL.
 */
+ (instancetype _Nullable)URLWithString:(NSString * _Nullable)URLString;

/**
 Initialize with a URL components and optional POST data.
 @param components Components of the URL.
 @param postData Optional POST data to include with the URL. The values should not be URL encoded,
 and only supports the following JSON-serializable types: @c NSString, @c NSNumber, @c NSDictionary,
 and @c NSArray.
 @return MoPub object representation of the URL.
 */
+ (instancetype _Nullable)URLWithComponents:(NSURLComponents * _Nullable)components postData:(NSDictionary<NSString *, NSObject *> * _Nullable)postData;

/**
 Attempts to retrieve the value for the POST data key as a @c NSArray.
 @param key POST data key.
 @return The value of the POST data key cast into a @c NSArray; otherwise @c nil.
 */
- (NSArray * _Nullable)arrayForPOSTDataKey:(NSString *)key;

/**
 Attempts to retrieve the value for the POST data key as a @c NSDictionary.
 @param key POST data key.
 @return The value of the POST data key cast into a @c NSDictionary; otherwise @c nil.
 */
- (NSDictionary * _Nullable)dictionaryForPOSTDataKey:(NSString *)key;

/**
 Attempts to retrieve the value for the POST data key as a @c NSNumber.
 @param key POST data key.
 @return The value of the POST data key cast into a @c NSNumber; otherwise @c nil.
 */
- (NSNumber * _Nullable)numberForPOSTDataKey:(NSString *)key;

/**
 Attempts to retrieve the value for the POST data key as a @c NSString.
 @param key POST data key.
 @return The value of the POST data key cast into a @c NSString; otherwise @c nil.
 */
- (NSString * _Nullable)stringForPOSTDataKey:(NSString *)key;

@end

NS_ASSUME_NONNULL_END
