#import "MWMMapDownloaderTableViewCell.h"
#import "MWMViewController.h"

#include "storage/index.hpp"

@interface MWMBaseMapDownloaderViewController : MWMViewController <UITableViewDelegate, UITableViewDataSource>

@property (weak, nonatomic) IBOutlet UILabel * allMapsLabel;

@property (nonatomic) NSMutableDictionary * offscreenCells;

@property (nonatomic) BOOL showAllMapsView;

@property (nonatomic) storage::TCountryId parentCountryId;

- (void)configTable;
- (void)configAllMapsView;

@end
