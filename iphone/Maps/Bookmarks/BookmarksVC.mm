#import "BookmarksVC.h"
#import "SearchCell.h"
#import "CustomNavigationView.h"

#include "../../map/Framework.hpp"


@implementation BookmarksVC

- (void)onCancelEdit
{
  [self setEditing:NO animated:NO];
  [m_table setEditing:NO animated:NO];
  m_navItem.rightBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemEdit target:self action:@selector(onEdit)] autorelease];
}

- (void)onEdit
{
  [self setEditing:YES animated:YES];
  [m_table setEditing:YES animated:YES];
  m_navItem.rightBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(onCancelEdit)] autorelease];
}

- (id)initWithFramework:(Framework *)f
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    m_framework = f;
  }
  return self;
}

- (void)loadView
{
  UIView * parentView = [[[CustomNavigationView alloc] init] autorelease];
  parentView.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;

  m_navItem = [[[UINavigationItem alloc] init] autorelease];

  m_navItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Maps", @"Bookmarks - Close bookmarks button") style: UIBarButtonItemStyleDone
                                                                   target:self action:@selector(onCloseButton:)] autorelease];
  // Display Edit button only if table is not empty
  if (m_framework->BookmarksCount())
    m_navItem.rightBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemEdit target:self action:@selector(onEdit)] autorelease];

  UINavigationBar * navBar = [[[UINavigationBar alloc] init] autorelease];
  [navBar pushNavigationItem:m_navItem animated:NO];

  [parentView addSubview:navBar];

  m_table = [[UITableView alloc] init];
  m_table.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
  m_table.delegate = self;
  m_table.dataSource = self;
  [parentView addSubview:m_table];

  self.view = parentView;
}

- (void)onCloseButton:(id)sender
{
  [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  // Return the number of rows in the section.
  return m_framework->BookmarksCount();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  SearchCell * cell = (SearchCell *)[tableView dequeueReusableCellWithIdentifier:@"FeatureCell"];
  if (!cell)
    cell = [[[SearchCell alloc] initWithReuseIdentifier:@"FeatureCell"] autorelease];

  Bookmark bm;
  m_framework->GetBookmark(indexPath.row, bm);

  cell.featureName.text = [NSString stringWithUTF8String:bm.GetName().c_str()];
  cell.featureCountry.text = [NSString stringWithUTF8String:"Region"];
  cell.featureType.text = [NSString stringWithUTF8String:"Type"];
  cell.featureDistance.text = [NSString stringWithFormat:@"%f", 0.0];

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < (NSInteger)m_framework->BookmarksCount())
  {
    Bookmark bm;
    m_framework->GetBookmark(indexPath.row, bm);
    m_framework->ShowRect(bm.GetViewport());

    // Same as "Close".
    [self dismissModalViewControllerAnimated:YES];
  }
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (editingStyle == UITableViewCellEditingStyleDelete)
  {
    m_framework->RemoveBookmark(indexPath.row);
    [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
  }
//  else if (editingStyle == UITableViewCellEditingStyleInsert)
//  {
//  }
}

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

@end
