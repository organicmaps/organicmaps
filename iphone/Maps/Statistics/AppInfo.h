
#import <Foundation/Foundation.h>
#import "Reachability.h"

extern NSString * const AppFeatureInterstitialAd;
extern NSString * const AppFeatureBannerAd;

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;

- (BOOL)featureAvailable:(NSString *)featureName;
- (NSString *)snapshot;

@property (nonatomic, readonly) NSString * countryCode;
@property (nonatomic, readonly) NSString * bundleVersion;
@property (nonatomic, readonly) NSString * deviceInfo;
@property (nonatomic, readonly) NSString * firmwareVersion;
@property (nonatomic, readonly) NSString * uniqueId;
@property (nonatomic, readonly) NSString * advertisingId;
@property (nonatomic, readonly) Reachability * reachability;
@property (nonatomic) NSInteger launchCount;

@end
