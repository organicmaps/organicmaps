#import "MWMMapDownloaderButtonTableViewCell.h"
#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTypes.h"
#import "MWMViewController.h"

@interface MWMBaseMapDownloaderViewController : MWMViewController <UITableViewDelegate, MWMMapDownloaderProtocol, MWMMapDownloaderButtonTableViewCellProtocol>

@property (weak, nonatomic) IBOutlet UILabel * allMapsLabel;

@property (nonatomic) BOOL showAllMapsView;

- (void)configTable;
- (void)configAllMapsView;

- (void)setParentCountryId:(NSString *)parentId mode:(TMWMMapDownloaderMode)mode;

@end
