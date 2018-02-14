#import "MWMEditBookmarkController.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMBookmarkTitleCell.h"
#import "MWMButtonCell.h"
#import "MWMNoteCell.h"
#import "MWMPlacePageData.h"
#import "SelectSetVC.h"
#import "SwiftBridge.h"
#import "UIImageView+Coloring.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

namespace
{
enum Sections
{
  MetaInfo,
  Description,
  Delete,
  SectionsCount
};

enum RowInMetaInfo
{
  Title,
  Color,
  Category,
  RowsInMetaInfoCount
};
}  // namespace

@interface MWMEditBookmarkController () <MWMButtonCellDelegate, MWMNoteCelLDelegate, MWMBookmarkColorDelegate,
                                         MWMSelectSetDelegate, MWMBookmarkTitleDelegate>
{
  df::MarkID m_cachedBookmarkId;
  df::MarkGroupID m_cachedBookmarkCatId;
}

@property (nonatomic) MWMNoteCell * cachedNote;
@property (copy, nonatomic) NSString * cachedDescription;
@property (copy, nonatomic) NSString * cachedTitle;
@property (copy, nonatomic) NSString * cachedColor;
@property (copy, nonatomic) NSString * cachedCategory;
@property(nonatomic) df::MarkGroupID cachedNewBookmarkCatId;

@end

@implementation MWMEditBookmarkController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.cachedNewBookmarkCatId = df::kInvalidMarkGroupId;
  auto data = self.data;
  NSAssert(data, @"Data can't be nil!");
  self.cachedDescription = data.bookmarkDescription;
  self.cachedTitle = data.title;
  self.cachedCategory = data.bookmarkCategory;
  self.cachedColor = data.bookmarkColor;
  m_cachedBookmarkId = data.bookmarkId;
  m_cachedBookmarkCatId = data.bookmarkCategoryId;
  [self configNavBar];
  [self registerCells];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [self.cachedNote updateTextViewForHeight:self.cachedNote.textViewContentHeight];
}

- (void)configNavBar
{
  [self showBackButton];
  self.title = L(@"bookmark").capitalizedString;
  self.navigationItem.rightBarButtonItem =
  [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave
                                                target:self
                                                action:@selector(onSave)];
}

- (void)registerCells
{
  UITableView * tv = self.tableView;
  [tv registerWithCellClass:[MWMButtonCell class]];
  [tv registerWithCellClass:[MWMBookmarkTitleCell class]];
  [tv registerWithCellClass:[MWMNoteCell class]];
}

- (void)onSave
{
  [self.view endEditing:YES];
  auto & f = GetFramework();
  BookmarkManager & bmManager = f.GetBookmarkManager();
  auto editSession = bmManager.GetEditSession();
  if (self.cachedNewBookmarkCatId != df::kInvalidMarkGroupId)
  {
    editSession.MoveBookmark(m_cachedBookmarkId, m_cachedBookmarkCatId, self.cachedNewBookmarkCatId);
    m_cachedBookmarkCatId = self.cachedNewBookmarkCatId;
  }

  auto bookmark = editSession.GetBookmarkForEdit(m_cachedBookmarkId);
  if (!bookmark)
    return;

  bookmark->SetType(self.cachedColor.UTF8String);
  bookmark->SetDescription(self.cachedDescription.UTF8String);
  bookmark->SetName(self.cachedTitle.UTF8String);
  
  f.UpdatePlacePageInfoForCurrentSelection();
  [self backTap];
}

#pragma mark - UITableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return SectionsCount;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  switch (section)
  {
  case MetaInfo:
    return RowsInMetaInfoCount;
  case Description:
  case Delete:
    return 1;
  default:
    NSAssert(false, @"Incorrect section!");
    return 0;
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch (indexPath.section)
  {
  case MetaInfo:
    switch (indexPath.row)
    {
    case Title:
    {
      Class cls = [MWMBookmarkTitleCell class];
      auto cell = static_cast<MWMBookmarkTitleCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [cell configureWithName:self.cachedTitle delegate:self];
      return cell;
    }
    case Color:
    {
      Class cls = [UITableViewCell class];
      auto cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
      cell.textLabel.text = ios_bookmark_ui_helper::LocalizedTitleForBookmarkColor(self.cachedColor);
      cell.imageView.image = ios_bookmark_ui_helper::ImageForBookmarkColor(self.cachedColor, YES);
      cell.imageView.layer.cornerRadius = cell.imageView.width / 2;
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
      return cell;
    }
    case Category:
    {
      Class cls = [UITableViewCell class];
      auto cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
      cell.textLabel.text = self.cachedCategory;
      cell.imageView.image = [UIImage imageNamed:@"ic_folder"];
      cell.imageView.mwm_coloring = MWMImageColoringBlack;
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
      return cell;
    }
    default:
      NSAssert(false, @"Incorrect row!");
      return nil;
    }
  case Description:
  {
    NSAssert(indexPath.row == 0, @"Incorrect row!");
    if (self.cachedNote)
    {
      return self.cachedNote;
    }
    else
    {
      Class cls = [MWMNoteCell class];
      self.cachedNote = static_cast<MWMNoteCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [self.cachedNote configWithDelegate:self noteText:self.cachedDescription
                              placeholder:L(@"placepage_personal_notes_hint")];
      return self.cachedNote;
    }
  }
  case Delete:
  {
    NSAssert(indexPath.row == 0, @"Incorrect row!");
    Class cls = [MWMButtonCell class];
    auto cell = static_cast<MWMButtonCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [cell configureWithDelegate:self title:L(@"placepage_delete_bookmark_button")];
    return cell;
  }
  default:
    NSAssert(false, @"Incorrect section!");
    return nil;
  }
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == Description)
  {
    NSAssert(indexPath.row == 0, @"Incorrect row!");
    return self.cachedNote ? self.cachedNote.cellHeight : [MWMNoteCell minimalHeight];
  }
  return self.tableView.rowHeight;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  switch (indexPath.row)
  {
  case Color:
  {
    MWMBookmarkColorViewController * cvc = [[MWMBookmarkColorViewController alloc] initWithColor:self.cachedColor
                                                                                        delegate:self];
    [self.navigationController pushViewController:cvc animated:YES];
    break;
  }
  case Category:
  {
    SelectSetVC * svc = [[SelectSetVC alloc] initWithCategory:self.cachedCategory
                                                   categoryId:m_cachedBookmarkCatId
                                                     delegate:self];
    [self.navigationController pushViewController:svc animated:YES];
    break;
  }
  default:
    break;
  }
}

#pragma mark - MWMButtonCellDelegate

- (void)cellSelect:(UITableViewCell *)cell
{
  [self.data updateBookmarkStatus:NO];
  GetFramework().UpdatePlacePageInfoForCurrentSelection();
  [self backTap];
}

#pragma mark - MWMNoteCellDelegate

- (void)cell:(MWMNoteCell *)cell didFinishEditingWithText:(NSString *)text
{
  self.cachedDescription = text;
}

- (void)cellShouldChangeSize:(MWMNoteCell *)cell text:(NSString *)text
{
  self.cachedDescription = text;
  [self.tableView refresh];
  NSIndexPath * ip = [self.tableView indexPathForCell:cell];
  [self.tableView scrollToRowAtIndexPath:ip
                        atScrollPosition:UITableViewScrollPositionBottom
                                animated:YES];
}

#pragma mark - MWMBookmarkColorDelegate

- (void)didSelectColor:(NSString *)color
{
  self.cachedColor = color;
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:Color inSection:MetaInfo]] withRowAnimation:UITableViewRowAnimationAutomatic];
}

#pragma mark - MWMSelectSetDelegate

- (void)didSelectCategory:(NSString *)category withCategoryId:(df::MarkGroupID)categoryId
{
  self.cachedCategory = category;
  self.cachedNewBookmarkCatId = categoryId;
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:Category inSection:MetaInfo]] withRowAnimation:UITableViewRowAnimationAutomatic];
}

#pragma mark - MWMBookmarkTitleDelegate

- (void)didFinishEditingBookmarkTitle:(NSString *)title
{
  self.cachedTitle = title;
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:Title inSection:MetaInfo]] withRowAnimation:UITableViewRowAnimationAutomatic];
}

@end
