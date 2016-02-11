#import "Common.h"
#import "MWMMapDownloaderDataSource.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"

#include "Framework.h"

using namespace storage;

@implementation MWMMapDownloaderDataSource

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath
{
  TCountryId const countryId = [self countryIdForIndexPath:indexPath];
  auto const & s = GetFramework().Storage();
  NodeAttrs nodeAttrs;
  s.GetNodeAttrs(countryId, nodeAttrs);
  [cell setTitleText:@(nodeAttrs.m_nodeLocalName.c_str())];
  [cell setDownloadSizeText:formattedSize(nodeAttrs.m_mwmSize)];

  if ([cell isKindOfClass:[MWMMapDownloaderLargeCountryTableViewCell class]])
  {
    MWMMapDownloaderLargeCountryTableViewCell * tCell = (MWMMapDownloaderLargeCountryTableViewCell *)cell;
    [tCell setMapCountText:@(nodeAttrs.m_mwmCounter).stringValue];
  }
  else if ([cell isKindOfClass:[MWMMapDownloaderPlaceTableViewCell class]])
  {
    MWMMapDownloaderPlaceTableViewCell * tCell = (MWMMapDownloaderPlaceTableViewCell *)cell;
    NSString * areaText = self.isParentRoot ? @(nodeAttrs.m_parentLocalName.c_str()) : @"";
    [tCell setAreaText:areaText];
    if ([cell isKindOfClass:[MWMMapDownloaderSubplaceTableViewCell class]])
    {
      BOOL const correctDataSource = [self respondsToSelector:@selector(searchMatchedResultForCountryId:)];
      NSAssert(correctDataSource, @"Invalid data source");
      if (!correctDataSource)
        return;
      MWMMapDownloaderSubplaceTableViewCell * tCell = (MWMMapDownloaderSubplaceTableViewCell *)cell;
      [tCell setSubplaceText:[self searchMatchedResultForCountryId:countryId]];
    }
  }
}

#pragma mark - UITableViewDataSource

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  MWMMapDownloaderTableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
  [self fillCell:cell atIndexPath:indexPath];
  return cell;
}

#pragma mark - MWMMapDownloaderDataSourceProtocol

- (BOOL)isParentRoot
{
  return YES;
}

- (TCountryId)parentCountryId
{
  return kInvalidCountryId;
}

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  return kInvalidCountryId;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  return nil;
}

@end
