#import "MWMMapDownloaderDefaultDataSource.h"

#include "Framework.h"

extern NSString * const kCountryCellIdentifier;
extern NSString * const kSubplaceCellIdentifier;
extern NSString * const kPlaceCellIdentifier;
extern NSString * const kLargeCountryCellIdentifier;

using namespace storage;

@interface MWMMapDownloaderDefaultDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * indexes;
@property (copy, nonatomic) NSDictionary<NSString *, NSArray<NSString *> *> * countryIds;

@end

@implementation MWMMapDownloaderDefaultDataSource
{
  TCountryId m_parentId;
}

- (instancetype)initForRootCountryId:(storage::TCountryId)countryId delegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super initWithDelegate:delegate];
  if (self)
    [self configData:countryId];
  return self;
}

- (void)configData:(TCountryId)countryId
{
  m_parentId = countryId;
  auto const & s = GetFramework().Storage();
  TCountriesVec children;
  s.GetChildren(m_parentId, children);
  NSMutableSet<NSString *> * indexSet = [NSMutableSet setWithCapacity:children.size()];
  NSMutableDictionary<NSString *, NSArray<NSString *> *> * countryIds = [@{} mutableCopy];
  BOOL const isParentRoot = self.isParentRoot;
  for (auto const & countryId : children)
  {
    NSString * nsCountryId = @(countryId.c_str());
    NSString * index = isParentRoot ? [L(nsCountryId) substringToIndex:1].capitalizedString : @"all_values";
    [indexSet addObject:index];

    NSMutableArray<NSString *> * letterIds = [countryIds[index] mutableCopy];
    letterIds = letterIds ? letterIds : [@[] mutableCopy];
    [letterIds addObject:nsCountryId];
    countryIds[index] = [letterIds copy];
  }
  NSLocale * currentLocale = [NSLocale currentLocale];
  auto sort = ^NSComparisonResult(NSString * s1, NSString * s2)
  {
    NSString * l1 = L(s1);
    return [l1 compare:L(s2) options:NSCaseInsensitiveSearch range:{0, l1.length} locale:currentLocale];
  };
  self.indexes = [[indexSet allObjects] sortedArrayUsingComparator:sort];
  [countryIds enumerateKeysAndObjectsUsingBlock:^(NSString * key, NSArray<NSString *> * obj, BOOL * stop)
  {
    countryIds[key] = [obj sortedArrayUsingComparator:sort];
  }];
  self.countryIds = countryIds;
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.indexes.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  NSString * index = self.indexes[section];
  return self.countryIds[index].count;
}

- (NSArray<NSString *> * _Nullable)sectionIndexTitlesForTableView:(UITableView *)tableView
{
  return self.isParentRoot ? self.indexes : nil;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index
{
  return index;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return self.isParentRoot ? self.indexes[section] : nil;
}

#pragma mark - MWMMapDownloaderDataSourceProtocol

- (BOOL)isParentRoot
{
  return (m_parentId == GetFramework().Storage().GetRootId());
}

- (TCountryId)parentCountryId
{
  return m_parentId;
}

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSString * index = self.indexes[indexPath.section];
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

@end
