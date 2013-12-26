
#import <Foundation/Foundation.h>
#import "Reachability.h"

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;

- (BOOL)featureAvailable:(NSString *)featureName;
- (NSString *)snapshot;

@property (nonatomic, strong) NSString * countryCode;
@property (nonatomic, strong) NSString * bundleVersion;
@property (nonatomic, strong) NSString * deviceInfo;
@property (nonatomic, strong) NSString * firmwareVersion;
@property (nonatomic, strong) NSString * uniqueId;
@property (nonatomic, strong) NSString * advertisingId;
@property (nonatomic, strong) Reachability * reachability;

@end
