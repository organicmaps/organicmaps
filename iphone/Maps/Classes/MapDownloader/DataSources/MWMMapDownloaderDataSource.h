#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCell.h"
#import "MWMMapDownloaderTypes.h"

#include "storage/index.hpp"

@interface MWMMapDownloaderDataSource : NSObject <UITableViewDataSource>

@property (nonatomic, readonly) BOOL isParentRoot;
@property (nonatomic, readonly) TMWMMapDownloaderMode mode;

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol>)delegate mode:(TMWMMapDownloaderMode)mode;
- (NSString *)parentCountryId;
- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath;
- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

- (NSString *)searchMatchedResultForCountryId:(NSString *)countryId;

@end
