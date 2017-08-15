#import "MWMFrameworkObservers.h"
#import "MWMMapDownloaderMode.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCellProtocol.h"
#import "MWMTableViewCell.h"

namespace storage
{
struct NodeAttrs;
}  // storage

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell <MWMMapDownloaderTableViewCellProtocol, MWMFrameworkStorageObserver>

@property(nonatomic) BOOL isHeightCell;
@property(weak, nonatomic) id<MWMMapDownloaderProtocol> delegate;
@property(nonatomic) MWMMapDownloaderMode mode;

- (void)config:(storage::NodeAttrs const &)nodeAttrs;
- (void)setCountryId:(NSString *)countryId searchQuery:(NSString *)query;

@end
