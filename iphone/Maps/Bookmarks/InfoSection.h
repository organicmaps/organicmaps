#import "TableSectionDataSource.h"

@protocol InfoSectionDelegate <NSObject>

- (UITableViewCell *)infoCellForTableView:(UITableView *)tableView;

@end

@interface InfoSection : NSObject <TableSectionDataSource>

- (instancetype)initWithDelegate:(id<InfoSectionDelegate>)delegate;

@end

