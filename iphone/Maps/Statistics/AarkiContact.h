#import <UIKit/UIKit.h>

typedef void(^AarkiRewards)(NSString *placementId, NSNumber *rewards);

@interface AarkiContact : NSObject

// Register Application in Aarki System
+ (void)registerApp:(NSString *)securityKey;

// Pass security configuration (DEPRECATED in SDK 4.3)
+ (void)registerAppWithClientSecurityKey:(NSString *)securityKey __attribute__ ((deprecated("Please use registerApp:(NSString *)securityKey")));

// Register Application (DEPRECATED in SDK 4.3)
+ (void)registerApp:(NSString *)installEventId withClientSecurityKey:(NSString *)securityKey __attribute__ ((deprecated("Please use registerApp:(NSString *)securityKey")));

// Register custom event
+ (void)registerEvent:(NSString *)eventId;

// Set reward callback
+ (void)setRewardCallback:(AarkiRewards)rewardCallback;

// Set custom user ID
+ (void)setUserId:(NSString *)userId;

// For extension developers, pass values to distinguish client type, e. g. "unity"
+ (void)setClientType:(NSString *)clientType;

// Per-app unique identifier
+ (NSString *)aarkiIdentifier;

// User ID
+ (NSString *)userId;

// Advertising Identifier
+ (NSString *)advertisingId;

+ (BOOL)advertisingTrackingEnabled;

// Aarki SDK version
+ (NSString *)libraryVersion;

// Collect standard request information
+ (void)setStandardRequestParameters:(NSMutableDictionary *)requestDict;

@end
