#import "MWMTableViewCell.h"

@protocol MWMMapDownloaderButtonTableViewCellProtocol <NSObject>

- (void)onAddMaps;

@end

@interface MWMMapDownloaderButtonTableViewCell : MWMTableViewCell

@property (weak, nonatomic) id<MWMMapDownloaderButtonTableViewCellProtocol> delegate;

@end
