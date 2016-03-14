#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMMapDownloaderExtendedDataSource.h"

#include "Framework.h"

using namespace storage;

@interface MWMMapDownloaderDefaultDataSource ()

@property (nonatomic, readonly) NSInteger downloadedCountrySection;
@property (nonatomic, readwrite) BOOL needFullReload;

- (void)load;

@end

@interface MWMMapDownloaderExtendedDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * closestCoutryIds;

@property (nonatomic) NSInteger closestCountriesSection;
@property (nonatomic, readonly) NSInteger closestCountriesSectionShift;

@property (nonatomic) BOOL needReloadClosestCountriesSection;

@end

@implementation MWMMapDownloaderExtendedDataSource

- (std::vector<NSInteger>)getReloadSections
{
  std::vector<NSInteger> sections = [super getReloadSections];
  if (!self.haveClosestCountries)
    return sections;
  for (auto & section : sections)
    section += self.closestCountriesSectionShift;
  if (self.needReloadClosestCountriesSection)
    sections.push_back(self.closestCountriesSection);
  return sections;
}

- (void)load
{
  [super load];
  [self configNearMeSection];
}

- (void)reload
{
  NSSet<NSString *> * closestCoutryIds = [NSSet setWithArray:self.closestCoutryIds];
  [super reload];

  self.needReloadClosestCountriesSection =
      ![closestCoutryIds isEqualToSet:[NSSet setWithArray:self.closestCoutryIds]];
  self.needFullReload |= self.needReloadClosestCountriesSection && self.closestCoutryIds.count == 0;
}

- (void)configNearMeSection
{
  self.closestCoutryIds = nil;
  self.closestCountriesSection = NSNotFound;
  LocationManager * lm = MapsAppDelegate.theApp.m_locationManager;
  if (!lm.lastLocationIsValid)
    return;
  auto & countryInfoGetter = GetFramework().CountryInfoGetter();
  TCountriesVec closestCoutryIds;
  countryInfoGetter.GetRegionsCountryId(lm.lastLocation.mercator, closestCoutryIds);
  NSMutableArray<NSString *> * nsClosestCoutryIds = [@[] mutableCopy];
  auto const & s = GetFramework().Storage();
  for (auto const & countryId : closestCoutryIds)
  {
    NodeStatuses nodeStatuses{};
    s.GetNodeStatuses(countryId, nodeStatuses);
    if (nodeStatuses.m_status == NodeStatus::NotDownloaded)
      [nsClosestCoutryIds addObject:@(countryId.c_str())];
  }
  if (nsClosestCoutryIds.count != 0)
  {
    self.closestCoutryIds = nsClosestCoutryIds;
    self.closestCountriesSection = 0;
  }
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [super numberOfSectionsInTableView:tableView] + self.closestCountriesSectionShift;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == self.closestCountriesSection)
    return self.closestCoutryIds.count;
  return [super tableView:tableView numberOfRowsInSection:section - self.closestCountriesSectionShift];
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return [super tableView:tableView sectionForSectionIndexTitle:title atIndex:index] + self.closestCountriesSectionShift;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == self.closestCountriesSection)
    return L(@"search_mode_nearme");
  return [super tableView:tableView titleForHeaderInSection:section - self.closestCountriesSectionShift];
}

#pragma mark - MWMMapDownloaderDataSource

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const row = indexPath.row;
  NSInteger const section = indexPath.section;
  if (section == self.closestCountriesSection)
    return self.closestCoutryIds[row].UTF8String;
  return [super countryIdForIndexPath:[NSIndexPath indexPathForRow:row inSection:section - self.closestCountriesSectionShift]];
}

#pragma mark - Properties

- (NSInteger)closestCountriesSectionShift
{
  return (self.haveClosestCountries ? self.closestCountriesSection + 1 : 0);
}

- (BOOL)haveClosestCountries
{
  return (self.closestCountriesSection != NSNotFound);
}

@end
