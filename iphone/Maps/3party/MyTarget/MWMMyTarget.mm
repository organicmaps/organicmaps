#import "MWMMyTarget.h"
#import <MyTargetSDK/MTRGManager.h>
#import <MyTargetSDK/MTRGNativeAppwallAd.h>
#import "MWMCommon.h"
#import "MWMSettings.h"
#import "MapsAppDelegate.h"
#import "Statistics.h"

// If you have a "missing header error" here, then please run configure.sh script in the root repo
// folder.
#import "private.h"

@interface MWMMyTarget ()<MTRGNativeAppwallAdDelegate>

@property(weak, nonatomic) NSTimer * checkAdServerForbiddenTimer;

@property(nonatomic) MTRGNativeAppwallAd * appWallAd;

@end

@implementation MWMMyTarget

+ (MWMMyTarget *)manager { return [MapsAppDelegate theApp].myTarget; }
- (MTRGNativeAppwallBanner *)bannerAtIndex:(NSUInteger)index
{
  NSAssert(index < self.bannersCount, @"Invalid banner index");
  return self.appWallAd.banners[index];
}

- (void)handleBannerShowAtIndex:(NSUInteger)index
{
  MTRGNativeAppwallBanner * banner = [self bannerAtIndex:index];
  [Statistics logEvent:kStatMyTargetAppsDisplayed withParameters:@{kStatAd : banner.title}];
  [self.appWallAd handleShow:banner];
}

- (void)handleBannerClickAtIndex:(NSUInteger)index withController:(UIViewController *)controller
{
  MTRGNativeAppwallBanner * banner = [self bannerAtIndex:index];
  [Statistics logEvent:kStatMyTargetAppsClicked withParameters:@{kStatAd : banner.title}];
  [self.appWallAd handleClick:banner withController:controller];
}

#pragma mark - Refresh

- (void)refresh
{
  if (self.bannersCount != 0)
    return;
  [self.appWallAd close];
  if ([MWMSettings adServerForbidden])
    return;
  self.appWallAd = [[MTRGNativeAppwallAd alloc] initWithSlotId:MY_TARGET_KEY];
  self.appWallAd.closeButtonTitle = L(@"close");
  self.appWallAd.delegate = self;
  [self.appWallAd load];
}

- (void)checkAdServerForbidden
{
  NSURLSession * session = [NSURLSession sharedSession];
  NSURL * url = [NSURL URLWithString:@(AD_PERMISION_SERVER_URL)];
  NSURLSessionDataTask * task = [session
        dataTaskWithURL:url
      completionHandler:^(NSData * data, NSURLResponse * response, NSError * error) {
        bool const adServerForbidden = (error || [(NSHTTPURLResponse *)response statusCode] != 200);
        [MWMSettings setAdServerForbidden:adServerForbidden];
        dispatch_async(dispatch_get_main_queue(), ^{
          [self refresh];
        });
      }];
  [task resume];
}

+ (void)startAdServerForbiddenCheckTimer
{
  MWMMyTarget * manager = [self manager];
  [manager checkAdServerForbidden];
  [manager.checkAdServerForbiddenTimer invalidate];
  manager.checkAdServerForbiddenTimer =
      [NSTimer scheduledTimerWithTimeInterval:AD_PERMISION_CHECK_DURATION
                                       target:manager
                                     selector:@selector(checkAdServerForbidden)
                                     userInfo:nil
                                      repeats:YES];
}

#pragma mark - MTRGNativeAppwallAdDelegate

- (void)onLoadWithAppwallBanners:(NSArray *)appwallBanners
                       appwallAd:(MTRGNativeAppwallAd *)appwallAd
{
  if (![appwallAd isEqual:self.appWallAd])
    return;
  if (appwallBanners.count == 0)
    [self.appWallAd close];
  [self.delegate onAppWallRefresh];
}

- (void)onNoAdWithReason:(NSString *)reason appwallAd:(MTRGNativeAppwallAd *)appwallAd
{
  if (![appwallAd isEqual:self.appWallAd])
    return;
  [self.appWallAd close];
}

#pragma mark - Properties

- (NSUInteger)bannersCount { return self.appWallAd.banners.count; }
@end
