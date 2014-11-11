/*!
 @header    GAILogger.h
 @abstract  Google Analytics iOS SDK Source
 @copyright Copyright 2011 Google Inc. All rights reserved.
 */

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, GAILogLevel) {
  kGAILogLevelNone = 0,
  kGAILogLevelError = 1,
  kGAILogLevelWarning = 2,
  kGAILogLevelInfo = 3,
  kGAILogLevelVerbose = 4
};

/*!
 Protocol to be used for logging debug and informational messages from the SDK.
 Implementations of this protocol can be provided to the |GAI| class,
 to be used as the logger by the SDK.  See the |logger| property in GAI.h.
 */
@protocol GAILogger<NSObject>
@required

/*!
 Only messages of |logLevel| and below are logged.
 */
@property (nonatomic, assign) GAILogLevel logLevel;

/*!
 Logs message with log level |kGAILogLevelVerbose|.
 */
- (void)verbose:(NSString *)message;

/*!
 Logs message with log level |kGAILogLevelInfo|.
 */
- (void)info:(NSString *)message;

/*!
 Logs message with log level |kGAILogLevelWarning|.
 */
- (void)warning:(NSString *)message;

/*!
 Logs message with log level |kGAILogLevelError|.
 */
- (void)error:(NSString *)message;
@end
