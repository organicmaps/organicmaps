#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMMapDownloaderExtendedDataSource.h"

#include "Framework.h"

using namespace storage;

@interface MWMMapDownloaderDataSource ()

@property (nonatomic, readwrite) BOOL needFullReload;

@end

@interface MWMMapDownloaderDefaultDataSource ()

@property (nonatomic, readonly) NSInteger downloadedCountrySection;

- (void)load;

@end

@interface MWMMapDownloaderExtendedDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * nearmeCountries;

@end

@implementation MWMMapDownloaderExtendedDataSource

- (void)load
{
  [super load];
  [self configNearMeSection];
}

- (void)reload
{
  NSInteger const closestCoutriesCountBeforeUpdate = self.nearmeCountries.count;

  [super reload];

  NSInteger const closestCoutriesCountAfterUpdate = self.nearmeCountries.count;
  if (closestCoutriesCountBeforeUpdate != closestCoutriesCountAfterUpdate &&
      (closestCoutriesCountBeforeUpdate == 0 || closestCoutriesCountAfterUpdate == 0))
    self.needFullReload = YES;
  if (self.needFullReload)
    return;
  [self.reloadSections shiftIndexesStartingAtIndex:0 by:self.nearmeSectionShift];
  if (closestCoutriesCountBeforeUpdate != closestCoutriesCountAfterUpdate)
    [self.reloadSections addIndex:self.nearmeSection];
}

- (void)configNearMeSection
{
  LocationManager * lm = MapsAppDelegate.theApp.locationManager;
  if (!lm.lastLocationIsValid)
    return;
  auto & countryInfoGetter = GetFramework().CountryInfoGetter();
  TCountriesVec closestCoutryIds;
  countryInfoGetter.GetRegionsCountryId(lm.lastLocation.mercator, closestCoutryIds);
  NSMutableArray<NSString *> * nearmeCountries = [@[] mutableCopy];
  auto const & s = GetFramework().Storage();
  for (auto const & countryId : closestCoutryIds)
  {
    NodeStatuses nodeStatuses{};
    s.GetNodeStatuses(countryId, nodeStatuses);
    if (nodeStatuses.m_status == NodeStatus::NotDownloaded)
      [nearmeCountries addObject:@(countryId.c_str())];
  }
  self.nearmeCountries = nearmeCountries;
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [super numberOfSectionsInTableView:tableView] + self.nearmeSectionShift;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == self.nearmeSection)
    return self.nearmeCountries.count;
  return [super tableView:tableView numberOfRowsInSection:section - self.nearmeSectionShift];
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return [super tableView:tableView sectionForSectionIndexTitle:title atIndex:index] + self.nearmeSectionShift;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == self.nearmeSection)
    return L(@"downloader_near_me_subtitle");
  return [super tableView:tableView titleForHeaderInSection:section - self.nearmeSectionShift];
}

#pragma mark - MWMMapDownloaderDataSource

- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const row = indexPath.row;
  NSInteger const section = indexPath.section;
  if (section == self.nearmeSection)
    return self.nearmeCountries[row];
  return [super countryIdForIndexPath:[NSIndexPath indexPathForRow:row inSection:section - self.nearmeSectionShift]];
}

#pragma mark - Properties

- (NSInteger)nearmeSectionShift
{
  return (self.nearmeCountries.count != 0 ? self.nearmeSection + 1 : 0);
}

- (NSInteger)nearmeSection
{
  return self.nearmeCountries.count != 0 ? 0 : NSNotFound;
}

@end
