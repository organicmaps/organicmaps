#import "MWMMapDownloaderButtonTableViewCell.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTypes.h"
#import "MWMViewController.h"

@interface MWMBaseMapDownloaderViewController : MWMViewController <UITableViewDelegate, MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>

- (void)configTable;
- (void)configAllMapsView;

- (void)setParentCountryId:(NSString *)parentId mode:(mwm::DownloaderMode)mode;

@end
