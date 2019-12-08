#import "MWMFacilitiesController.h"
#import "SwiftBridge.h"

@implementation MWMFacilitiesController

- (void)viewDidLoad {
  [super viewDidLoad];

  self.tableView.estimatedRowHeight = 44;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  [self.tableView registerWithCellClass:[MWMPPFacilityCell class]];

  self.title = self.name;
}

#pragma mark - TableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  return self.facilities.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  Class cls = [MWMPPFacilityCell class];
  MWMPPFacilityCell *c = (MWMPPFacilityCell *)[tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  [c configWith:self.facilities[indexPath.row].name];
  return c;
}

@end
