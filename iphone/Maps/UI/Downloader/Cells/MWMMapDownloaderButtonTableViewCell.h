#import "MWMTableViewCell.h"

@protocol MWMMapDownloaderButtonTableViewCellProtocol <NSObject>

- (void)openAvailableMaps;

@end

@interface MWMMapDownloaderButtonTableViewCell : MWMTableViewCell

@property (weak, nonatomic) id<MWMMapDownloaderButtonTableViewCellProtocol> delegate;

@end
