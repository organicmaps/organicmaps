
#import "AppInfo.h"
#import "Reachability.h"
#import <CoreTelephony/CTCarrier.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <sys/utsname.h>

NSString * const FeaturesKey = @"FeaturesKey"; // value is array of dictionaries

@interface AppInfo ()

@property Reachability * reachability;
@property (nonatomic) NSMutableDictionary * info;
@property NSDictionary * featuresByDefault;

@end

@implementation AppInfo

- (instancetype)init
{
  self = [super init];

  self.featuresByDefault = @{@"ads" : @NO, @"popup" : @YES};

  [self update];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];

  return self;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
  [self.info writeToFile:[self infoFilePath] atomically:YES];
}

- (NSString *)snapshot
{
  return [NSString stringWithFormat:@"MapsWithMe ver. %@, %@ (iOS %@) %@", self.bundleVersion, self.deviceInfo, self.firmwareVersion, self.countryCode];
}

- (void)update
{
  // get info from server
  // if not success then subscribe for reachability updates
  self.reachability = [Reachability reachabilityForInternetConnection];
  __weak id weakSelf = self;
  self.reachability.reachableBlock = ^(Reachability * r){
    [weakSelf update];
  };
  [self.reachability startNotifier];
  self.info[FeaturesKey] = @[@{}, @{@"ads" : @"ru,en"}];
}

- (BOOL)featureAvailable:(NSString *)featureName
{
  if (!self.info[FeaturesKey])
    return [self.featuresByDefault[featureName] boolValue];

  for (NSDictionary * feature in self.info[FeaturesKey])
  {
    if ([[[feature allKeys] firstObject] isEqualToString:featureName])
      return ([feature[@"locals"] rangeOfString:self.countryCode].location != NSNotFound) || [feature[@"locals"] isEqualToString:@"*"];
  }
  return NO;
}

- (NSString *)infoFilePath
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
  return [paths[0] stringByAppendingPathComponent:@"mwm_info"];
}

#pragma mark - Public properties

- (NSString *)userId
{
  UIDevice * device = [UIDevice currentDevice];
  return [device respondsToSelector:@selector(identifierForVendor)] ? [device.identifierForVendor UUIDString] : nil;
}

- (NSString *)countryCode
{
  if (!_countryCode)
  {
    CTTelephonyNetworkInfo * networkInfo = [[CTTelephonyNetworkInfo alloc] init];
    CTCarrier * carrier = networkInfo.subscriberCellularProvider;
    _countryCode = [carrier.isoCountryCode uppercaseString];
    if (!_countryCode)
      _countryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
  }
  return _countryCode;
}

- (NSString *)bundleVersion
{
  return [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
}

- (NSString *)deviceInfo
{
  struct utsname systemInfo;
  uname(&systemInfo);
  return [NSString stringWithCString:systemInfo.machine
                            encoding:NSUTF8StringEncoding];
}

- (NSString *)firmwareVersion
{
  return [UIDevice currentDevice].systemVersion;
}

#pragma mark - Private properties

- (NSMutableDictionary *)info
{
  if (!_info)
  {
    _info = [NSMutableDictionary dictionaryWithContentsOfFile:[self infoFilePath]];
    if (!_info)
      _info = [NSMutableDictionary dictionary];
  }
  return _info;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [self.reachability stopNotifier];
}

+ (instancetype)sharedInfo
{
  static AppInfo * appInfo;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    appInfo = [[self alloc] init];
  });
  return appInfo;
}

@end
