
#import "MoreAppsVC.h"
#include "../../platform/platform.hpp"
#include "Framework.h"
#import "MoreAppsCell.h"
#import "UIKitCategories.h"
#import <iAd/iAd.h>
#import "AppInfo.h"
#import "Statistics.h"

@interface MoreAppsVC () <UITableViewDataSource, UITableViewDelegate, ADBannerViewDelegate>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) NSArray * guideRegions;
@property (nonatomic) NSArray * data;
@property (nonatomic) ADBannerView * bannerView;

@end

NSString * TitleMWM;
NSString * TitleGuides;
NSString * TitleAds;

using namespace::storage;

@implementation MoreAppsVC

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.title = NSLocalizedString(@"more_apps_title", nil);

  guides::GuidesManager & manager = GetFramework().GetGuidesManager();
  NSString * JSONPath = [NSString stringWithUTF8String:manager.GetDataFileFullPath().c_str()];
  NSData * JSONData = [NSData dataWithContentsOfFile:JSONPath];
  if (!JSONData)
  {
    JSONPath = [[NSBundle mainBundle] pathForResource:@"ios-guides" ofType:@"json"];
    JSONData = [NSData dataWithContentsOfFile:JSONPath];
  }
  NSDictionary * JSON = [NSJSONSerialization JSONObjectWithData:JSONData options:NSJSONReadingMutableContainers error:nil];
  NSMutableArray * guideRegions = [[NSMutableArray alloc] init];
  for (NSString * key in [JSON allKeys])
  {
    if ([key isEqualToString:@"version"])
      continue;
    NSDictionary * guide = JSON[key];
    NSString * language = [[NSLocale preferredLanguages] firstObject];
    NSString * message = guide[@"adMessages"][language];
    if (!message)
      message = guide[@"adMessages"][@"en"];
    NSString * country = [[message componentsSeparatedByString:@":"] firstObject];
    
    NSDictionary * info = @{@"Country" : country, @"URI" : guide[@"url"], @"GuideName" : key};
    [guideRegions addObject:info];
  }
  self.guideRegions = [guideRegions sortedArrayUsingDescriptors:@[[NSSortDescriptor sortDescriptorWithKey:@"Country" ascending:YES]]];

  TitleMWM = @"maps.me pro";
  TitleGuides = NSLocalizedString(@"more_apps_guides", nil);
  TitleAds = NSLocalizedString(@"more_apps_ads", nil);

  [self updateData];

  [self.view addSubview:self.tableView];

  [[Statistics instance] logEvent:@"MoreApps screen launched"];
}

- (void)updateData
{
  NSMutableArray * mData = [[NSMutableArray alloc] init];

  if (!GetPlatform().IsPro())
    [mData addObject:TitleMWM];

  [mData addObject:TitleGuides];

  if ([[AppInfo sharedInfo] featureAvailable:AppFeatureMoreAppsBanner])
  {
    if (self.bannerView.isBannerLoaded)
      [mData addObject:TitleAds];
  }

  self.data = mData;
}

- (UIImage *)iconImageWithImage:(UIImage *)image
{
  CGRect rect = CGRectMake(0, 0, image.size.width, image.size.height);
  UIGraphicsBeginImageContextWithOptions(rect.size, NO, [UIScreen mainScreen].scale);
  [[UIBezierPath bezierPathWithRoundedRect:rect cornerRadius:6.5] addClip];
  [image drawInRect:rect];
  UIImage * iconImage = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();

  return iconImage;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * title = self.data[indexPath.section];
  if ([title isEqualToString:TitleMWM] || [title isEqualToString:TitleGuides])
  {
    MoreAppsCell * cell = [tableView dequeueReusableCellWithIdentifier:@"Cell"];
    if (!cell)
      cell = [[MoreAppsCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Cell"];

    if ([title isEqualToString:TitleMWM])
    {
      cell.textLabel.text = TitleMWM;
      cell.imageView.image = [self iconImageWithImage:[UIImage imageNamed:@"MapsWithMeProIcon"]];
      [cell setFree:NO];
    }
    else if ([title isEqualToString:TitleGuides])
    {
      NSDictionary * guide = self.guideRegions[indexPath.row];
      cell.textLabel.text = guide[@"Country"];
      NSString * imageName = [[guide[@"GuideName"] stringByReplacingOccurrencesOfString:@" " withString:@""] lowercaseString];
      cell.imageView.image = [self iconImageWithImage:[UIImage imageNamed:imageName]];
      [cell setFree:YES];
    }
    return cell;
  }
  else if ([title isEqualToString:TitleAds])
  {
    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"iAdCell"];
    if (!cell)
    {
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"iAdCell"];
      self.bannerView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
      [cell.contentView addSubview:self.bannerView];
    }
    self.bannerView.midX = cell.width / 2;
    return cell;
  }
  return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  NSString * title = self.data[indexPath.section];
  if ([title isEqualToString:TitleMWM])
  {
    [[Statistics instance] logEvent:@"MoreApps MWM clicked"];
    [[UIApplication sharedApplication] openProVersionFrom:@"ios_more_apps"];
  }
  else if ([title isEqualToString:TitleGuides])
  {
    [[Statistics instance] logEvent:@"MoreApps guide clicked" withParameters:@{@"Guide" : self.guideRegions[indexPath.row][@"GuideName"]}];
    NSDictionary * guide = self.guideRegions[indexPath.row];
    [[UIApplication sharedApplication] openGuideWithName:guide[@"GuideName"] itunesURL:guide[@"URI"]];
  }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  NSString * title = self.data[section];
  if ([title isEqualToString:TitleMWM])
    return 1;
  else if ([title isEqualToString:TitleGuides])
    return [self.guideRegions count];
  else if ([title isEqualToString:TitleAds])
    return 1;
  return 0;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [self.data count];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return self.data[section];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * title = self.data[indexPath.section];
  if ([title isEqualToString:TitleMWM] || [title isEqualToString:TitleGuides])
    return 44;
  else
    return self.bannerView.height;
}

- (void)bannerViewDidLoadAd:(ADBannerView *)banner
{
  NSInteger section = [self.data indexOfObject:TitleAds];
  [self updateData];
  if (section == NSNotFound)
  {
    section = [self.data indexOfObject:TitleAds];
    [self.tableView insertSections:[NSIndexSet indexSetWithIndex:section] withRowAnimation:UITableViewRowAnimationAutomatic];
  }
}

- (void)bannerView:(ADBannerView *)banner didFailToReceiveAdWithError:(NSError *)error
{
  NSInteger section = [self.data indexOfObject:TitleAds];
  [self updateData];
  if (section != NSNotFound)
    [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:section] withRowAnimation:UITableViewRowAnimationAutomatic];
}

- (void)bannerViewActionDidFinish:(ADBannerView *)banner
{
  [[Statistics instance] logEvent:@"MoreApps banner clicked"];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
  if ([[AppInfo sharedInfo] featureAvailable:AppFeatureMoreAppsBanner])
  {
    self.bannerView.currentContentSizeIdentifier = UIInterfaceOrientationIsPortrait(self.interfaceOrientation) ? ADBannerContentSizeIdentifierPortrait : ADBannerContentSizeIdentifierLandscape;
    [self updateData];
    [self.tableView reloadData];
  }
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:self.view.bounds];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    _tableView.dataSource = self;
    _tableView.delegate = self;
  }
  return _tableView;
}

- (UIView *)bannerView
{
  if (!_bannerView)
  {
    _bannerView = [[ADBannerView alloc] initWithFrame:CGRectZero];
    _bannerView.currentContentSizeIdentifier = UIInterfaceOrientationIsPortrait(self.interfaceOrientation) ? ADBannerContentSizeIdentifierPortrait : ADBannerContentSizeIdentifierLandscape;
    _bannerView.delegate = self;
  }
  return _bannerView;
}

@end
