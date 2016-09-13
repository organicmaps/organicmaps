#import "MWMSearchNoResults.h"

@interface MWMSearchTabbedCollectionViewCell : UICollectionViewCell

@property(weak, nonatomic) IBOutlet UITableView * tableView;

- (void)addNoResultsView:(MWMSearchNoResults *)view;
- (void)removeNoResultsView;

@end
