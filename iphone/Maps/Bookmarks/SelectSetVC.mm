
#import "SelectSetVC.h"
#import "Framework.h"
#import "AddSetVC.h"
#import "UIKitCategories.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageEntity.h"

@interface SelectSetVC () <AddSetVCDelegate>

@property (weak, nonatomic) MWMPlacePageViewManager * manager;
@property (nonatomic) CGFloat realPlacePageHeight;

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

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (!self.iPadOwnerNavigationController)
    return;
  [self.iPadOwnerNavigationController setNavigationBarHidden:NO];
  self.realPlacePageHeight = self.iPadOwnerNavigationController.view.height;
  UIImage * backImage = [UIImage imageNamed:@"NavigationBarBackButton"];
  UIButton * backButton = [[UIButton alloc] initWithFrame:CGRectMake(0., 0., backImage.size.width, backImage.size.height)];
  [backButton addTarget:self action:@selector(backTap) forControlEvents:UIControlEventTouchUpInside];
  [backButton setImage:backImage forState:UIControlStateNormal];
  [self.navigationItem setLeftBarButtonItem:[[UIBarButtonItem alloc] initWithCustomView:backButton]];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (!self.iPadOwnerNavigationController)
    return;

  CGFloat const bottomOffset = 88.;
  self.iPadOwnerNavigationController.view.height = self.tableView.height + bottomOffset;
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

    BookmarkAndCategory const bac = self.manager.entity.bac;

    if (bac.first == indexPath.row)
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
  entity.bookmarkCategory = [NSString stringWithUTF8String:category->GetName().c_str()];
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
    [self.manager reloadBookmark];
    [self popViewController];
  }
}

- (void)popViewController
{
  if (!self.iPadOwnerNavigationController)
  {
    [self.navigationController popViewControllerAnimated:YES];
    return;
  }

  [UIView animateWithDuration:0.1 animations:^
   {
     self.iPadOwnerNavigationController.view.height = self.realPlacePageHeight;
   }
                   completion:^(BOOL finished)
   {
     [self.navigationController popViewControllerAnimated:YES];
   }];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  if (!self.iPadOwnerNavigationController)
    return;

  self.iPadOwnerNavigationController.navigationBar.hidden = YES;
}

@end
