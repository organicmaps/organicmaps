#import "MWMMapDownloaderTableViewCellProtocol.h"
#import "MWMTableViewCell.h"

@protocol MWMMapDownloaderButtonTableViewCellProtocol <NSObject>

- (void)openAvailableMaps;

@end

@interface MWMMapDownloaderButtonTableViewCell : MWMTableViewCell <MWMMapDownloaderTableViewCellProtocol>

@property (weak, nonatomic) id<MWMMapDownloaderButtonTableViewCellProtocol> delegate;

@end
