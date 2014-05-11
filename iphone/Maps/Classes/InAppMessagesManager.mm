
#import "InAppMessagesManager.h"
#import "MPInterstitialAdController.h"
#import "MPInterstitialViewController.h"
#import "MPAdView.h"
#import "AppInfo.h"
#import "Statistics.h"
#import "InterstitialView.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"

@interface BannerImageView : UIImageView

@property (nonatomic) NSString * imageType;

@end

@implementation BannerImageView

@end


typedef void (^CompletionBlock)(NSArray * images, NSString * imageType);

NSString * const InAppMessageInterstitial = @"InAppMessageInterstitial";
NSString * const InAppMessageBanner = @"InAppMessageBanner";

NSString * const MoPubImageType = @"MoPub";
NSString * const MWMProVersionPrefix = @"MWMPro";

@interface InAppMessagesManager () <MPInterstitialAdControllerDelegate, MPAdViewDelegate, InterstitialViewDelegate>

@property (nonatomic) NSMutableDictionary * observers;

@property (nonatomic) MPInterstitialAdController * interstitialAd;
@property (nonatomic) MPAdView * moPubBanner;
@property (nonatomic) BannerImageView * banner;

@end

@implementation InAppMessagesManager

#pragma mark - Public methods

- (instancetype)init
{
  self = [super init];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];

  return self;
}

+ (instancetype)sharedManager
{
  static InAppMessagesManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] init];
  });
  return manager;
}

- (void)registerController:(UIViewController *)vc forMessage:(NSString *)messageName
{
  self.currentController = vc;

  __weak UIViewController * weakVC = vc;
  self.observers[messageName] = weakVC;
}

- (void)unregisterControllerFromAllMessages:(UIViewController *)vc
{
  for (NSString * key in [self.observers allKeys])
  {
    if (self.observers[key] == vc)
      [self.observers removeObjectForKey:key];
  }
  [self cleanBanners];
}

- (void)triggerMessage:(NSString *)messageName
{
  UIViewController * vc = self.observers[messageName];
  if (!(vc && [self shouldShowMessage:messageName]) || [MapsAppDelegate theApp].m_mapViewController.popoverVC)
    return;

  [self findVariantForMessage:messageName completion:^(NSArray * images, NSString * imageType){
    if ([messageName isEqualToString:InAppMessageInterstitial])
    {
      if ([imageType isEqualToString:MoPubImageType])
      {
#ifdef OMIM_LITE
        if (self.interstitialAd.ready)
        {
          [self.interstitialAd showFromViewController:vc];
        }
        else
        {
          NSDictionary * imageVariants = [[AppInfo sharedInfo] featureValue:[self appFeatureNameForMessage:messageName] forKey:@"Variants"];
          NSString * unitId = imageVariants[imageType][@"UnitId"];
          self.interstitialAd = [MPInterstitialAdController interstitialAdControllerForAdUnitId:unitId];
          self.interstitialAd.delegate = self;
          [self.interstitialAd loadAd];
        }
#endif
      }
      else if ([images count])
      {
        InterstitialView * interstitial = [[InterstitialView alloc] initWithImages:images inAppMessageName:messageName imageType:imageType delegate:self];
        [interstitial show];
      }
    }
    else if ([messageName isEqualToString:InAppMessageBanner])
    {
      [self cleanBanners];

      UIView * bannerView;
      if ([imageType isEqualToString:MoPubImageType])
      {
#ifdef OMIM_LITE
        NSDictionary * imageVariants = [[AppInfo sharedInfo] featureValue:[self appFeatureNameForMessage:messageName] forKey:@"Variants"];
        NSString * unitId = imageVariants[imageType][@"UnitId"];
        MPAdView * moPubBanner = [[MPAdView alloc] initWithAdUnitId:unitId size:MOPUB_BANNER_SIZE];
        moPubBanner.hidden = YES;
        moPubBanner.delegate = self;
        [moPubBanner startAutomaticallyRefreshingContents];
        [moPubBanner loadAd];
        self.moPubBanner = moPubBanner;
        bannerView = moPubBanner;
#endif
      }
      else if ([images count])
      {
        BannerImageView * banner = [[BannerImageView alloc] initWithImage:[images firstObject]];
        banner.imageType = imageType;
        banner.userInteractionEnabled = YES;
        UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(bannerTap:)];
        [banner addGestureRecognizer:tap];
        self.banner = banner;
        bannerView = banner;

        NSString * eventName = [NSString stringWithFormat:@"%@ showed", InAppMessageBanner];
        [[Statistics instance] logInAppMessageEvent:eventName imageType:imageType];
      }

      if (bannerView)
      {
        if (!SYSTEM_VERSION_IS_LESS_THAN(@"7"))
          bannerView.minY = 20;
        bannerView.midX = vc.view.width / 2;
        bannerView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
        [vc.view addSubview:bannerView];
      }
    }
  }];
}

#pragma mark - Private methods

- (void)cleanBanners
{
  [self.banner removeFromSuperview];
  self.banner = nil;
  [self.moPubBanner removeFromSuperview];
  self.moPubBanner.delegate = nil;
  self.moPubBanner = nil;
}

- (BOOL)shouldShowMessage:(NSString *)messageName
{
  NSString * featureName = [self appFeatureNameForMessage:messageName];

  BOOL featureAvailable = [[AppInfo sharedInfo] featureAvailable:featureName];

  NSTimeInterval period = [[[AppInfo sharedInfo] featureValue:featureName forKey:@"Period"] doubleValue];
  NSDate * lastShowDate = [self lastShowTimeOfMessage:messageName];
  BOOL showTime = lastShowDate ? [[NSDate date] timeIntervalSinceDate:lastShowDate] > period : YES;

  UIViewController * vc = self.observers[messageName];
  BOOL isCurrentVC = YES;
  if (vc)
    isCurrentVC = self.currentController == vc;

  return featureAvailable && showTime && isCurrentVC;
}

- (void)applicationWillEnterForeground:(NSNotification *)notification
{
  [self performAfterDelay:0.7 block:^{
    for (NSString * messageName in [self.observers allKeys])
    {
      [self triggerMessage:messageName];
    }
  }];
}

- (void)findVariantForMessage:(NSString *)messageName completion:(CompletionBlock)block
{
  AppInfo * info = [AppInfo sharedInfo];
  NSDictionary * imageVariants = [info featureValue:[self appFeatureNameForMessage:messageName] forKey:@"Variants"];

// checking which types are actual
  NSMutableDictionary * actualImageVariants = [[NSMutableDictionary alloc] init];
  for (NSString * type in [imageVariants allKeys])
  {
    NSDictionary * imageParameters = imageVariants[type];

    BOOL inTimeInterval = YES;
    NSArray * timeLimitInterval = imageParameters[@"TimeLimit"];
    if ([timeLimitInterval count] == 2)
    {
      NSTimeInterval timeValue = [[NSDate date] timeIntervalSinceDate:info.firstLaunchDate];
      inTimeInterval = timeValue >= [timeLimitInterval[0] doubleValue] && timeValue <= [timeLimitInterval[1] doubleValue];
    }

    BOOL inLaunchInterval = YES;
    NSArray * launchCountInterval = imageParameters[@"LaunchCount"];
    if ([launchCountInterval count] == 2)
      inLaunchInterval = info.launchCount >= [launchCountInterval[0] doubleValue] && info.launchCount <= [launchCountInterval[1] doubleValue];

    BOOL shouldShow = [imageParameters[@"Online"] boolValue] ? [info.reachability isReachable] : YES;

    NSString * idiom = imageParameters[@"Idiom"];
    BOOL rightIdiom = YES;
    if ([idiom isEqualToString:@"Pad"] && !IPAD)
      rightIdiom = NO;
    else if ([idiom isEqualToString:@"Phone"] && IPAD)
      rightIdiom = NO;

    if (inTimeInterval && inLaunchInterval && shouldShow && rightIdiom)
      actualImageVariants[type] = imageVariants[type];
  }

// getting random type from actual
  NSInteger chancesSum = 0;
  for (NSString * type in [actualImageVariants allKeys])
  {
    chancesSum += [actualImageVariants[type][@"Chance"] integerValue];
  }
  if (!chancesSum)
  {
    block(nil, nil);
    return;
  }

  NSInteger rand = arc4random() % chancesSum;
  NSString * imageType;
  NSInteger currentChance = 0;
  for (NSString * type in [actualImageVariants allKeys])
  {
    currentChance += [actualImageVariants[type][@"Chance"] integerValue];
    if (currentChance > rand)
    {
      imageType = type;
      break;
    }
  }

  if ([imageType isEqualToString:MoPubImageType])
    block(nil, imageType);
  else
    [self downloadAndSaveImagesForMessage:messageName imageType:imageType completion:block];
}

- (void)downloadAndSaveImagesForMessage:(NSString *)messageName imageType:(NSString *)imageType completion:(CompletionBlock)block
{
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
    NSMutableArray * images = [[NSMutableArray alloc] init];
    NSString * language = [self languageForMessage:messageName imageType:imageType];
    if (!language)
    {
      block(nil, imageType);
      return;
    }
    NSArray * imageNames = [self imageNamesForMessage:messageName imageType:imageType];
    for (NSString * imageName in imageNames)
    {
      UIImage * image = [self getImageWithName:imageName language:language];
      if (image)
        [images addObject:image];
    }
    dispatch_async(dispatch_get_main_queue(), ^{
      block(images, imageType);
    });
  });
}

- (UIImage *)getImageWithName:(NSString *)imageName language:(NSString *)language
{
  NSString * imagePath = [self imagePathForImageWithName:imageName language:language];
  UIImage * image = [[UIImage alloc] initWithContentsOfFile:imagePath];
  if (image)
  {
    return image;
  }
  else
  {
    NSString * urlString = [NSString stringWithFormat:@"http://application.server/ios/messages/%@/%@", language, imageName];
    NSURLRequest * request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    NSHTTPURLResponse * response;
    NSData * data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:nil];
    if ([response statusCode] == 200)
    {
      [data writeToFile:imagePath atomically:YES];
      return [UIImage imageWithData:data scale:[UIScreen mainScreen].scale];
    }
  }
  return nil;
}

- (NSArray *)imageNamesForMessage:(NSString *)messageName imageType:(NSString *)imageType
{
  NSString * idiom = IPAD ? @"Ipad" : @"Iphone";
  if ([messageName isEqualToString:InAppMessageInterstitial])
  {
    CGSize screenSize = [UIScreen mainScreen].bounds.size;
    NSInteger max = MAX(screenSize.width, screenSize.height);
    NSInteger min = MIN(screenSize.width, screenSize.height);
    NSMutableArray * imageNames = [[NSMutableArray alloc] init];
    [imageNames addObject:[self imageNameForMessage:messageName imageType:imageType idiom:idiom height:max]];
    if (IPAD)
      [imageNames addObject:[self imageNameForMessage:messageName imageType:imageType idiom:idiom height:min]];

    return imageNames;
  }
  else if ([messageName isEqualToString:InAppMessageBanner])
  {
    return @[[self imageNameForMessage:messageName imageType:imageType idiom:idiom height:0]];
  }
  return nil;
}

- (NSString *)imageNameForMessage:(NSString *)messageName imageType:(NSString *)imageType idiom:(NSString *)idiom height:(NSInteger)height
{
  NSString * scaleString = [UIScreen mainScreen].scale == 2 ? @"@2x" : @"";
  NSString * heightString = height ? [NSString stringWithFormat:@"%i", height] : @"";
  return [NSString stringWithFormat:@"%@_%@_%@%@%@.png", messageName, imageType, idiom, heightString, scaleString];
}

- (NSString *)imagePathForImageWithName:(NSString *)imageName language:(NSString *)language
{
  NSString * rootPath = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) firstObject];
  NSString * languagePath = [rootPath stringByAppendingPathComponent:language];
  if (![[NSFileManager defaultManager] fileExistsAtPath:languagePath])
    [[NSFileManager defaultManager] createDirectoryAtPath:languagePath withIntermediateDirectories:NO attributes:nil error:nil];
  return [languagePath stringByAppendingPathComponent:imageName];
}

- (NSString *)languageForMessage:(NSString *)messageName imageType:(NSString *)imageType
{
  NSDictionary * imageParameters = [[AppInfo sharedInfo] featureValue:[self appFeatureNameForMessage:messageName] forKey:@"Variants"];
  NSArray * languages = imageParameters[imageType][@"Languages"];
  // if message does not use localized text we should pass '*' instead of language name
  // then, message image must be in /all directory on server
  if ([languages containsObject:[[NSLocale preferredLanguages] firstObject]])
    return [[NSLocale preferredLanguages] firstObject];
  if ([languages containsObject:@"*"])
    return @"all";
  return nil;
}

- (NSString *)appFeatureNameForMessage:(NSString *)messageName
{
  if ([messageName isEqualToString:InAppMessageInterstitial])
    return AppFeatureInterstitial;
  else if ([messageName isEqualToString:InAppMessageBanner])
    return AppFeatureBanner;
  else
    return nil;
}

- (NSDate *)lastShowTimeOfMessage:(NSString *)messageName
{
  NSString * key = [NSString stringWithFormat:@"Show%@", messageName];
  return [[NSUserDefaults standardUserDefaults] objectForKey:key];
}

- (void)updateShowTimeOfMessage:(NSString *)messageName
{
  NSString * key = [NSString stringWithFormat:@"Show%@", messageName];
  [[NSUserDefaults standardUserDefaults] setObject:[NSDate date] forKey:key];
  [[NSUserDefaults standardUserDefaults] synchronize];
}

#pragma mark - InterstitialView callbacks

- (void)interstitialViewWillOpen:(InterstitialView *)interstitial
{
  [self updateShowTimeOfMessage:interstitial.inAppMessageName];

  NSString * eventName = [NSString stringWithFormat:@"%@ showed", interstitial.inAppMessageName];
  [[Statistics instance] logInAppMessageEvent:eventName imageType:interstitial.imageType];
}

- (void)interstitialView:(InterstitialView *)interstitial didCloseWithResult:(InterstitialViewResult)result
{
  NSString * eventName;
  if (result == InterstitialViewResultClicked)
    eventName = [NSString stringWithFormat:@"%@ clicked", interstitial.inAppMessageName];
  else if (result == InterstitialViewResultDismissed)
    eventName = [NSString stringWithFormat:@"%@ dismissed", interstitial.inAppMessageName];

  [[Statistics instance] logInAppMessageEvent:eventName imageType:interstitial.imageType];

  if (result == InterstitialViewResultClicked)
  {
    if ([interstitial.imageType hasPrefix:MWMProVersionPrefix])
    {
      [[UIApplication sharedApplication] openProVersionFrom:@"ios_interstitial"];
    }
    else
    {
      NSDictionary * imageVariants = [[AppInfo sharedInfo] featureValue:AppFeatureBanner forKey:@"Variants"];
      NSString * url = imageVariants[interstitial.imageType][@"URI"];
      if (url)
        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
    }
  }
}

#pragma mark - BannerImageView callbacks

- (void)bannerTap:(UITapGestureRecognizer *)sender
{
  NSString * imageType = ((BannerImageView *)sender.view).imageType;
  NSString * eventName = [NSString stringWithFormat:@"%@ clicked", InAppMessageBanner];
  [[Statistics instance] logInAppMessageEvent:eventName imageType:imageType];

  if ([imageType hasPrefix:MWMProVersionPrefix])
  {
    [[UIApplication sharedApplication] openProVersionFrom:@"ios_banner"];
  }
  else
  {
    NSDictionary * imageVariants = [[AppInfo sharedInfo] featureValue:AppFeatureBanner forKey:@"Variants"];
    NSString * url = imageVariants[imageType][@"URI"];
    if (url)
      [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
  }
}

#pragma mark - MPInterstitialAdController callbacks

- (void)interstitialDidLoadAd:(MPInterstitialAdController *)interstitial
{
  UIViewController * vc = self.observers[InAppMessageInterstitial];
  [interstitial showFromViewController:vc];
}

- (void)interstitialWillAppear:(MPInterstitialAdController *)interstitial
{
  [self updateShowTimeOfMessage:InAppMessageInterstitial];
  NSString * eventName = [NSString stringWithFormat:@"%@ showed", InAppMessageInterstitial];
  [[Statistics instance] logInAppMessageEvent:eventName imageType:MoPubImageType];
}

- (void)interstitialDidDisappear:(MPInterstitialAdController *)interstitial
{
  NSString * eventName = [NSString stringWithFormat:@"%@ dismissed", InAppMessageInterstitial];
  [[Statistics instance] logInAppMessageEvent:eventName imageType:MoPubImageType];
}

#pragma mark - MPAdView callbacks

- (void)adViewDidLoadAd:(MPAdView *)view
{
  view.hidden = NO;
  NSString * eventName = [NSString stringWithFormat:@"%@ showed", InAppMessageBanner];
  [[Statistics instance] logInAppMessageEvent:eventName imageType:MoPubImageType];
}

- (void)willPresentModalViewForAd:(MPAdView *)view
{
  NSString * eventName = [NSString stringWithFormat:@"%@ clicked", InAppMessageBanner];
  [[Statistics instance] logInAppMessageEvent:eventName imageType:MoPubImageType];
}

- (void)adViewDidFailToLoadAd:(MPAdView *)view
{
  view.hidden = YES;
}

- (UIViewController *)viewControllerForPresentingModalView
{
  return self.observers[InAppMessageBanner];
}

#pragma mark - Other

- (NSMutableDictionary *)observers
{
  if (!_observers)
    _observers = [[NSMutableDictionary alloc] init];
  return _observers;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
