#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCell.h"

#include "storage/index.hpp"

@protocol MWMMapDownloaderDataSourceProtocol <UITableViewDataSource>

- (BOOL)isParentRoot;
- (storage::TCountryId)parentCountryId;
- (storage::TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath;
- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

@optional

- (NSString *)searchMatchedResultForCountryId:(storage::TCountryId)countryId;

@end

@interface MWMMapDownloaderDataSource : NSObject <MWMMapDownloaderDataSourceProtocol>

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol>)delegate;

@end
