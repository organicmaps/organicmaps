#import "MWMMapDownloaderSearchDataSource.h"

#include "Framework.h"

using namespace storage;

extern NSString * const kCountryCellIdentifier;
extern NSString * const kSubplaceCellIdentifier;
extern NSString * const kPlaceCellIdentifier;
extern NSString * const kLargeCountryCellIdentifier;

@interface MWMMapDownloaderSearchDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * searchCountryIds;
@property (copy, nonatomic) NSDictionary<NSString *, NSString *> * searchMatchedResults;
@property (copy, nonatomic) NSString * searchQuery;

@end

@implementation MWMMapDownloaderSearchDataSource

- (instancetype)initWithSearchResults:(DownloaderSearchResults const &)results delegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super initWithDelegate:delegate];
  if (self)
  {
    NSMutableOrderedSet<NSString *> * nsSearchCountryIds =
        [NSMutableOrderedSet orderedSetWithCapacity:results.m_results.size()];
    NSMutableDictionary<NSString *, NSString *> * nsSearchResults = [@{} mutableCopy];
    for (auto const & result : results.m_results)
    {
      NSString * nsCountryId = @(result.m_countryId.c_str());
      [nsSearchCountryIds addObject:nsCountryId];
      nsSearchResults[nsCountryId] = @(result.m_matchedName.c_str());
    }
    _searchCountryIds = [nsSearchCountryIds array];
    _searchMatchedResults = nsSearchResults;
    _searchQuery = @(results.m_query.c_str());
  }
  return self;
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return self.searchCountryIds.count;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return L(@"downloader_search_results");
}

#pragma mark - MWMMapDownloaderDataSource

- (NSString *)parentCountryId
{
  return @(GetFramework().Storage().GetRootId().c_str());
}

- (BOOL)isParentRoot
{
  return NO;
}

- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  return self.searchCountryIds[indexPath.row];
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().Storage();
  NSString * countryId = [self countryIdForIndexPath:indexPath];
  TCountriesVec children;
  s.GetChildren(countryId.UTF8String, children);
  BOOL const haveChildren = !children.empty();
  if (haveChildren)
    return kLargeCountryCellIdentifier;
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(countryId.UTF8String, nodeAttrs);
  NSString * nodeLocalName = @(nodeAttrs.m_nodeLocalName.c_str());
  NSString * matchedResult = [self searchMatchedResultForCountryId:countryId];
  if (![nodeLocalName isEqualToString:matchedResult])
    return kSubplaceCellIdentifier;
  if (nodeAttrs.m_parentInfo.size() == 1 && nodeAttrs.m_parentInfo[0].m_id == s.GetRootId())
    return kCountryCellIdentifier;
  return kPlaceCellIdentifier;
}

- (NSString *)searchMatchedResultForCountryId:(NSString *)countryId
{
  return self.searchMatchedResults[countryId];
}

- (BOOL)needFullReload
{
  return YES;
}

@end
