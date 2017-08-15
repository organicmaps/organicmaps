#import "MWMMapDownloaderButtonTableViewCell.h"
#import "MWMMapDownloaderMode.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMViewController.h"

@interface MWMBaseMapDownloaderViewController : MWMViewController <UITableViewDelegate, MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>

- (void)configTable;
- (void)configAllMapsView;

- (void)setParentCountryId:(NSString *)parentId mode:(MWMMapDownloaderMode)mode;

@end
