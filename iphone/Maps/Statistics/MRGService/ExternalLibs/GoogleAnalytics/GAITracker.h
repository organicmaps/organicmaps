/*!
 @header    GAITracker.h
 @abstract  Google Analytics iOS SDK Tracker Header
 @version   3.0
 @copyright Copyright 2013 Google Inc. All rights reserved.
*/

#import <Foundation/Foundation.h>

/*!
 Google Analytics tracking interface. Obtain instances of this interface from
 [GAI trackerWithTrackingId:] to track screens, events, transactions, timing,
 and exceptions. The implementation of this interface is thread-safe, and no
 calls are expected to block or take a long time.  All network and disk activity
 will take place in the background.
 */
@protocol GAITracker<NSObject>

/*!
 Name of this tracker.
 */
@property(nonatomic, readonly) NSString *name;

/*!
 Allow collection of IDFA and related fields if set to true.  Default is false.
 */
@property(nonatomic) BOOL allowIDFACollection;

/*!
 Set a tracking parameter.

 @param parameterName The parameter name.

 @param value The value to set for the parameter. If this is nil, the
 value for the parameter will be cleared.
 */
- (void)set:(NSString *)parameterName
      value:(NSString *)value;

/*!
 Get a tracking parameter.

 @param parameterName The parameter name.

 @returns The parameter value, or nil if no value for the given parameter is
 set.
 */
- (NSString *)get:(NSString *)parameterName;

/*!
 Queue tracking information with the given parameter values.

 @param parameters A map from parameter names to parameter values which will be
 set just for this piece of tracking information, or nil for none.
 */
- (void)send:(NSDictionary *)parameters;

@end
