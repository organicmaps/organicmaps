#import "BookmarksVC.h"
#import "SearchCell.h"
#import "CustomNavigationView.h"
#import "BalloonView.h"
#import "MapsAppDelegate.h"
#import "SelectSetVC.h"

#include "Framework.h"


@implementation BookmarksVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    self.title = NSLocalizedString(@"bookmarks", @"Boormarks - dialog title");
    
    self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"maps", @"Bookmarks - Close bookmarks button") style: UIBarButtonItemStyleDone
                                                                           target:self action:@selector(onCloseButton:)] autorelease];
    // Display Edit button only if table is not empty
    BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
    if (cat && cat->GetBookmarksCount())
      self.navigationItem.rightBarButtonItem = self.editButtonItem;
  }
  return self;
}

- (void)onCloseButton:(id)sender
{
  [self dismissModalViewControllerAnimated:YES];
}

- (void)onVisibilitySwitched:(id)sender
{
  BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
  if (cat)
    cat->SetVisible(((UISwitch *)sender).on);
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
  if (section == 0)
    return 1;
  BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
  if (cat)
    return cat->GetBookmarksCount();
  return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 0)
  {
    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarkSetCell"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarkSetCell"] autorelease];
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
      cell.textLabel.text = NSLocalizedString(@"set", @"Bookmarks dialog - Bookmark set cell");
    }
    cell.detailTextLabel.text = m_balloon.setName;
    return cell;
  }
  else
  {
    SearchCell * cell = (SearchCell *)[tableView dequeueReusableCellWithIdentifier:@"FeatureCell"];
    if (!cell)
      cell = [[[SearchCell alloc] initWithReuseIdentifier:@"FeatureCell"] autorelease];

    BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
    if (cat)
    {
      Bookmark const * bm = cat->GetBookmark(indexPath.row);
      if (bm)
      {
        cell.featureName.text = [NSString stringWithUTF8String:bm->GetName().c_str()];
        Framework::AddressInfo info;
        GetFramework().GetAddressInfo(bm->GetOrg(), info);
        cell.featureCountry.text = [NSString stringWithUTF8String:info.FormatAddress().c_str()];
        cell.featureType.text = [NSString stringWithUTF8String:info.FormatTypes().c_str()];
        cell.featureDistance.text = @"@TODO";
      }
    }
    return cell;
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 0)
  {
    SelectSetVC * ssVC = [[SelectSetVC alloc] initWithBalloonView:m_balloon andEditMode:NO];
    [self.navigationController pushViewController:ssVC animated:YES];
    [ssVC release];
  }
  else
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
    if (cat)
    {
      Bookmark const * bm = cat->GetBookmark(indexPath.row);
      if (bm)
      {
        // Same as "Close".
        [self dismissModalViewControllerAnimated:YES];
        GetFramework().ShowRect(bm->GetViewport());
      }
    }
  }
}


- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Return NO if you do not want the specified item to be editable.
  if (indexPath.section == 0)
    return NO;
  return YES;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 1)
  {
    if (editingStyle == UITableViewCellEditingStyleDelete)
    {
      BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
      if (cat)
      {
        cat->DeleteBookmark(indexPath.row);
        [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        // Disable edit mode if no bookmarks are left
        if (cat->GetBookmarksCount() == 0)
        {
          self.navigationItem.rightBarButtonItem = nil;
          [self setEditing:NO animated:YES];
        }
      }
    }
  }
}

@end
