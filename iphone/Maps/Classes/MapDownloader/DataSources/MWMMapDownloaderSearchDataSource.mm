#import "MWMMapDownloaderSearchDataSource.h"

#include "Framework.h"

using namespace storage;

extern NSString * const kCountryCellIdentifier;
extern NSString * const kSubplaceCellIdentifier;
extern NSString * const kPlaceCellIdentifier;

@interface MWMMapDownloaderSearchDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * searchCoutryIds;
@property (copy, nonatomic) NSDictionary<NSString *, NSString *> * searchMatchedResults;

@end

@implementation MWMMapDownloaderSearchDataSource

- (instancetype)initWithSearchResults:(search::Results const &)results delegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super initWithDelegate:delegate];
  if (self)
    [self configSearchResults:results];
  return self;
}

- (void)configSearchResults:(search::Results const &)results
{
  auto const & countryInfoGetter = GetFramework().CountryInfoGetter();
  NSMutableOrderedSet<NSString *> * nsSearchCoutryIds = [NSMutableOrderedSet orderedSetWithCapacity:results.GetCount()];
  NSMutableDictionary<NSString *, NSString *> * nsSearchResults = [@{} mutableCopy];
  for (auto it = results.Begin(); it != results.End(); ++it)
  {
    if (!it->HasPoint())
      continue;
    auto const & mercator = it->GetFeatureCenter();
    NSString * countryId = @(countryInfoGetter.GetRegionCountryId(mercator).c_str());
    [nsSearchCoutryIds addObject:countryId];
    nsSearchResults[countryId] = @(it->GetString().c_str());
  }
  NSAssert(nsSearchCoutryIds.count != 0, @"Search results can not be empty.");
  self.searchCoutryIds = [nsSearchCoutryIds array];
  self.searchMatchedResults = nsSearchResults;
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return self.searchCoutryIds.count;
}

#pragma mark - MWMMapDownloaderDataSourceProtocol

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  return self.searchCoutryIds[indexPath.row].UTF8String;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  TCountryId const countryId = [self countryIdForIndexPath:indexPath];
  s.GetNodeAttrs(countryId, nodeAttrs);
  NSString * nodeLocalName = @(nodeAttrs.m_nodeLocalName.c_str());
  NSString * matchedResult = [self searchMatchedResultForCountryId:countryId];
  if (![nodeLocalName isEqualToString:matchedResult])
    return kSubplaceCellIdentifier;
  if (nodeAttrs.m_parentInfo.size() == 1 && nodeAttrs.m_parentInfo[0].m_id == s.GetRootId())
    return kCountryCellIdentifier;
  return kPlaceCellIdentifier;
}

- (NSString *)searchMatchedResultForCountryId:(storage::TCountryId)countryId
{
  return self.searchMatchedResults[@(countryId.c_str())];
}

@end
