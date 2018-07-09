#import "MWMMapDownloaderSearchDataSource.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"

#include "Framework.h"

using namespace storage;

@interface MWMMapDownloaderSearchDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * searchCountryIds;
@property (copy, nonatomic) NSDictionary<NSString *, NSString *> * searchMatchedResults;
@property (copy, nonatomic) NSString * searchQuery;

@end

@implementation MWMMapDownloaderSearchDataSource

- (instancetype)initWithSearchResults:(DownloaderSearchResults const &)results delegate:(id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate
{
  self = [super initWithDelegate:delegate mode:MWMMapDownloaderModeAvailable];
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
  return @(GetFramework().GetStorage().GetRootId().c_str());
}

- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < self.searchCountryIds.count)
    return self.searchCountryIds[indexPath.row];
  return @(kInvalidCountryId.c_str());
}

- (Class)cellClassForIndexPath:(NSIndexPath *)indexPath
{
  auto const & s = GetFramework().GetStorage();
  NSString * countryId = [self countryIdForIndexPath:indexPath];
  TCountriesVec children;
  s.GetChildren(countryId.UTF8String, children);
  BOOL const haveChildren = !children.empty();
  if (haveChildren)
    return [MWMMapDownloaderLargeCountryTableViewCell class];
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(countryId.UTF8String, nodeAttrs);
  NSString * nodeLocalName = @(nodeAttrs.m_nodeLocalName.c_str());
  NSString * matchedResult = [self searchMatchedResultForCountryId:countryId];
  if (![nodeLocalName isEqualToString:matchedResult])
    return [MWMMapDownloaderSubplaceTableViewCell class];
  if (nodeAttrs.m_parentInfo.size() == 1 && nodeAttrs.m_parentInfo[0].m_id == s.GetRootId())
    return [MWMMapDownloaderTableViewCell class];
  return [MWMMapDownloaderPlaceTableViewCell class];
}

- (NSString *)searchMatchedResultForCountryId:(NSString *)countryId
{
  return self.searchMatchedResults[countryId];
}

#pragma mark - Properties

- (BOOL)isEmpty { return self.searchCountryIds.count == 0; }
@end
