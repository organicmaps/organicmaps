#import <Foundation/Foundation.h>

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;
- (instancetype)init __attribute__((unavailable("init is not available")));

@property (nonatomic, readonly) NSString * snapshot;
@property (nonatomic, readonly) NSString * countryCode;
@property (nonatomic, readonly) NSString * uniqueId;
@property (nonatomic, readonly) NSInteger launchCount;
@property (nonatomic, readonly) NSDate * firstLaunchDate;
@property (nonatomic, readonly) NSString * bundleVersion;
@property (nonatomic, readonly) NSString * deviceInfo;
@property (nonatomic, readonly) NSUUID * advertisingId;
@property (nonatomic, readonly) NSString * languageId;

@end
