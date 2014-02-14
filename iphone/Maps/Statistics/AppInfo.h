
#import <Foundation/Foundation.h>
#import "Reachability.h"

extern NSString * const AppFeatureInterstitial;
extern NSString * const AppFeatureBanner;
extern NSString * const AppFeatureProButtonOnMap;
extern NSString * const AppFeatureMoreAppsBanner;

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;

- (BOOL)featureAvailable:(NSString *)featureName;
- (id)featureValue:(NSString *)featureName forKey:(NSString *)key;

- (NSString *)snapshot;

@property (nonatomic, readonly) NSString * countryCode;
@property (nonatomic, readonly) NSString * bundleVersion;
@property (nonatomic, readonly) NSString * deviceInfo;
@property (nonatomic, readonly) NSString * firmwareVersion;
@property (nonatomic, readonly) NSString * uniqueId;
@property (nonatomic, readonly) NSUUID * advertisingId;
@property (nonatomic, readonly) Reachability * reachability;
@property (nonatomic, readonly) NSInteger launchCount;
@property (nonatomic, readonly) NSDate * firstLaunchDate;

@end
