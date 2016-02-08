#import "MWMMapDownloaderTableViewCell.h"
#import "MWMViewController.h"

#include "storage/index.hpp"

@interface MWMBaseMapDownloaderViewController : MWMViewController <UITableViewDelegate, UITableViewDataSource>

@property (weak, nonatomic) IBOutlet UILabel * allMapsLabel;

@property (nonatomic) NSMutableDictionary * offscreenCells;

@property (nonatomic) BOOL showAllMapsView;

- (void)configTable;
- (void)configAllMapsView;

- (storage::TCountryId)GetRootCountryId;
- (void)SetRootCountryId:(storage::TCountryId)rootId;

@end
