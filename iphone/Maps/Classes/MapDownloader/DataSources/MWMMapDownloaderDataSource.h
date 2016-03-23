#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCell.h"

#include "storage/index.hpp"

@interface MWMMapDownloaderDataSource : NSObject <UITableViewDataSource>

@property (nonatomic, readonly) BOOL isParentRoot;
@property (nonatomic, readonly) BOOL needFullReload;
@property (nonatomic, readonly) NSMutableIndexSet * reloadSections;

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol>)delegate;
- (NSString *)parentCountryId;
- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath;
- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

- (NSString *)searchMatchedResultForCountryId:(NSString *)countryId;

@end
