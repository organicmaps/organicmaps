#import "Common.h"
#import "MWMMapDownloaderDataSource.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"

#include "Framework.h"

using namespace storage;

@interface MWMMapDownloaderDataSource ()

@property (weak, nonatomic) id<MWMMapDownloaderProtocol> delegate;

@end

@implementation MWMMapDownloaderDataSource

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol>)delegate
{
  self = [super init];
  if (self)
    _delegate = delegate;
  return self;
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath
{
  TCountryId const countryId = [self countryIdForIndexPath:indexPath];

  if ([cell isKindOfClass:[MWMMapDownloaderPlaceTableViewCell class]])
    static_cast<MWMMapDownloaderPlaceTableViewCell *>(cell).needDisplayArea = self.isParentRoot;

  if ([cell isKindOfClass:[MWMMapDownloaderSubplaceTableViewCell class]])
    [static_cast<MWMMapDownloaderSubplaceTableViewCell *>(cell)
        setSubplaceText:[self searchMatchedResultForCountryId:countryId]];

  [cell setCountryId:countryId];
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
  [cell registerObserver];
  [self fillCell:cell atIndexPath:indexPath];
  return cell;
}

#pragma mark - MWMMapDownloaderDataSource

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

- (NSString *)searchMatchedResultForCountryId:(storage::TCountryId)countryId
{
  return nil;
}

@end
