#import "MWMFrameworkObservers.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMTableViewCell.h"

#include "storage/storage.hpp"

@protocol MWMMapDownloaderTableViewCellProtocol <NSObject>

+ (CGFloat)estimatedHeight;

@end

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell <MWMMapDownloaderTableViewCellProtocol, MWMFrameworkStorageObserver>

@property (nonatomic) BOOL isHeightCell;
@property (weak, nonatomic) id<MWMMapDownloaderProtocol> delegate;

- (void)config:(storage::NodeAttrs const &)nodeAttrs;
- (void)setCountryId:(storage::TCountryId const &)countryId;

@end
