#import "MWMController.h"

NS_ASSUME_NONNULL_BEGIN

@interface MWMTableViewController : UITableViewController <MWMController>

@end

@interface UITableView (MWMTableViewController)

- (UITableViewCell *)dequeueDefaultCellForIndexPath:(NSIndexPath *)indexPath;

@end

NS_ASSUME_NONNULL_END
