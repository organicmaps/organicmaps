#import "MWMEditBookmarkController.h"
#import "Common.h"
#import "MWMBookmarkColorViewController.h"
#import "MWMBookmarkTitleCell.h"
#import "MWMButtonCell.h"
#import "MWMNoteCell.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "SelectSetVC.h"
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
  BookmarkAndCategory m_cachedBac;
}

@property (nonatomic) MWMNoteCell * cachedNote;
@property (copy, nonatomic) NSString * cachedDescription;
@property (copy, nonatomic) NSString * cachedTitle;
@property (copy, nonatomic) NSString * cachedColor;
@property (copy, nonatomic) NSString * cachedCategory;

@end

@implementation MWMEditBookmarkController

- (void)viewDidLoad
{
  [super viewDidLoad];
  auto data = self.data;
  NSAssert(self.manager || data, @"Entity and data can't be nil both!");
  if (IPAD)
  {
    MWMPlacePageEntity * en = self.manager.entity;
    self.cachedDescription = en.bookmarkDescription;
    self.cachedTitle = en.bookmarkTitle;
    self.cachedCategory = en.bookmarkCategory;
    self.cachedColor = en.bookmarkColor;
    m_cachedBac = en.bac;
  }
  else
  {
    self.cachedDescription = data.bookmarkDescription;
    self.cachedTitle = data.externalTitle ? data.externalTitle : data.title;
    self.cachedCategory = data.bookmarkCategory;
    self.cachedColor = data.bookmarkColor;
    m_cachedBac = data.bac;
  }

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
  auto registerClass = ^ (Class c)
  {
    NSString * className = NSStringFromClass(c);
    [self.tableView registerNib:[UINib nibWithNibName:className bundle:nil]
         forCellReuseIdentifier:className];
  };

  registerClass([MWMButtonCell class]);
  registerClass([MWMBookmarkTitleCell class]);
  registerClass([MWMNoteCell class]);
}

- (void)onSave
{
  [self.view endEditing:YES];
  if (IPAD)
  {
    MWMPlacePageEntity * en = self.manager.entity;
    en.bookmarkDescription = self.cachedDescription;
    en.bookmarkColor = self.cachedColor;
    en.bookmarkCategory = self.cachedCategory;
    en.bookmarkTitle = self.cachedTitle;
    en.bac = m_cachedBac;
    [en synchronize];
    [self.manager reloadBookmark];
  }
  else
  {
    Framework & f = GetFramework();
    auto const & bac = self.data.bac;
    BookmarkCategory * category = f.GetBmCategory(bac.m_categoryIndex);
    if (!category)
      return;

    {
      BookmarkCategory::Guard guard(*category);
      Bookmark * bookmark =
          static_cast<Bookmark *>(guard.m_controller.GetUserMarkForEdit(bac.m_bookmarkIndex));
      if (!bookmark)
        return;

      bookmark->SetType(self.cachedColor.UTF8String);
      bookmark->SetDescription(self.cachedDescription.UTF8String);
      bookmark->SetName(self.cachedTitle.UTF8String);
    }

    category->SaveToKMLFile();
    f.UpdatePlacePageInfoForCurrentSelection();
  }
  [self backTap];
}

- (void)backTap
{
  if (IPAD)
    [self dismissViewControllerAnimated:YES completion:nil];
  else
    [super backTap];
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
      MWMBookmarkTitleCell * cell = [tableView dequeueReusableCellWithIdentifier:[MWMBookmarkTitleCell className]];
      [cell configureWithName:self.cachedTitle delegate:self];
      return cell;
    }
    case Color:
    {
      UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
      cell.textLabel.text = ios_bookmark_ui_helper::LocalizedTitleForBookmarkColor(self.cachedColor);
      cell.imageView.image = ios_bookmark_ui_helper::ImageForBookmarkColor(self.cachedColor, YES);
      cell.imageView.layer.cornerRadius = cell.imageView.width / 2;
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
      return cell;
    }
    case Category:
    {
      UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
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
      self.cachedNote = [tableView dequeueReusableCellWithIdentifier:[MWMNoteCell className]];
      [self.cachedNote configWithDelegate:self noteText:self.cachedDescription
                              placeholder:L(@"placepage_personal_notes_hint")];
      return self.cachedNote;
    }
  }
  case Delete:
  {
    NSAssert(indexPath.row == 0, @"Incorrect row!");
    MWMButtonCell * cell = [tableView dequeueReusableCellWithIdentifier:[MWMButtonCell className]];
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
                                                          bac:m_cachedBac
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
  [self.manager removeBookmark];
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
  [self.tableView beginUpdates];
  [self.tableView endUpdates];
  [self.tableView scrollToRowAtIndexPath:[self.tableView indexPathForCell:cell]
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

- (void)didSelectCategory:(NSString *)category withBac:(BookmarkAndCategory const &)bac
{
  self.cachedCategory = category;
  m_cachedBac = bac;
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:Category inSection:MetaInfo]] withRowAnimation:UITableViewRowAnimationAutomatic];
}

#pragma mark - MWMBookmarkTitleDelegate

- (void)didFinishEditingBookmarkTitle:(NSString *)title
{
  self.cachedTitle = title;
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:Title inSection:MetaInfo]] withRowAnimation:UITableViewRowAnimationAutomatic];
}

@end
