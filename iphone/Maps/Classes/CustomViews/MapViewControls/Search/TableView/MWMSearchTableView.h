#import "MWMSearchNoResults.h"

@interface MWMSearchTableView : UIView

@property(weak, nonatomic) IBOutlet UITableView * tableView;

- (void)addNoResultsView:(MWMSearchNoResults *)view;
- (void)removeNoResultsView;

@end
