#import "SelectSetVC.h"
#import "AddSetVC.h"
#import "SwiftBridge.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

@interface SelectSetVC () <AddSetVCDelegate>
{
  size_t m_categoryIndex;
}

@property (copy, nonatomic) NSString * category;
@property (weak, nonatomic) id<MWMSelectSetDelegate> delegate;

@end

@implementation SelectSetVC

- (instancetype)initWithCategory:(NSString *)category
                   categoryIndex:(size_t)categoryIndex
                        delegate:(id<MWMSelectSetDelegate>)delegate
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    _category = category;
    m_categoryIndex = categoryIndex;
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

  return GetFramework().GetBmCategoriesCount();
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
    BookmarkCategory * cat = GetFramework().GetBmCategory(indexPath.row);
    if (cat)
      cell.textLabel.text = @(cat->GetName().c_str());

    if (m_categoryIndex == indexPath.row)
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
      cell.accessoryType = UITableViewCellAccessoryNone;
  }
  return cell;
}

- (void)addSetVC:(AddSetVC *)vc didAddSetWithIndex:(int)setIndex
{
  [self moveBookmarkToSetWithIndex:setIndex];
  [self.tableView reloadData];
  [self.delegate didSelectCategory:self.category withCategoryIndex:setIndex];
}

- (void)moveBookmarkToSetWithIndex:(int)setIndex
{
  BookmarkCategory const * category =
      GetFramework().GetBookmarkManager().GetBmCategory(setIndex);
  self.category = @(category->GetName().c_str());
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
    [self moveBookmarkToSetWithIndex:static_cast<int>(indexPath.row)];
    [self.delegate didSelectCategory:self.category withCategoryIndex:indexPath.row];
    [self backTap];
  }
}

@end
