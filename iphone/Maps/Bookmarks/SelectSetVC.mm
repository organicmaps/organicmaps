
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
  [self.ownerNavigationController setNavigationBarHidden:NO];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (!self.ownerNavigationController)
    return;
  self.realPlacePageHeight = self.ownerNavigationController.view.height;
  CGFloat const bottomOffset = 88.;
  self.ownerNavigationController.view.height = self.tableView.height + bottomOffset;
  UIImage * backImage = [UIImage imageNamed:@"NavigationBarBackButton"];
  UIButton * backButton = [[UIButton alloc] initWithFrame:CGRectMake(0., 0., backImage.size.width, backImage.size.height)];
  [backButton addTarget:self action:@selector(backTap:) forControlEvents:UIControlEventTouchUpInside];
  [backButton setImage:backImage forState:UIControlStateNormal];
  UIBarButtonItem * leftButton = [[UIBarButtonItem alloc] initWithCustomView:backButton];
  [self.navigationItem setLeftBarButtonItem:leftButton];
}

- (void)backTap:(id)sender
{
  [self.ownerNavigationController popViewControllerAnimated:YES];
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

    BookmarkAndCategory bac = self.manager.entity.bac;

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
  BookmarkAndCategory bac = entity.bac;
  bac.second = static_cast<int>(GetFramework().MoveBookmark(entity.bac.second, entity.bac.first, setIndex));
  bac.first = setIndex;
  entity.bac = bac;

  BookmarkCategory * category = GetFramework().GetBookmarkManager().GetBmCategory(bac.first);
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
    [self.navigationController popViewControllerAnimated:YES];
  }
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  [self.manager reloadBookmark];

  if (!self.ownerNavigationController)
    return;

  self.ownerNavigationController.navigationBar.hidden = YES;
  [self.ownerNavigationController setNavigationBarHidden:YES];
  self.ownerNavigationController.view.height = self.realPlacePageHeight;
}

@end
