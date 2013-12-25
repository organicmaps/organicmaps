
#import <Foundation/Foundation.h>

@interface AppInfo : NSObject

- (BOOL)featureAvailable:(NSString *)featureName;

@property (nonatomic, strong) NSString * countryCode;
@property (nonatomic, strong) NSString * bundleVersion;
@property (nonatomic, strong) NSString * deviceInfo;
@property (nonatomic, strong) NSString * firmwareVersion;
@property (nonatomic, strong) NSString * userId;

- (NSString *)snapshot;

+ (instancetype)sharedInfo;

@end
