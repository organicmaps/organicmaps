#import "MapsAppDelegate.h"
#import "MWMLocationManager.h"
#import "MWMMapDownloaderExtendedDataSource.h"

#include "Framework.h"

using namespace storage;

namespace
{
auto constexpr extraSection = MWMMapDownloaderDataSourceExtraSection::NearMe;
}  // namespace

@interface MWMMapDownloaderDefaultDataSource ()

@property (nonatomic, readonly) NSInteger downloadedCountrySection;

- (void)load;

@end

@interface MWMMapDownloaderExtendedDataSource ()

@property(copy, nonatomic) NSArray<NSString *> * nearmeCountries;

@end

@implementation MWMMapDownloaderExtendedDataSource
{
  vector<MWMMapDownloaderDataSourceExtraSection> m_extraSections;
}

- (void)load
{
  [super load];
  if (self.mode == mwm::DownloaderMode::Available)
    [self configNearMeSection];
}

- (void)configNearMeSection
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (!lastLocation)
    return;
  auto & countryInfoGetter = GetFramework().GetCountryInfoGetter();
  TCountriesVec closestCoutryIds;
  countryInfoGetter.GetRegionsCountryId(lastLocation.mercator, closestCoutryIds);
  NSMutableArray<NSString *> * nearmeCountries = [@[] mutableCopy];
  for (auto const & countryId : closestCoutryIds)
    [nearmeCountries addObject:@(countryId.c_str())];
  self.nearmeCountries = nearmeCountries;
  if (nearmeCountries.count != 0)
    [self addExtraSection:extraSection];
}

- (void)addExtraSection:(MWMMapDownloaderDataSourceExtraSection)extraSection
{
  m_extraSections.push_back(extraSection);
}

- (BOOL)isExtraSection:(MWMMapDownloaderDataSourceExtraSection)extraSection
               atIndex:(NSInteger)sectionIndex
{
  if (sectionIndex >= m_extraSections.size())
    return NO;
  return m_extraSections[sectionIndex] == extraSection;
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [super numberOfSectionsInTableView:tableView] + m_extraSections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if ([self isExtraSection:extraSection atIndex:section])
    return self.nearmeCountries.count;
  return [super tableView:tableView numberOfRowsInSection:section - m_extraSections.size()];
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return [super tableView:tableView sectionForSectionIndexTitle:title atIndex:index] +
         m_extraSections.size();
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if ([self isExtraSection:extraSection atIndex:section])
    return L(@"downloader_near_me_subtitle");
  return [super tableView:tableView titleForHeaderInSection:section - m_extraSections.size()];
}

#pragma mark - MWMMapDownloaderDataSource

- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const row = indexPath.row;
  NSInteger const section = indexPath.section;
  if ([self isExtraSection:extraSection atIndex:section])
    return self.nearmeCountries[row];
  NSAssert(section >= m_extraSections.size(), @"Invalid section");
  return
      [super countryIdForIndexPath:[NSIndexPath indexPathForRow:row
                                                      inSection:section - m_extraSections.size()]];
}

@end
