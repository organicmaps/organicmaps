
#import <Foundation/Foundation.h>
#import "Reachability.h"

extern NSString * const AppFeatureMoPubInterstitial;
extern NSString * const AppFeatureMWMProInterstitial;
extern NSString * const AppFeatureMoPubBanner;
extern NSString * const AppFeatureMWMProBanner;

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;

- (void)setup; // call on application start

- (BOOL)featureAvailable:(NSString *)featureName;
- (id)featureValue:(NSString *)featureName forKey:(NSString *)key defaultValue:(id)defaultValue;

- (NSString *)snapshot;

@property (nonatomic, readonly) NSString * countryCode;
@property (nonatomic, readonly) NSString * bundleVersion;
@property (nonatomic, readonly) NSString * deviceInfo;
@property (nonatomic, readonly) NSString * firmwareVersion;
@property (nonatomic, readonly) NSString * uniqueId;
@property (nonatomic, readonly) NSString * advertisingId;
@property (nonatomic, readonly) Reachability * reachability;
@property (nonatomic, readonly) NSInteger launchCount;
@property (nonatomic, readonly) NSDate * firstLaunchDate;

@end
