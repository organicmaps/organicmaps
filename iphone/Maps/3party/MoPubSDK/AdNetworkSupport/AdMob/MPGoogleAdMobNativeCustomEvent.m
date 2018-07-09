#import "MPGoogleAdMobNativeAdAdapter.h"
#import "MPGoogleAdMobNativeCustomEvent.h"
#import "MPInstanceProvider.h"
#import "MPLogging.h"
#import "MPNativeAd.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdError.h"
#import "MPNativeAdUtils.h"

static void MPGoogleLogInfo(NSString *message) {
  message = [[NSString alloc] initWithFormat:@"<Google Adapter> - %@", message];
  MPLogInfo(message);
}

/// Holds the preferred location of the AdChoices icon.
static GADAdChoicesPosition adChoicesPosition;

@interface MPGoogleAdMobNativeCustomEvent () <
    GADAdLoaderDelegate, GADNativeAppInstallAdLoaderDelegate, GADNativeContentAdLoaderDelegate>

/// GADAdLoader instance.
@property(nonatomic, strong) GADAdLoader *adLoader;

@end

@implementation MPGoogleAdMobNativeCustomEvent

+ (void)setAdChoicesPosition:(GADAdChoicesPosition)position {
  // Since this adapter only supports one position for all instances of native ads, publishers might
  // access this class method in multiple threads and try to set the position for various native
  // ads, so its better to use synchronized block to make "adChoicesPosition" variable thread safe.
  @synchronized([self class]) {
    adChoicesPosition = position;
  }
}

- (void)requestAdWithCustomEventInfo:(NSDictionary *)info {
  NSString *applicationID = [info objectForKey:@"appid"];
  if (applicationID) {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      [GADMobileAds configureWithApplicationID:applicationID];
    });
  }
  NSString *adUnitID = info[@"adunit"];
  if (!adUnitID) {
    [self.delegate nativeCustomEvent:self
            didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(
                                         @"Ad unit ID cannot be nil.")];
    return;
  }

  UIWindow *window = [UIApplication sharedApplication].keyWindow;
  UIViewController *rootViewController = window.rootViewController;
  while (rootViewController.presentedViewController) {
    rootViewController = rootViewController.presentedViewController;
  }
  GADRequest *request = [GADRequest request];
  request.requestAgent = @"MoPub";
  GADNativeAdImageAdLoaderOptions *nativeAdImageLoaderOptions =
      [[GADNativeAdImageAdLoaderOptions alloc] init];
  nativeAdImageLoaderOptions.disableImageLoading = YES;
  nativeAdImageLoaderOptions.shouldRequestMultipleImages = NO;
  nativeAdImageLoaderOptions.preferredImageOrientation =
      GADNativeAdImageAdLoaderOptionsOrientationAny;

  // In GADNativeAdViewAdOptions, the default preferredAdChoicesPosition is
  // GADAdChoicesPositionTopRightCorner.
  GADNativeAdViewAdOptions *nativeAdViewAdOptions = [[GADNativeAdViewAdOptions alloc] init];
  nativeAdViewAdOptions.preferredAdChoicesPosition = adChoicesPosition;

  self.adLoader = [[GADAdLoader alloc]
        initWithAdUnitID:adUnitID
      rootViewController:rootViewController
                 adTypes:@[ kGADAdLoaderAdTypeNativeAppInstall, kGADAdLoaderAdTypeNativeContent ]
                 options:@[ nativeAdImageLoaderOptions, nativeAdViewAdOptions ]];
  self.adLoader.delegate = self;
  [self.adLoader loadRequest:request];
}

#pragma mark GADAdLoaderDelegate implementation

- (void)adLoader:(GADAdLoader *)adLoader didFailToReceiveAdWithError:(GADRequestError *)error {
  [self.delegate nativeCustomEvent:self didFailToLoadAdWithError:error];
}

#pragma mark GADNativeAppInstallAdLoaderDelegate implementation

- (void)adLoader:(GADAdLoader *)adLoader
    didReceiveNativeAppInstallAd:(GADNativeAppInstallAd *)nativeAppInstallAd {
  if (![self isValidAppInstallAd:nativeAppInstallAd]) {
    MPGoogleLogInfo(@"App install ad is missing one or more required assets, failing the request");
    [self.delegate nativeCustomEvent:self
            didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(
                                         @"Missing one or more required assets.")];
    return;
  }

  MPGoogleAdMobNativeAdAdapter *adapter =
      [[MPGoogleAdMobNativeAdAdapter alloc] initWithAdMobNativeAppInstallAd:nativeAppInstallAd];
  MPNativeAd *moPubNativeAd = [[MPNativeAd alloc] initWithAdAdapter:adapter];

  NSMutableArray *imageURLs = [NSMutableArray array];

  if ([moPubNativeAd.properties[kAdIconImageKey] length]) {
    if (![MPNativeAdUtils addURLString:moPubNativeAd.properties[kAdIconImageKey]
                            toURLArray:imageURLs]) {
      [self.delegate nativeCustomEvent:self
              didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidImageURL()];
    }
  }

  if ([moPubNativeAd.properties[kAdMainImageKey] length]) {
    if (![MPNativeAdUtils addURLString:moPubNativeAd.properties[kAdMainImageKey]
                            toURLArray:imageURLs]) {
      [self.delegate nativeCustomEvent:self
              didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidImageURL()];
    }
  }

  [super precacheImagesWithURLs:imageURLs
                completionBlock:^(NSArray *errors) {
                  if (errors) {
                    [self.delegate nativeCustomEvent:self
                            didFailToLoadAdWithError:MPNativeAdNSErrorForImageDownloadFailure()];
                  } else {
                    [self.delegate nativeCustomEvent:self didLoadAd:moPubNativeAd];
                  }
                }];
}

#pragma mark GADNativeContentAdLoaderDelegate implementation

- (void)adLoader:(GADAdLoader *)adLoader
    didReceiveNativeContentAd:(GADNativeContentAd *)nativeContentAd {
  if (![self isValidContentAd:nativeContentAd]) {
    MPGoogleLogInfo(@"Content ad is missing one or more required assets, failing the request");
    [self.delegate nativeCustomEvent:self
            didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidAdServerResponse(
                                         @"Missing one or more required assets.")];
    return;
  }

  MPGoogleAdMobNativeAdAdapter *adapter =
      [[MPGoogleAdMobNativeAdAdapter alloc] initWithAdMobNativeContentAd:nativeContentAd];
  MPNativeAd *interfaceAd = [[MPNativeAd alloc] initWithAdAdapter:adapter];

  NSMutableArray *imageURLs = [NSMutableArray array];

  if ([interfaceAd.properties[kAdIconImageKey] length]) {
    if (![MPNativeAdUtils addURLString:interfaceAd.properties[kAdIconImageKey]
                            toURLArray:imageURLs]) {
      [self.delegate nativeCustomEvent:self
              didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidImageURL()];
    }
  }

  if ([interfaceAd.properties[kAdMainImageKey] length]) {
    if (![MPNativeAdUtils addURLString:interfaceAd.properties[kAdMainImageKey]
                            toURLArray:imageURLs]) {
      [self.delegate nativeCustomEvent:self
              didFailToLoadAdWithError:MPNativeAdNSErrorForInvalidImageURL()];
    }
  }

  [super precacheImagesWithURLs:imageURLs
                completionBlock:^(NSArray *errors) {
                  if (errors) {
                    [self.delegate nativeCustomEvent:self
                            didFailToLoadAdWithError:MPNativeAdNSErrorForImageDownloadFailure()];
                  } else {
                    [self.delegate nativeCustomEvent:self didLoadAd:interfaceAd];
                  }
                }];
}

#pragma mark - Private Methods

/// Checks the app install ad has required assets or not.
- (BOOL)isValidAppInstallAd:(GADNativeAppInstallAd *)appInstallAd {
  return (appInstallAd.headline && appInstallAd.body && appInstallAd.icon &&
          appInstallAd.images.count && appInstallAd.callToAction);
}

/// Checks the content ad has required assets or not.
- (BOOL)isValidContentAd:(GADNativeContentAd *)contentAd {
  return (contentAd.headline && contentAd.body && contentAd.logo && contentAd.images.count &&
          contentAd.callToAction);
}
@end
