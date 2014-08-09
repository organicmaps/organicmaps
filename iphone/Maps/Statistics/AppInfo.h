
#import <Foundation/Foundation.h>
#import "Reachability.h"

extern NSString * const AppFeatureInterstitial;
extern NSString * const AppFeatureBanner;
extern NSString * const AppFeatureProButtonOnMap;
extern NSString * const AppFeatureMoreAppsBanner;
extern NSString * const AppFeatureBottomMenuItems;

extern NSString * const AppInfoSyncedNotification;

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;

- (BOOL)featureAvailable:(NSString *)featureName;
- (id)featureValue:(NSString *)featureName forKey:(NSString *)key;

- (NSString *)snapshot;

@property (nonatomic, readonly) NSString * countryCode;
@property (nonatomic, readonly) NSString * uniqueId;
@property (nonatomic, readonly) Reachability * reachability;
@property (nonatomic, readonly) NSInteger launchCount;
@property (nonatomic, readonly) NSDate * firstLaunchDate;
- (NSString *)bundleVersion;
- (NSString *)deviceInfo;
- (NSString *)firmwareVersion;
- (NSUUID *)advertisingId;

@end
