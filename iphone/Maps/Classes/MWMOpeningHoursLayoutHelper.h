@class MWMPlacePageData;

@interface MWMOpeningHoursLayoutHelper : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView;
- (void)registerCells;
- (void)configWithData:(MWMPlacePageData *)data;
- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath;

@end
