#import "AddSetVC.h"
#import "Common.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "SelectSetVC.h"
#import "UIColor+MapsMeColor.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

@interface SelectSetVC () <AddSetVCDelegate>

@property (weak, nonatomic) MWMPlacePageViewManager * manager;

@end

@implementation SelectSetVC

- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    self.manager = manager;
    self.title = L(@"bookmark_sets");
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self.iPadOwnerNavigationController setNavigationBarHidden:NO];
  if (self.iPadOwnerNavigationController)
  {
    [(UIViewController *)self showBackButton];
    [self.tableView reloadData];
    CGFloat const navBarHeight = self.navigationController.navigationBar.height;
    [UIView animateWithDuration:kDefaultAnimationDuration animations:^
    {
      [self.manager changeHeight:self.tableView.contentSize.height + navBarHeight];
    }];
  }
}

- (void)popViewController
{
  [self.iPadOwnerNavigationController setNavigationBarHidden:YES];
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)backTap
{
  [self popViewController];
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

    BookmarkAndCategory const bac = self.manager.entity.bac;

    if (bac.first == indexPath.row)
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
      cell.accessoryType = UITableViewCellAccessoryNone;
  }
  cell.backgroundColor = [UIColor white];
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  return cell;
}

- (void)addSetVC:(AddSetVC *)vc didAddSetWithIndex:(int)setIndex
{
  [self moveBookmarkToSetWithIndex:setIndex];

  [self.tableView reloadData];
  [self.manager reloadBookmark];
}

- (void)moveBookmarkToSetWithIndex:(int)setIndex
{
  MWMPlacePageEntity * entity = self.manager.entity;
  BookmarkAndCategory bac;
  bac.second = static_cast<int>(GetFramework().MoveBookmark(entity.bac.second, entity.bac.first, setIndex));
  bac.first = setIndex;
  entity.bac = bac;

  BookmarkCategory const * category = GetFramework().GetBookmarkManager().GetBmCategory(bac.first);
  entity.bookmarkCategory = @(category->GetName().c_str());
  [self.manager changeBookmarkCategory:bac];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (indexPath.section == 0)
  {
    AddSetVC * asVC = [[AddSetVC alloc] init];
    asVC.delegate = self;
    if (IPAD)
      asVC.preferredContentSize = self.preferredContentSize;
    [self.navigationController pushViewController:asVC animated:YES];
  }
  else
  {
    [self moveBookmarkToSetWithIndex:static_cast<int>(indexPath.row)];
    [self.manager reloadBookmark];
    [self popViewController];
  }
}

@end
