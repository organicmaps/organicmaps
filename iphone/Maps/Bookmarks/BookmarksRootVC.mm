#import "BookmarksRootVC.h"
#import "BookmarksVC.h"
#import "MWMBookmarksManager.h"
#import "MWMCircularProgress.h"
#import "Statistics.h"
#import "UIImageView+Coloring.h"

#include "Framework.h"

#define TEXTFIELD_TAG 999

extern NSString * const kBookmarkCategoryDeletedNotification =
    @"BookmarkCategoryDeletedNotification";

@interface BookmarksRootVC ()<MWMBookmarksObserver>

@property(nonatomic) MWMCircularProgress * spinner;

@end

@implementation BookmarksRootVC

- (id)init
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    self.title = L(@"bookmarks");

    self.tableView.allowsSelectionDuringEditing = YES;
    [MWMBookmarksManager addObserver:self];
  }
  return self;
}

// Used to display add bookmarks hint
- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  CGRect const rect = tableView.bounds;
  // Use UILabel inside custom view to add padding on the left and right (there is no other way to do it)
  if (!m_hint)
  {
    m_hint = [[UIView alloc] initWithFrame:rect];
    m_hint.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    m_hint.backgroundColor = UIColor.clearColor;

    UILabel * label = [[UILabel alloc] initWithFrame:rect];
    label.backgroundColor = UIColor.clearColor;
    if ([self shouldShowSpinner])
    {
      label.text = L(@"load_kmz_title");
    }
    else
    {
      bool const showDetailedHint = !GetFramework().GetBmCategoriesCount();
      label.text =
          showDetailedHint ? L(@"bookmarks_usage_hint") : L(@"bookmarks_usage_hint_import_only");
    }
    label.textAlignment = NSTextAlignmentCenter;
    label.lineBreakMode = NSLineBreakByWordWrapping;
    label.numberOfLines = 0;
    label.textColor = [UIColor blackPrimaryText];
    [m_hint addSubview:label];
  }
  UILabel * label = m_hint.subviews.firstObject;

  CGFloat offset = 10;
  if (@available(iOS 11.0, *))
  {
    UIEdgeInsets const safeAreaInsets = tableView.safeAreaInsets;
    offset = max(max(safeAreaInsets.top, safeAreaInsets.bottom),
                 max(safeAreaInsets.left, safeAreaInsets.right));
  }
  label.bounds = CGRectInset(rect, offset, offset);
  [label sizeToIntegralFit];
  m_hint.bounds = CGRectMake(0, 0, rect.size.width, label.bounds.size.height + 2 * offset);
  label.center = CGPointMake(m_hint.bounds.size.width / 2, m_hint.bounds.size.height / 2);

  return m_hint.bounds.size.height;
}

- (BOOL)shouldShowSpinner
{
  return ![MWMBookmarksManager areBookmarksLoaded];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return [self shouldShowSpinner] ? 40 : 0;
}

// Used to display hint when no any categories with bookmarks are present
- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  return m_hint;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (![self shouldShowSpinner])
    return nil;
  auto header = [[UIView alloc] initWithFrame:tableView.bounds];
  CGFloat const size = [self tableView:tableView heightForHeaderInSection:section];
  CGFloat const offset = 10;
  CGRect const rect = CGRectInset({{}, {size, size}}, offset, offset);
  auto spinnerView = [[UIView alloc] initWithFrame:rect];
  [header addSubview:spinnerView];
  spinnerView.center = {header.width / 2, spinnerView.center.y};
  self.spinner = [[MWMCircularProgress alloc] initWithParentView:spinnerView];
  self.spinner.state = MWMCircularProgressStateSpinner;
  return header;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return GetFramework().GetBmCategoriesCount();
}

- (void)onEyeTapped:(id)sender
{
  NSInteger row = ((UITapGestureRecognizer *)sender).view.tag;
  UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:row inSection:0]];
  BookmarkCategory * cat = GetFramework().GetBmCategory(row);
  if (cell && cat)
  {
    // Invert visibility
    bool visible = !cat->IsVisible();
    [Statistics logEvent:kStatEventName(kStatBookmarks, kStatToggleVisibility)
                     withParameters:@{kStatValue : visible ? kStatVisible : kStatHidden}];
    cell.imageView.image = [UIImage imageNamed:(visible ? @"ic_show" : @"ic_hide")];
    cell.imageView.mwm_coloring = visible ? MWMImageColoringBlue : MWMImageColoringBlack;
    cat->SetIsVisible(visible);
    cat->NotifyChanges();
    cat->SaveToKMLFile();
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksRootVCSetCell"];
  if (!cell)
  {
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksRootVCSetCell"];
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    // Add "Eye" icon click handler to switch visibility
    cell.imageView.userInteractionEnabled = YES;
    UITapGestureRecognizer * tapped = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onEyeTapped:)];
    tapped.numberOfTapsRequired = 1;
    [cell.imageView addGestureRecognizer:tapped];
  }
  // To detect which row was tapped when user clicked on image
  cell.imageView.tag = indexPath.row;

  BookmarkCategory const * cat = GetFramework().GetBmCategory(indexPath.row);
  if (cat)
  {
    NSString * title = @(cat->GetName().c_str());
    cell.textLabel.text = [self truncateString:title toWidth:(self.tableView.width - 122) withFont:cell.textLabel.font];
    BOOL const isVisible = cat->IsVisible();
    cell.imageView.image = [UIImage imageNamed:(isVisible ? @"ic_show" : @"ic_hide")];
    cell.imageView.mwm_coloring = isVisible ? MWMImageColoringBlue : MWMImageColoringBlack;
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%ld", cat->GetUserMarkCount() + cat->GetTracksCount()];
  }
  cell.backgroundColor = [UIColor white];
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  return cell;
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  [coordinator
      animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        NSArray<NSIndexPath *> * ips = self.tableView.indexPathsForVisibleRows;
        [self.tableView reloadRowsAtIndexPaths:ips withRowAnimation:UITableViewRowAnimationFade];
      }
                      completion:nil];
}

- (NSString *)truncateString:(NSString *)string toWidth:(CGFloat)width withFont:(UIFont *)font
{
  CGFloat tailLength = 3;
  CGFloat incrementStep = 1;
  if ([string length] < tailLength + incrementStep)
    return string;
  BOOL firstTime = YES;
  NSDictionary * attrs = @{NSFontAttributeName:font};
  while ([string sizeWithAttributes:attrs].width > width)
  {
    if (!firstTime)
      string = [[string substringToIndex:([string length] - tailLength - incrementStep)] stringByAppendingString:@"..."];
    firstTime = NO;
  }
  return string;
}

- (void)applyCategoryRenaming
{
  for (UITableViewCell * cell in self.tableView.visibleCells)
  {
    UITextField * f = (UITextField *)[cell viewWithTag:TEXTFIELD_TAG];
    if (f)
    {
      NSString * txt = f.text;
      // Update edited category name
      NSString * cellLabel = cell.textLabel.text;
      if (txt.length && ![txt isEqualToString:cellLabel])
      {
        cell.textLabel.text = txt;
        // Rename category
        BookmarkCategory * cat = GetFramework().GetBmCategory([self.tableView indexPathForCell:cell].row);
        if (cat)
        {
          cat->SetName(txt.UTF8String);
          cat->SaveToKMLFile();
        }
      }
      [f removeFromSuperview];
      cell.textLabel.hidden = NO;
      cell.detailTextLabel.hidden = NO;
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
      break;
    }
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];

  if (tableView.editing)
  {
    [self applyCategoryRenaming];
    CGRect r = cell.textLabel.frame;
    r.size.width = cell.contentView.bounds.size.width - r.origin.x;
    UITextField * f = [[UITextField alloc] initWithFrame:r];
    f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    f.returnKeyType = UIReturnKeyDone;
    f.enablesReturnKeyAutomatically = YES;
    f.clearButtonMode = UITextFieldViewModeWhileEditing;
    f.autocorrectionType = UITextAutocorrectionTypeNo;
    f.adjustsFontSizeToFitWidth = YES;
    f.text = cell.textLabel.text;
    f.textColor = cell.detailTextLabel.textColor;
    f.placeholder = L(@"bookmark_set_name");
    f.font = [cell.textLabel.font fontWithSize:[cell.textLabel.font pointSize]];
    f.tag = TEXTFIELD_TAG;
    f.delegate = self;
    f.autocapitalizationType = UITextAutocapitalizationTypeSentences;
    cell.textLabel.hidden = YES;
    cell.detailTextLabel.hidden = YES;
    cell.accessoryType = UITableViewCellAccessoryNone;
    [cell.contentView addSubview:f];
    [f becomeFirstResponder];
  }
  else
  {
    BookmarksVC * bvc = [[BookmarksVC alloc] initWithCategory:indexPath.row];
    [self.navigationController pushViewController:bvc animated:YES];
  }
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  return YES;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (editingStyle == UITableViewCellEditingStyleDelete)
  {
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatRemove)];
    [NSNotificationCenter.defaultCenter postNotificationName:kBookmarkCategoryDeletedNotification
                                                      object:@(indexPath.row)];
    Framework & f = GetFramework();
    f.DeleteBmCategory(indexPath.row);
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    // Disable edit mode if no categories are left
    if (!f.GetBmCategoriesCount())
    {
      self.navigationItem.rightBarButtonItem = nil;
      [self setEditing:NO animated:YES];
    }
  }
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  // Display Edit button only if table is not empty
  if (GetFramework().GetBmCategoriesCount())
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
  else
    self.navigationItem.rightBarButtonItem = nil;

  // Always reload table - we can open it after deleting bookmarks in any category
  [self.tableView reloadData];
}

// Used to remove active UITextField from the cell
- (void)setEditing:(BOOL)editing animated:(BOOL)animated
{
  if (editing == NO)
    [self applyCategoryRenaming];

  [super setEditing:editing animated:animated];

  // Set or remove selection style for all cells
  NSInteger const rowsCount = [self.tableView numberOfRowsInSection:0];
  for (NSInteger i = 0; i < rowsCount; ++i)
  {
    UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:0]];
    if (self.editing)
    {
      cell.selectionStyle = UITableViewCellSelectionStyleNone;
      cell.textLabel.textColor = cell.detailTextLabel.textColor;
    }
    else
    {
      cell.selectionStyle = UITableViewCellSelectionStyleBlue;
      cell.textLabel.textColor = [UIColor blackPrimaryText];
    }
  }
}

// To hide keyboard and apply changes
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [Statistics logEvent:kStatEventName(kStatBookmarks, kStatRename)];
  if (textField.text.length == 0)
    return YES;

  [textField resignFirstResponder];
  [self applyCategoryRenaming];
  // Exit from edit mode
  [self setEditing:NO animated:YES];
  return NO;
}

#pragma mark - MWMBookmarksObserver
- (void)onBookmarksLoadFinished { [self.tableView reloadData]; }

@end
