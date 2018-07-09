#import "MWMMapDownloaderDefaultDataSource.h"
#import "MWMCommon.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "SwiftBridge.h"

#include "Framework.h"

namespace
{
auto compareStrings = ^NSComparisonResult(NSString * s1, NSString * s2) {
  return [s1 compare:s2
             options:NSCaseInsensitiveSearch
               range:{ 0, s1.length }
              locale:NSLocale.currentLocale];
};

auto compareLocalNames = ^NSComparisonResult(NSString * s1, NSString * s2)
{
  auto const & s = GetFramework().GetStorage();
  string const l1 = s.GetNodeLocalName(s1.UTF8String);
  string const l2 = s.GetNodeLocalName(s2.UTF8String);
  return compareStrings(@(l1.c_str()), @(l2.c_str()));
};
} // namespace

using namespace storage;

@interface MWMMapDownloaderDefaultDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * indexes;
@property (copy, nonatomic) NSDictionary<NSString *, NSArray<NSString *> *> * availableCountries;
@property (copy, nonatomic) NSArray<NSString *> * downloadedCountries;

@end

@implementation MWMMapDownloaderDefaultDataSource
{
  TCountryId m_parentId;
}

@synthesize isParentRoot = _isParentRoot;

- (instancetype)
initForRootCountryId:(NSString *)countryId
            delegate:
                (id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate
                mode:(MWMMapDownloaderMode)mode
{
  self = [super initWithDelegate:delegate mode:mode];
  if (self)
  {
    m_parentId = countryId.UTF8String;
    _isParentRoot = (m_parentId == GetFramework().GetStorage().GetRootId());
    [self load];
  }
  return self;
}

- (void)load
{
  auto const & s = GetFramework().GetStorage();
  TCountriesVec downloadedChildren;
  TCountriesVec availableChildren;
  s.GetChildrenInGroups(m_parentId, downloadedChildren, availableChildren, true /* keepAvailableChildren */);
  if (self.mode == MWMMapDownloaderModeAvailable)
  {
    self.downloadedCountries = nil;
    [self configAvailableSections:availableChildren];
  }
  else
  {
    self.indexes = nil;
    self.availableCountries = nil;
    [self configDownloadedSection:downloadedChildren];
  }
}

- (void)configAvailableSections:(TCountriesVec const &)availableChildren
{
  NSMutableSet<NSString *> * indexSet = [NSMutableSet setWithCapacity:availableChildren.size()];
  NSMutableDictionary<NSString *, NSMutableArray<NSString *> *> * availableCountries = [@{} mutableCopy];
  BOOL const isParentRoot = self.isParentRoot;
  auto const & s = GetFramework().GetStorage();
  for (auto const & countryId : availableChildren)
  {
    NSString * nsCountryId = @(countryId.c_str());
    string localName = s.GetNodeLocalName(countryId);
    NSString * index = isParentRoot ? [@(localName.c_str()) substringToIndex:1].capitalizedString : L(@"downloader_available_maps");
    [indexSet addObject:index];

    NSMutableArray<NSString *> * letterIds = availableCountries[index];
    letterIds = letterIds ?: [@[] mutableCopy];
    [letterIds addObject:nsCountryId];
    availableCountries[index] = letterIds;
  }
  self.indexes = [[indexSet allObjects] sortedArrayUsingComparator:compareStrings];
  [availableCountries enumerateKeysAndObjectsUsingBlock:^(NSString * key, NSMutableArray<NSString *> * obj, BOOL * stop)
  {
    [obj sortUsingComparator:compareLocalNames];
  }];
  self.availableCountries = availableCountries;
}

- (void)configDownloadedSection:(TCountriesVec const &)downloadedChildren
{
  NSMutableArray<NSString *> * downloadedCountries = [@[] mutableCopy];
  for (auto const & countryId : downloadedChildren)
    [downloadedCountries addObject:@(countryId.c_str())];
  self.downloadedCountries = [downloadedCountries sortedArrayUsingComparator:compareLocalNames];
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  if (self.downloadedCountries && self.downloadedCountries.count)
    return self.isParentRoot ? 2 : 1;
  if (self.indexes)
    return self.indexes.count;
  return 0;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if ([self isButtonCell:section])
    return 1;
  if (self.downloadedCountries && self.downloadedCountries.count)
    return self.downloadedCountries.count;
  NSString * index = self.indexes[section];
  return self.availableCountries[index].count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isButtonCell:indexPath.section])
  {
    Class cls = [MWMMapDownloaderButtonTableViewCell class];
    auto cell = static_cast<MWMMapDownloaderButtonTableViewCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    cell.delegate = self.delegate;
    return cell;
  }
  else
  {
    return [super tableView:tableView cellForRowAtIndexPath:indexPath];
  }
}

- (NSArray<NSString *> *)sectionIndexTitlesForTableView:(UITableView *)tableView
{
  return self.isParentRoot ? self.indexes : nil;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return index;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (self.downloadedCountries)
  {
    if ([self isButtonCell:section])
      return @"";
    NodeAttrs nodeAttrs;
    GetFramework().GetStorage().GetNodeAttrs(m_parentId, nodeAttrs);
    if (nodeAttrs.m_localMwmSize == 0)
      return [NSString stringWithFormat:@"%@", L(@"downloader_downloaded_subtitle")];
    else
      return [NSString stringWithFormat:@"%@ (%@)", L(@"downloader_downloaded_subtitle"), formattedSize(nodeAttrs.m_localMwmSize)];
  }
  return self.indexes[section];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return nil;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isButtonCell:indexPath.section])
    return NO;
  NodeAttrs nodeAttrs;
  GetFramework().GetStorage().GetNodeAttrs([self countryIdForIndexPath:indexPath].UTF8String, nodeAttrs);
  NodeStatus const status = nodeAttrs.m_status;
  return (status == NodeStatus::OnDisk || status == NodeStatus::OnDiskOutOfDate || status == NodeStatus::Partly);
}

#pragma mark - MWMMapDownloaderDataSource

- (NSString *)parentCountryId
{
  return @(m_parentId.c_str());
}

- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const section = indexPath.section;
  NSInteger const row = indexPath.row;
  if (row < self.downloadedCountries.count)
    return self.downloadedCountries[row];
  NSString * index = self.indexes[section];
  NSArray<NSString *> * availableCountries = self.availableCountries[index];
  NSString * nsCountryId = availableCountries[indexPath.row];
  return nsCountryId;
}

- (Class)cellClassForIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().GetStorage();
  TCountriesVec children;
  s.GetChildren([self countryIdForIndexPath:indexPath].UTF8String, children);
  BOOL const haveChildren = !children.empty();
  if (haveChildren)
    return [MWMMapDownloaderLargeCountryTableViewCell class];
  if (self.isParentRoot)
    return [MWMMapDownloaderTableViewCell class];
  return [MWMMapDownloaderPlaceTableViewCell class];
}

#pragma mark - Helpers

- (BOOL)isButtonCell:(NSInteger)section
{
  return self.downloadedCountries && self.isParentRoot && section != 0;
}

@end
