#include "platform/location.hpp"

NS_ASSUME_NONNULL_BEGIN

@protocol TableSectionDataSource <NSObject>

- (NSInteger)numberOfRows;
- (nullable NSString *)title;
- (BOOL)canEdit;

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row;

- (BOOL)didSelectRow:(NSInteger)row;
- (void)deleteRow:(NSInteger)row;

@optional

- (void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(location::GpsInfo const &)gpsInfo;

@end

NS_ASSUME_NONNULL_END
