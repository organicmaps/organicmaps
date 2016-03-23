#import "MWMMapDownloaderSearchDataSource.h"

#include "Framework.h"

using namespace storage;

extern NSString * const kCountryCellIdentifier;
extern NSString * const kSubplaceCellIdentifier;
extern NSString * const kPlaceCellIdentifier;

@interface MWMMapDownloaderSearchDataSource ()

@property (copy, nonatomic) NSArray<NSString *> * searchCountryIds;
@property (copy, nonatomic) NSDictionary<NSString *, NSString *> * searchMatchedResults;

@end

@implementation MWMMapDownloaderSearchDataSource

- (instancetype)initWithSearchResults:(DownloaderSearchResults const &)results delegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super initWithDelegate:delegate];
  if (self)
  {
    NSMutableOrderedSet<NSString *> * nsSearchCountryIds = [NSMutableOrderedSet orderedSetWithCapacity:results.m_results.size()];
    NSMutableDictionary<NSString *, NSString *> * nsSearchResults = [@{} mutableCopy];
    for (auto const & result : results.m_results)
    {
      NSString * nsCountryId = @(result.m_countryId.c_str());
      [nsSearchCountryIds addObject:nsCountryId];
      nsSearchResults[nsCountryId] = @(result.m_matchedName.c_str());
    }
    self.searchCountryIds = [nsSearchCountryIds array];
    self.searchMatchedResults = nsSearchResults;
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

- (TCountryId)parentCountryId
{
  return GetFramework().Storage().GetRootId();
}

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  return self.searchCountryIds[indexPath.row].UTF8String;
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

- (BOOL)needFullReload
{
  return YES;
}

@end
