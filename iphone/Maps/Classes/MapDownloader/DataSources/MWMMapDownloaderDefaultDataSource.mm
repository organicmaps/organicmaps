#import "Common.h"
#import "MWMMapDownloaderDefaultDataSource.h"

#include "Framework.h"

extern NSString * const kCountryCellIdentifier;
extern NSString * const kSubplaceCellIdentifier;
extern NSString * const kPlaceCellIdentifier;
extern NSString * const kLargeCountryCellIdentifier;

namespace
{
auto compareStrings = ^NSComparisonResult(NSString * s1, NSString * s2)
{
  return [s1 compare:s2 options:NSCaseInsensitiveSearch range:{0, s1.length} locale:[NSLocale currentLocale]];
};
auto compareLocalNames = ^NSComparisonResult(NSString * s1, NSString * s2)
{
  auto const & s = GetFramework().Storage();
  string l1 = s.GetNodeLocalName(s1.UTF8String);
  string l2 = s.GetNodeLocalName(s2.UTF8String);
  return compareStrings(@(l1.c_str()), @(l2.c_str()));
};
} // namespace

using namespace storage;

@interface MWMMapDownloaderDefaultDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * indexes;
@property (copy, nonatomic) NSDictionary<NSString *, NSArray<NSString *> *> * countryIds;

@property (copy, nonatomic) NSArray<NSString *> * downloadedCoutryIds;
@property (nonatomic, readwrite) NSInteger downloadedCountrySection;
@property (nonatomic, readonly) NSInteger countrySectionsShift;

@property (nonatomic, readwrite) BOOL needFullReload;

@end

@implementation MWMMapDownloaderDefaultDataSource
{
  TCountryId m_parentId;
  std::vector<NSInteger> m_reloadSections;
}

@synthesize isParentRoot = _isParentRoot;

- (instancetype)initForRootCountryId:(storage::TCountryId)countryId delegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super initWithDelegate:delegate];
  if (self)
  {
    m_parentId = countryId;
    _isParentRoot = (m_parentId == GetFramework().Storage().GetRootId());
    [self load];
  }
  return self;
}

- (void)load
{
  auto const & s = GetFramework().Storage();
  TCountriesVec downloadedChildren;
  TCountriesVec availableChildren;
  s.GetChildrenInGroups(m_parentId, downloadedChildren, availableChildren);
  [self configAvailableSections:availableChildren];
  [self configDownloadedSection:downloadedChildren];
}

- (void)reload
{
  // Get old data for comparison.
  NSArray<NSString *> * downloadedCoutryIds = [self.downloadedCoutryIds copy];
  NSDictionary<NSString *, NSArray<NSString *> *> * countryIds = [self.countryIds copy];
  BOOL const hadDownloadedCountries = self.haveDownloadedCountries;

  // Load updated data.
  [self load];

  // Compare new data vs old data to understand what kind of reload is required and what sections need reload.
  self.needFullReload = (hadDownloadedCountries != self.haveDownloadedCountries || countryIds.count == 0);
  if (self.needFullReload)
    return;
  if (hadDownloadedCountries || ![downloadedCoutryIds isEqualToArray:self.downloadedCoutryIds])
    m_reloadSections.push_back(self.downloadedCountrySection);
  [countryIds enumerateKeysAndObjectsUsingBlock:^(NSString * key, NSArray<NSString *> * obj, BOOL * stop)
  {
    NSArray<NSString *> * sectionCountries = self.countryIds[key];
    if (!sectionCountries)
    {
      self.needFullReload = YES;
      *stop = YES;
    }
    if (![obj isEqualToArray:sectionCountries])
      self->m_reloadSections.push_back([self.indexes indexOfObject:key] + self.countrySectionsShift);
  }];
}

- (std::vector<NSInteger>)getReloadSections
{
  return m_reloadSections;
}

- (void)configAvailableSections:(TCountriesVec const &)availableChildren
{
  NSMutableSet<NSString *> * indexSet = [NSMutableSet setWithCapacity:availableChildren.size()];
  NSMutableDictionary<NSString *, NSArray<NSString *> *> * countryIds = [@{} mutableCopy];
  BOOL const isParentRoot = self.isParentRoot;
  auto const & s = GetFramework().Storage();
  for (auto const & countryId : availableChildren)
  {
    NSString * nsCountryId = @(countryId.c_str());
    string localName = s.GetNodeLocalName(countryId);
    NSString * index = isParentRoot ? [@(localName.c_str()) substringToIndex:1].capitalizedString : L(@"downloader_available_maps");
    [indexSet addObject:index];

    NSMutableArray<NSString *> * letterIds = [countryIds[index] mutableCopy];
    letterIds = letterIds ? letterIds : [@[] mutableCopy];
    [letterIds addObject:nsCountryId];
    countryIds[index] = [letterIds copy];
  }
  self.indexes = [[indexSet allObjects] sortedArrayUsingComparator:compareStrings];
  [countryIds enumerateKeysAndObjectsUsingBlock:^(NSString * key, NSArray<NSString *> * obj, BOOL * stop)
  {
    countryIds[key] = [obj sortedArrayUsingComparator:compareLocalNames];
  }];
  self.countryIds = countryIds;
}

- (void)configDownloadedSection:(TCountriesVec const &)downloadedChildren
{
  self.downloadedCoutryIds = nil;
  self.downloadedCountrySection = NSNotFound;
  NSMutableArray<NSString *> * nsDownloadedCoutryIds = [@[] mutableCopy];
  for (auto const & countryId : downloadedChildren)
    [nsDownloadedCoutryIds addObject:@(countryId.c_str())];
  [nsDownloadedCoutryIds sortUsingComparator:compareLocalNames];
  if (nsDownloadedCoutryIds.count != 0)
  {
    self.downloadedCoutryIds = nsDownloadedCoutryIds;
    self.downloadedCountrySection = 0;
  }
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.indexes.count + self.countrySectionsShift;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == self.downloadedCountrySection)
    return self.downloadedCoutryIds.count;
  NSString * index = self.indexes[section - self.countrySectionsShift];
  return self.countryIds[index].count;
}

- (NSArray<NSString *> *)sectionIndexTitlesForTableView:(UITableView *)tableView
{
  return self.isParentRoot ? self.indexes : nil;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return index + self.countrySectionsShift;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == self.downloadedCountrySection)
  {
    NodeAttrs nodeAttrs;
    GetFramework().Storage().GetNodeAttrs(m_parentId, nodeAttrs);
    if (nodeAttrs.m_localMwmSize == 0)
      return [NSString stringWithFormat:@"%@", L(@"downloader_downloaded")];
    else
      return [NSString stringWithFormat:@"%@ (%@)", L(@"downloader_downloaded"), formattedSize(nodeAttrs.m_localMwmSize)];
  }
  return self.indexes[section - self.countrySectionsShift];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return nil;
}

#pragma mark - MWMMapDownloaderDataSource

- (TCountryId)parentCountryId
{
  return m_parentId;
}

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const section = indexPath.section;
  NSInteger const row = indexPath.row;
  if (section == self.downloadedCountrySection)
    return self.downloadedCoutryIds[row].UTF8String;
  NSString * index = self.indexes[section - self.countrySectionsShift];
  NSArray<NSString *> * countryIds = self.countryIds[index];
  NSString * nsCountryId = countryIds[indexPath.row];
  return nsCountryId.UTF8String;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  TCountriesVec children;
  s.GetChildren([self countryIdForIndexPath:indexPath], children);
  BOOL const haveChildren = !children.empty();
  if (haveChildren)
    return kLargeCountryCellIdentifier;
  return self.isParentRoot ? kCountryCellIdentifier : kPlaceCellIdentifier;
}

#pragma mark - Properties

- (NSInteger)countrySectionsShift
{
  return (self.haveDownloadedCountries ? self.downloadedCountrySection + 1 : 0);
}

- (BOOL)haveDownloadedCountries
{
  return (self.downloadedCountrySection != NSNotFound);
}

- (void)setNeedFullReload:(BOOL)needFullReload
{
  _needFullReload = needFullReload;
  if (needFullReload)
    m_reloadSections.clear();
}

@end
