NS_ASSUME_NONNULL_BEGIN

@class CLLocation;

@protocol TableSectionDataSource <NSObject>

- (NSInteger)numberOfRows;
- (nullable NSString *)title;
- (BOOL)canEdit;

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row;

- (BOOL)didSelectRow:(NSInteger)row;
- (void)deleteRow:(NSInteger)row;

@optional

- (void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(CLLocation *)location;

@end

NS_ASSUME_NONNULL_END
