#import "MWMFrameworkObservers.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCellProtocol.h"
#import "MWMTableViewCell.h"

#include "storage/storage.hpp"

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell <MWMMapDownloaderTableViewCellProtocol, MWMFrameworkStorageObserver>

@property (nonatomic) BOOL isHeightCell;
@property (weak, nonatomic) id<MWMMapDownloaderProtocol> delegate;

- (void)config:(storage::NodeAttrs const &)nodeAttrs;
- (void)setCountryId:(NSString *)countryId searchQuery:(NSString *)query;

@end
