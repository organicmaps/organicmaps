#import "MWMMapDownloaderProtocol.h"
#import "MWMMapDownloaderTableViewCell.h"
#import "MWMViewController.h"

@interface MWMBaseMapDownloaderViewController : MWMViewController <UITableViewDelegate, MWMMapDownloaderProtocol>

@property (weak, nonatomic) IBOutlet UILabel * allMapsLabel;

@property (nonatomic) BOOL showAllMapsView;

@property (nonatomic) NSString * parentCountryId;

- (void)configTable;
- (void)configAllMapsView;

@end
