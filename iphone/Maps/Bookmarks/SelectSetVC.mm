
#import "SelectSetVC.h"
#import "Framework.h"
#import "AddSetVC.h"
#import "UIKitCategories.h"

@interface SelectSetVC () <AddSetVCDelegate>

@end

@implementation SelectSetVC
{
  BookmarkAndCategory m_bookmarkAndCategory;
}

- (id)initWithBookmarkAndCategory:(BookmarkAndCategory const &)bookmarkAndCategory
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_bookmarkAndCategory = bookmarkAndCategory;
    self.title = L(@"bookmark_sets");
  }
  return self;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
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
  static NSString * kSetCellId = @"AddSetCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:kSetCellId];
  if (cell == nil)
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:kSetCellId];
  // Customize cell
  if (indexPath.section == 0)
  {
    cell.textLabel.text = L(@"add_new_set");
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  }
  else
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(indexPath.row);
    if (cat)
      cell.textLabel.text = [NSString stringWithUTF8String:cat->GetName().c_str()];

    if (m_bookmarkAndCategory.first == indexPath.row)
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
  [self.delegate selectSetVC:self didUpdateBookmarkAndCategory:m_bookmarkAndCategory];
}

- (void)moveBookmarkToSetWithIndex:(int)setIndex
{
  m_bookmarkAndCategory.second = static_cast<int>(GetFramework().MoveBookmark(m_bookmarkAndCategory.second, m_bookmarkAndCategory.first, setIndex));
  m_bookmarkAndCategory.first = setIndex;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (indexPath.section == 0)
  {
    AddSetVC * asVC = [[AddSetVC alloc] init];
    asVC.delegate = self;
    if (IPAD)
      [asVC setContentSizeForViewInPopover:[self contentSizeForViewInPopover]];
    [self.navigationController pushViewController:asVC animated:YES];
  }
  else
  {
    [self moveBookmarkToSetWithIndex:static_cast<int>(indexPath.row)];
    [self.delegate selectSetVC:self didUpdateBookmarkAndCategory:m_bookmarkAndCategory];
    [self.navigationController popViewControllerAnimated:YES];
  }
}
@end
