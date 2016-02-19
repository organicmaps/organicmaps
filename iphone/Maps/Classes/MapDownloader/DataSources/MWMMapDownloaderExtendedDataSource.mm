#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMMapDownloaderExtendedDataSource.h"

#include "Framework.h"

using namespace storage;

@interface MWMMapDownloaderExtendedDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * closestCoutryIds;
@property (copy, nonatomic) NSArray<NSString *> * downloadedCoutryIds;
@property (nonatomic) NSInteger baseSectionShift;
@property (nonatomic) NSInteger closestCountriesSection;
@property (nonatomic) NSInteger downloadedCountriesSection;

@end

@implementation MWMMapDownloaderExtendedDataSource

- (instancetype)initForRootCountryId:(storage::TCountryId)countryId delegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super initForRootCountryId:countryId delegate:delegate];
  if (self)
  {
    self.baseSectionShift = 0;
    [self configNearMeSection];
    [self configDownloadedSection];
  }
  return self;
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
  for (auto const & countryId : closestCoutryIds)
    [nsClosestCoutryIds addObject:@(countryId.c_str())];
  if (nsClosestCoutryIds.count != 0)
  {
    self.closestCoutryIds = nsClosestCoutryIds;
    self.closestCountriesSection = self.baseSectionShift++;
  }
}

- (void)configDownloadedSection
{
  self.downloadedCoutryIds = nil;
  self.downloadedCountriesSection = NSNotFound;
  auto const & s = GetFramework().Storage();
  TCountriesVec downloadedCoutryIds;
  s.GetDownloadedChildren(self.parentCountryId, downloadedCoutryIds);
  NSMutableArray<NSString *> * nsDownloadedCoutryIds = [@[] mutableCopy];
  for (auto const & countryId : downloadedCoutryIds)
    [nsDownloadedCoutryIds addObject:@(countryId.c_str())];
  if (nsDownloadedCoutryIds.count != 0)
  {
    self.downloadedCoutryIds = nsDownloadedCoutryIds;
    self.downloadedCountriesSection = self.baseSectionShift++;
  }
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  NSInteger const numberOfSections = [super numberOfSectionsInTableView:tableView];
  return numberOfSections + self.baseSectionShift;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section >= self.baseSectionShift)
    return [super tableView:tableView numberOfRowsInSection:section - self.baseSectionShift];
  if (section == self.closestCountriesSection)
    return self.closestCoutryIds.count;
  if (section == self.downloadedCountriesSection)
    return self.downloadedCoutryIds.count;
  NSAssert(NO, @"Invalid section");
  return 0;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return [super tableView:tableView sectionForSectionIndexTitle:title atIndex:index] + self.baseSectionShift;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section >= self.baseSectionShift)
    return [super tableView:tableView titleForHeaderInSection:section - self.baseSectionShift];
  if (section == self.closestCountriesSection)
    return L(@"search_mode_nearme");
  if (section == self.downloadedCountriesSection)
    return L(@"downloader_downloaded_maps");
  NSAssert(NO, @"Invalid section");
  return nil;
}

#pragma mark - MWMMapDownloaderDataSourceProtocol

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const row = indexPath.row;
  NSInteger const section = indexPath.section;
  if (section >= self.baseSectionShift)
    return [super countryIdForIndexPath:[NSIndexPath indexPathForRow:row inSection:section - self.baseSectionShift]];
  if (section == self.closestCountriesSection)
    return self.closestCoutryIds[row].UTF8String;
  if (section == self.downloadedCountriesSection)
    return self.downloadedCoutryIds[row].UTF8String;
  NSAssert(NO, @"Invalid section");
  return kInvalidCountryId;
}

@end
