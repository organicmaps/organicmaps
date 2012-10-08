#import "BookmarksRootVC.h"
#import "BookmarksVC.h"

#include "Framework.h"

@implementation BookmarksRootVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    self.title = NSLocalizedString(@"bookmarks", @"Boormarks - dialog title");

    self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc]
        initWithTitle:NSLocalizedString(@"maps", @"Bookmarks - Close bookmarks button")
        style: UIBarButtonItemStyleDone
        target:self
        action:@selector(onCloseButton:)] autorelease];
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

// Used to display bookmarks hint when no any bookmarks are added
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  // Display hint only if at least one category with bookmarks is present
  if (GetFramework().GetBmCategoriesCount())
    return 0.;
  return tableView.bounds.size.height / 2.;
}

// Used to display hint when no any categories with bookmarks are present
- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (GetFramework().GetBmCategoriesCount())
    return nil;

  CGRect rect = tableView.bounds;
  rect.size.height /= 2.;
  rect.size.width = rect.size.width * 2./3.;
  UILabel * hint = [[[UILabel alloc] initWithFrame:rect] autorelease];
  hint.textAlignment = UITextAlignmentCenter;
  hint.lineBreakMode = UILineBreakModeWordWrap;
  hint.numberOfLines = 0;
  hint.text = NSLocalizedString(@"bookmarks_usage_hint", @"Text hint in Bookmarks dialog, displayed if it's empty");
  hint.backgroundColor = [UIColor clearColor];
  return hint;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return GetFramework().GetBmCategoriesCount();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksRootVCSetCell"];
  if (!cell)
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksRootVCSetCell"] autorelease];
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  }

  BookmarkCategory * cat = GetFramework().GetBmCategory(indexPath.row);
  if (cat)
  {
    cell.textLabel.text = [NSString stringWithUTF8String:cat->GetName().c_str()];
    // @TODO: add checkmark icon
    //cell.imageView.image = cat->IsVisible() ? checkedImage : nil;
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%ld", cat->GetBookmarksCount()];
  }
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];
  BookmarksVC * bvc = [[BookmarksVC alloc] initWithCategory:indexPath.row];
  [self.navigationController pushViewController:bvc animated:YES];
  [bvc release];
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

- (void)viewWillAppear:(BOOL)animated
{
  // Display Edit button only if table is not empty
  if (GetFramework().GetBmCategoriesCount())
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
  else
    self.navigationItem.rightBarButtonItem = nil;

  // Always reload table - we can open it after deleting bookmarks in any category
  [self.tableView reloadData];
  [super viewWillAppear:animated];
}

@end
