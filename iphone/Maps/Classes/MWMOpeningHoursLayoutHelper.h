@class MWMPlacePageData;

@interface MWMOpeningHoursLayoutHelper : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView;
- (void)configWithData:(MWMPlacePageData *)data;
- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath;

@end
