#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCell.h"

#include "storage/index.hpp"

@interface MWMMapDownloaderDataSource : NSObject <UITableViewDataSource>

@property (nonatomic, readonly) BOOL isParentRoot;

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol>)delegate;
- (storage::TCountryId)parentCountryId;
- (storage::TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath;
- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

- (NSString *)searchMatchedResultForCountryId:(storage::TCountryId)countryId;

@end
