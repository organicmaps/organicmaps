#import "AddSetVC.h"
#import "SelectSetVC.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

@interface SelectSetVC () <AddSetVCDelegate>
{
  BookmarkAndCategory m_bac;
}

@property (copy, nonatomic) NSString * category;
@property (weak, nonatomic) id<MWMSelectSetDelegate> delegate;

@end

@implementation SelectSetVC

- (instancetype)initWithCategory:(NSString *)category
                             bac:(BookmarkAndCategory const &)bac
                        delegate:(id<MWMSelectSetDelegate>)delegate
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    _category = category;
    m_bac = bac;
    _delegate = delegate;
  }
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  NSAssert(self.category, @"Category can't be nil!");
  NSAssert(self.delegate, @"Delegate can't be nil!");
  NSAssert(m_bac.IsValid(), @"Invalid BookmarkAndCategory's instance!");
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
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
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

    if (m_bac.m_categoryIndex == indexPath.row)
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
  [self.delegate didSelectCategory:self.category withBac:m_bac];
}

- (void)moveBookmarkToSetWithIndex:(int)setIndex
{
  BookmarkAndCategory bac;
  bac.m_bookmarkIndex = static_cast<size_t>(
      GetFramework().MoveBookmark(m_bac.m_bookmarkIndex, m_bac.m_categoryIndex, setIndex));
  bac.m_categoryIndex = setIndex;
  m_bac = bac;

  BookmarkCategory const * category =
      GetFramework().GetBookmarkManager().GetBmCategory(bac.m_categoryIndex);
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
    [self.delegate didSelectCategory:self.category withBac:m_bac];
    [self backTap];
  }
}

@end
