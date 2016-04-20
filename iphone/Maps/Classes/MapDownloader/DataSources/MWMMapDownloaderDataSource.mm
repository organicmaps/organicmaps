#import "Common.h"
#import "MWMMapDownloaderDataSource.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "MWMMapDownloaderTypes.h"

#include "Framework.h"

using namespace storage;

@implementation MWMMapDownloaderDataSource

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate mode:(TMWMMapDownloaderMode)mode
{
  self = [super init];
  if (self)
  {
    _delegate = delegate;
    _mode = mode;
  }
  return self;
}

#pragma mark - Fill cells with data

- (void)fillCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath
{
  if (![cell isKindOfClass:[MWMMapDownloaderTableViewCell class]])
    return;

  NSString * countryId = [self countryIdForIndexPath:indexPath];
  if ([cell isKindOfClass:[MWMMapDownloaderPlaceTableViewCell class]])
  {
    MWMMapDownloaderPlaceTableViewCell * placeCell = static_cast<MWMMapDownloaderPlaceTableViewCell *>(cell);
    placeCell.needDisplayArea = self.isParentRoot;
  }

  if ([cell isKindOfClass:[MWMMapDownloaderSubplaceTableViewCell class]])
  {
    MWMMapDownloaderSubplaceTableViewCell * subplaceCell = static_cast<MWMMapDownloaderSubplaceTableViewCell *>(cell);
    [subplaceCell setSubplaceText:[self searchMatchedResultForCountryId:countryId]];
  }

  MWMMapDownloaderTableViewCell * tCell = static_cast<MWMMapDownloaderTableViewCell *>(cell);
  [tCell setCountryId:countryId searchQuery:[self searchQuery]];
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
  cell.delegate = self.delegate;
  cell.mode = self.mode;
  [self fillCell:cell atIndexPath:indexPath];
  return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  return NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (editingStyle == UITableViewCellEditingStyleDelete)
    [self.delegate deleteNode:[self countryIdForIndexPath:indexPath].UTF8String];
}

#pragma mark - MWMMapDownloaderDataSource

- (BOOL)isParentRoot
{
  return YES;
}

- (NSString *)parentCountryId
{
  return @(kInvalidCountryId.c_str());
}

- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  return @(kInvalidCountryId.c_str());
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  return nil;
}

- (NSString *)searchMatchedResultForCountryId:(NSString *)countryId
{
  return nil;
}

- (NSString *)searchQuery
{
  return nil;
}

#pragma mark - Helpers

- (BOOL)isButtonCell:(NSInteger)section
{
  return NO;
}

@end
