#import "MWMMapDownloaderButtonTableViewCell.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCell.h"
#import "MWMMapDownloaderTypes.h"

#include "storage/index.hpp"

@interface MWMMapDownloaderDataSource : NSObject <UITableViewDataSource>

@property (nonatomic, readonly) BOOL isParentRoot;
@property (nonatomic, readonly) TMWMMapDownloaderMode mode;
@property (weak, nonatomic, readonly) id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol> delegate;

- (instancetype)initWithDelegate:(id<MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>)delegate mode:(TMWMMapDownloaderMode)mode;
- (NSString *)parentCountryId;
- (NSString *)countryIdForIndexPath:(NSIndexPath *)indexPath;
- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;
- (BOOL)isButtonCell:(NSInteger)section;

- (NSString *)searchMatchedResultForCountryId:(NSString *)countryId;

@end
