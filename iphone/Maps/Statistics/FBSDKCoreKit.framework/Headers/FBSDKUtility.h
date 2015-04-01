// Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
//
// You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
// copy, modify, and distribute this software in source code or binary form for use
// in connection with the web services and APIs provided by Facebook.
//
// As with any software that integrates with the Facebook platform, your use of
// this software is subject to the Facebook Developer Principles and Policies
// [http://developers.facebook.com/policy/]. This copyright notice shall be
// included in all copies or substantial portions of the software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import <Foundation/Foundation.h>

/*!
 @abstract Class to contain common utility methods.
 */
@interface FBSDKUtility : NSObject

/*!
 @abstract Parses a query string into a dictionary.
 @param queryString The query string value.
 @return A dictionary with the key/value pairs.
 */
+ (NSDictionary *)dictionaryWithQueryString:(NSString *)queryString;

/*!
 @abstract Constructs a query string from a dictionary.
 @param dictionary The dictionary with key/value pairs for the query string.
 @param errorRef If an error occurs, upon return contains an NSError object that describes the problem.
 @result Query string representation of the parameters.
 */
+ (NSString *)queryStringWithDictionary:(NSDictionary *)dictionary error:(NSError *__autoreleasing *)errorRef;

/*!
 @abstract Decodes a value from an URL.
 @param value The value to decode.
 @result The decoded value.
 */
+ (NSString *)URLDecode:(NSString *)value;

/*!
 @abstract Encodes a value for an URL.
 @param value The value to encode.
 @result The encoded value.
 */
+ (NSString *)URLEncode:(NSString *)value;

@end
