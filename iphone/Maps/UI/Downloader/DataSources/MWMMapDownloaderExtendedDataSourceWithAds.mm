#import "MWMMapDownloaderExtendedDataSourceWithAds.h"
#import "MWMMapDownloaderAdsTableViewCell.h"
#import "MWMMyTarget.h"

#include "Framework.h"

namespace
{
auto constexpr extraSection = MWMMapDownloaderDataSourceExtraSection::Ads;
}  // namespace

@interface MWMMapDownloaderExtendedDataSource ()

- (void)load;
- (void)addExtraSection:(MWMMapDownloaderDataSourceExtraSection)extraSection;
- (void)removeExtraSection:(MWMMapDownloaderDataSourceExtraSection)extraSection;
- (BOOL)isExtraSection:(MWMMapDownloaderDataSourceExtraSection)extraSection
               atIndex:(NSInteger)sectionIndex;

@end

@implementation MWMMapDownloaderExtendedDataSourceWithAds

- (void)load
{
  [super load];
  if (self.mode == MWMMapDownloaderModeAvailable)
    [self configAdsSection];
}

- (void)configAdsSection
{
  [self removeExtraSection:extraSection];
  if ([UIColor isNightMode] || !GetFramework().GetStorage().HaveDownloadedCountries())
    return;
  if ([MWMMyTarget manager].bannersCount != 0)
    [self addExtraSection:extraSection];
}

- (MTRGAppwallBannerAdView *)viewForBannerAtIndex:(NSUInteger)index
{
  MTRGAppwallBannerAdView * adView = [[MTRGAppwallBannerAdView alloc] initWithDelegate:nil];

  adView.paddings = {.top = 12, .left = 16, .bottom = 12, .right = 16};
  adView.touchColor = [UIColor white];
  adView.normalColor = [UIColor white];
  adView.iconSize = {24, 24};
  UIEdgeInsets iconMargins = adView.iconMargins;
  iconMargins.right += 8;
  adView.iconMargins = iconMargins;
  adView.showTopBorder = NO;
  adView.showGotoAppIcon = NO;
  adView.showRating = NO;
  adView.showStatusIcon = NO;
  adView.showBubbleIcon = NO;
  adView.showCoins = NO;
  adView.showCrossNotifIcon = NO;
  adView.titleFont = [UIFont regular17];
  adView.titleColor = [UIColor blackPrimaryText];
  adView.descriptionColor = [UIColor blackSecondaryText];
  adView.descriptionFont = [UIFont regular13];

  [adView setAppWallBanner:[[MWMMyTarget manager] bannerAtIndex:index]];

  return adView;
}

#pragma mark - MWMMapDownloaderDataSource

- (Class)cellClassForIndexPath:(NSIndexPath *)indexPath
{
  if ([self isExtraSection:extraSection atIndex:indexPath.section])
    return [MWMMapDownloaderAdsTableViewCell class];
  return [super cellClassForIndexPath:indexPath];
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isExtraSection:extraSection atIndex:indexPath.section])
    return NO;
  return [super tableView:tableView canEditRowAtIndexPath:indexPath];
}

#pragma mark - Fill cells with data

- (void)fillCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath
{
  if (![self isExtraSection:extraSection atIndex:indexPath.section])
    return [super fillCell:cell atIndexPath:indexPath];

  MWMMapDownloaderAdsTableViewCell * tCell = static_cast<MWMMapDownloaderAdsTableViewCell *>(cell);
  tCell.adView = [self viewForBannerAtIndex:indexPath.row];
  [[MWMMyTarget manager] handleBannerShowAtIndex:indexPath.row];
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if ([self isExtraSection:extraSection atIndex:section])
    return [MWMMyTarget manager].bannersCount;
  return [super tableView:tableView numberOfRowsInSection:section];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if ([self isExtraSection:extraSection atIndex:section])
    return L(@"MY.COM");
  return [super tableView:tableView titleForHeaderInSection:section];
}

@end
