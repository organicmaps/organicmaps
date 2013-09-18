#import "BookmarksRootVC.h"
#import "BookmarksVC.h"
#import "Statistics.h"

#include "Framework.h"

#include "../../platform/platform.hpp"


#define TEXTFIELD_TAG 999


@implementation BookmarksRootVC

- (id) init
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    self.title = NSLocalizedString(@"bookmarks", @"Boormarks - dialog title");

    self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc]
        initWithTitle:NSLocalizedString(@"maps", @"Bookmarks - Close bookmarks button")
        style: UIBarButtonItemStyleDone
        target:self
        action:@selector(onCloseButton:)] autorelease];
    self.tableView.allowsSelectionDuringEditing = YES;
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(newCategoryAdded)
                                                 name:@"KML file added"
                                               object:nil];
  }
  return self;
}

- (void)onCloseButton:(id)sender
{
  [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

// Used to display add bookmarks hint
- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return tableView.bounds.size.height / 3.;
}

// Used to display hint when no any categories with bookmarks are present
- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  CGRect rect = tableView.bounds;
  rect.size.height /= 3.;
  // Use Label in custom view to add padding on the left and right (there is no other way to do it)
  UIView * customView = [[[UIView alloc] initWithFrame:rect] autorelease];
  customView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  customView.backgroundColor = [UIColor clearColor];

  UILabel * hint = [[[UILabel alloc] initWithFrame:CGRectInset(rect, 40, 0)] autorelease];
  // Orientation support
  hint.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  hint.backgroundColor = [UIColor clearColor];
  hint.text = NSLocalizedString(@"bookmarks_usage_hint", @"Text hint in Bookmarks dialog, displayed if it's empty");
  hint.textAlignment = UITextAlignmentCenter;
  hint.lineBreakMode = UILineBreakModeWordWrap;
  hint.numberOfLines = 0;

  [customView addSubview:hint];

  return customView;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return GetFramework().GetBmCategoriesCount();
}

-(void)onEyeTapped:(id)sender
{
  NSInteger row = ((UITapGestureRecognizer *)sender).view.tag;
  UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:row inSection:0]];
  BookmarkCategory * cat = GetFramework().GetBmCategory(row);
  if (cell && cat)
  {
    // Invert visibility
    bool visible = !cat->IsVisible();
    cell.imageView.image = [UIImage imageNamed:(visible ? @"eye" : @"empty")];
    cat->SetVisible(visible);
    cat->SaveToKMLFile();
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksRootVCSetCell"];
  if (!cell)
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksRootVCSetCell"] autorelease];
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    // Add "Eye" icon click handler to switch visibility
    cell.imageView.userInteractionEnabled = YES;
    UITapGestureRecognizer * tapped = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onEyeTapped:)] autorelease];
    tapped.numberOfTapsRequired = 1;
    [cell.imageView addGestureRecognizer:tapped];
  }
  // To detect which row was tapped when user clicked on image
  cell.imageView.tag = indexPath.row;

  BookmarkCategory const * cat = GetFramework().GetBmCategory(indexPath.row);
  if (cat)
  {
    cell.textLabel.text = [NSString stringWithUTF8String:cat->GetName().c_str()];
    cell.imageView.image = [UIImage imageNamed:(cat->IsVisible() ? @"eye" : @"empty")];
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%ld", cat->GetBookmarksCount()];
  }
  return cell;
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
      if (txt.length && ![txt isEqualToString:cell.textLabel.text])
      {
        cell.textLabel.text = txt;
        // Rename category
        BookmarkCategory * cat = GetFramework().GetBmCategory([self.tableView indexPathForCell:cell].row);
        if (cat)
        {
          cat->SetName([txt UTF8String]);
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
    UITextField * f = [[[UITextField alloc] initWithFrame:r] autorelease];
    f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    f.returnKeyType = UIReturnKeyDone;
    f.enablesReturnKeyAutomatically = YES;
    f.clearButtonMode = UITextFieldViewModeWhileEditing;
    f.autocorrectionType = UITextAutocorrectionTypeNo;
    f.adjustsFontSizeToFitWidth = YES;
    f.text = cell.textLabel.text;
    f.textColor = cell.detailTextLabel.textColor;
    f.placeholder = NSLocalizedString(@"bookmark_set_name", @"Add Bookmark Set dialog - hint when set name is empty");
    f.font = [cell.textLabel.font fontWithSize:[cell.textLabel.font pointSize]];
    f.tag = TEXTFIELD_TAG;
    f.delegate = self;
    f.autocapitalizationType = UITextAutocapitalizationTypeWords;
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
    [bvc release];
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
    Framework & f = GetFramework();
    f.DeleteBmCategory(indexPath.row);
    [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
    // Disable edit mode if no categories are left
    if (!f.GetBmCategoriesCount())
    {
      self.navigationItem.rightBarButtonItem = nil;
      [self setEditing:NO animated:YES];
    }
  }
}

// Banner dialog handler
- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    // Launch appstore
    [APP openURL:[NSURL URLWithString:MAPSWITHME_PREMIUM_APPSTORE_URL]];
    [[Statistics instance] logProposalReason:@"Bookmark Screen" withAnswer:@"YES"];
  }
  else
    [[Statistics instance] logProposalReason:@"Bookmark Screen" withAnswer:@"NO"];
  // Close view
  [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillAppear:(BOOL)animated
{
  // Disable bookmarks for free version
  if (!GetPlatform().HasBookmarks())
  {
    // Display banner for paid version
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"bookmarks_in_pro_version", nil)
                                    message:nil
                                    delegate:self
                                    cancelButtonTitle:NSLocalizedString(@"cancel", nil)
                                    otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];

    [alert show];
    [alert release];
  }
  else
  {
    // Display Edit button only if table is not empty
    if (GetFramework().GetBmCategoriesCount())
      self.navigationItem.rightBarButtonItem = self.editButtonItem;
    else
      self.navigationItem.rightBarButtonItem = nil;

    // Always reload table - we can open it after deleting bookmarks in any category
    [self.tableView reloadData];
  }
  [super viewWillAppear:animated];
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
      cell.textLabel.textColor = [UIColor blackColor];
    }
  }
}

// To hide keyboard and apply changes
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (textField.text.length == 0)
    return YES;

  [textField resignFirstResponder];
  [self applyCategoryRenaming];
  // Exit from edit mode
  [self setEditing:NO animated:YES];
  return NO;
}

-(void)newCategoryAdded
{
  [self.tableView reloadData];
}

-(void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (BOOL)prefersStatusBarHidden
{
  return YES;
}

@end
