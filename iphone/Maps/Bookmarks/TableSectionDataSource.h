#include "platform/location.hpp"

@protocol TableSectionDataSource <NSObject>

- (NSInteger)numberOfRows;
- (NSString *)title;
- (BOOL)canEdit;

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row;

- (BOOL)didSelectRow:(NSInteger)row;
- (BOOL)deleteRow:(NSInteger)row;

@optional

- (void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(location::GpsInfo const &)gpsInfo;

@end
