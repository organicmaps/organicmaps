#import "MWMMapDownloaderProtocol.h"
#import "MWMTableViewCell.h"

#include "storage/storage.hpp"

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell

@property (nonatomic, readonly) CGFloat estimatedHeight;
@property (weak, nonatomic) id<MWMMapDownloaderProtocol> delegate;

- (void)registerObserver;
- (void)config:(storage::NodeAttrs const &)nodeAttrs;
- (void)setCountryId:(storage::TCountryId const &)countryId;

@end
