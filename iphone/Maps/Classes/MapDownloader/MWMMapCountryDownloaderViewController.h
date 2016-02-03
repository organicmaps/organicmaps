#import "MWMMapDownloaderTableViewCell.h"
#import "MWMViewController.h"

@interface MWMMapCountryDownloaderViewController : MWMViewController <UITableViewDelegate, UITableViewDataSource>

@property (weak, nonatomic) IBOutlet UILabel * allMapsLabel;

@property (nonatomic) NSMutableDictionary * offscreenCells;

@property (nonatomic) BOOL showAllMapsView;

- (void)configTable;
- (void)configAllMapsView;

@end
