
#import <Foundation/Foundation.h>
#import "Reachability.h"

extern NSString * const AppFeatureInterstitialAd;
extern NSString * const AppFeatureBannerAd;

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;

- (BOOL)featureAvailable:(NSString *)featureName;
- (NSString *)snapshot;

@property (nonatomic, strong, readonly) NSString * countryCode;
@property (nonatomic, strong, readonly) NSString * bundleVersion;
@property (nonatomic, strong, readonly) NSString * deviceInfo;
@property (nonatomic, strong, readonly) NSString * firmwareVersion;
@property (nonatomic, strong, readonly) NSString * uniqueId;
@property (nonatomic, strong, readonly) NSString * advertisingId;
@property (nonatomic, strong, readonly) Reachability * reachability;
@property (nonatomic, readonly) NSInteger launchCount;

@end
