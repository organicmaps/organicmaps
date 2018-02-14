#import "SelectSetVC.h"
#import "AddSetVC.h"
#import "SwiftBridge.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

@interface SelectSetVC () <AddSetVCDelegate>
{
  df::MarkGroupID m_categoryId;
}

@property (copy, nonatomic) NSString * category;
@property (weak, nonatomic) id<MWMSelectSetDelegate> delegate;

@end

@implementation SelectSetVC

- (instancetype)initWithCategory:(NSString *)category
                      categoryId:(df::MarkGroupID)categoryId
                        delegate:(id<MWMSelectSetDelegate>)delegate
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    _category = category;
    m_categoryId = categoryId;
    _delegate = delegate;
  }
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  NSAssert(self.category, @"Category can't be nil!");
  NSAssert(self.delegate, @"Delegate can't be nil!");
  self.title = L(@"bookmark_sets");
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  // "Add new set" button
  if (section == 0)
    return 1;

  return GetFramework().GetBookmarkManager().GetBmGroupsIdList().size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [UITableViewCell class];
  auto cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  if (indexPath.section == 0)
  {
    cell.textLabel.text = L(@"add_new_set");
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  }
  else
  {
    auto & bmManager = GetFramework().GetBookmarkManager();
    auto categoryId = bmManager.GetBmGroupsIdList()[indexPath.row];
    if (bmManager.HasBmCategory(categoryId))
      cell.textLabel.text = @(bmManager.GetCategoryName(categoryId).c_str());

    if (m_categoryId == categoryId)
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
      cell.accessoryType = UITableViewCellAccessoryNone;
  }
  return cell;
}

- (void)addSetVC:(AddSetVC *)vc didAddSetWithCategoryId:(df::MarkGroupID)categoryId
{
  [self moveBookmarkToSetWithCategoryId:categoryId];
  [self.tableView reloadData];
  [self.delegate didSelectCategory:self.category withCategoryId:categoryId];
}

- (void)moveBookmarkToSetWithCategoryId:(df::MarkGroupID)categoryId
{
  self.category = @(GetFramework().GetBookmarkManager().GetCategoryName(categoryId).c_str());
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (indexPath.section == 0)
  {
    AddSetVC * asVC = [[AddSetVC alloc] init];
    asVC.delegate = self;
    [self.navigationController pushViewController:asVC animated:YES];
  }
  else
  {
    auto categoryId = GetFramework().GetBookmarkManager().GetBmGroupsIdList()[indexPath.row];
    [self moveBookmarkToSetWithCategoryId:categoryId];
    [self.delegate didSelectCategory:self.category withCategoryId:categoryId];
    [self backTap];
  }
}

@end
