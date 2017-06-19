#import "MWUGCCommentsController.h"
#import "MWMUGCCommentCell.h"
#import "SwiftBridge.h"

#include "ugc/types.hpp"

@interface MWUGCCommentsController ()
{
  std::vector<ugc::Review> m_dataSource;
}

@end

@implementation MWUGCCommentsController

+ (instancetype)instanceWithTitle:(NSString *)title comments:(std::vector<ugc::Review> const &)comments
{
  auto inst = [[MWUGCCommentsController alloc] initWithNibName:self.className bundle:nil];
  inst.title = title;
  inst->m_dataSource = comments;
  return inst;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  auto tv = self.tableView;
  tv.estimatedRowHeight = 44;
  tv.rowHeight = UITableViewAutomaticDimension;
  [tv registerWithCellClass:[MWMUGCCommentCell class]];
}

#pragma mark - UITableView's methods

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_dataSource.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [MWMUGCCommentCell class];
  auto c = static_cast<MWMUGCCommentCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  [c configWithReview:m_dataSource[indexPath.row]];
  return c;
}

@end
