#import <Foundation/Foundation.h>

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;
- (instancetype)init __attribute__((unavailable("init is not available")));

@property(nonatomic, readonly) NSString * countryCode;
@property(nonatomic, readonly) NSString * uniqueId;
@property(nonatomic, readonly) NSString * bundleVersion;
@property(nonatomic, readonly) NSString * buildNumber;
@property(nonatomic, readonly) NSUUID * advertisingId;
@property(nonatomic, readonly) NSString * languageId;
@property(nonatomic, readonly) NSDate * buildDate;
@property(nonatomic, readonly) NSString * deviceName;
@property(nonatomic, readonly) BOOL isMetalDriver;

@end
