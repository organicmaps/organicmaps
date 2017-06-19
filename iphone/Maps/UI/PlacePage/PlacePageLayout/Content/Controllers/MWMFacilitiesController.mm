#import "MWMFacilitiesController.h"
#import "SwiftBridge.h"

#include "partners_api/booking_api.hpp"

@implementation MWMFacilitiesController
{
  std::vector<booking::HotelFacility> m_dataSource;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.tableView.estimatedRowHeight = 44;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  [self.tableView registerWithCellClass:[MWMPPFacilityCell class]];
}

- (void)setHotelName:(NSString *)name { self.title = name; }

- (void)setFacilities:(std::vector<booking::HotelFacility> const &)facilities { m_dataSource = facilities; }

#pragma mark - TableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_dataSource.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [MWMPPFacilityCell class];
  auto c = static_cast<MWMPPFacilityCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  [c configWith:@(m_dataSource[indexPath.row].m_name.c_str())];
  return c;
}

@end
