#import "BookmarksVC.h"
#import "CustomNavigationView.h"
#import "MapsAppDelegate.h"
#import "CompassView.h"
#import "BookmarkCell.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "CircleView.h"
#import "ColorPickerView.h"

#include "Framework.h"

#include "../../../geometry/distance_on_sphere.hpp"
#include "../../../coding/zip_creator.hpp"
#include "../../../coding/internal/file_data.hpp"


#define TEXTFIELD_TAG 999
#define PINDIAMETER 18

#define EMPTY_SECTION -666


@interface BookmarksVC()
{
  int m_trackSection;
  int m_bookmarkSection;
  int m_shareSection;
  int m_sectionNumber;
  int m_numberOfSections;
}
@end

@implementation BookmarksVC

- (id) initWithCategory:(size_t)index
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_locationManager = [MapsAppDelegate theApp].m_locationManager;
    m_categoryIndex = index;
    self.title = [NSString stringWithUTF8String:GetFramework().GetBmCategory(index)->GetName().c_str()];
    [self calculateSections];
  }
  return self;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  if ([MFMailComposeViewController canSendMail])
   return 4;
  return 3;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == 0)
    return 2;
  else if (section == m_trackSection)
    return 1; //return GetFramework().GetBmCategory(m_categoryIndex)->trackCount ....
  else if (section == m_bookmarkSection)
    return GetFramework().GetBmCategory(m_categoryIndex)->GetBookmarksCount();
  else if (section == m_shareSection)
    return 1;
  else
    return 0;
}

- (void)onVisibilitySwitched:(UISwitch *)sender
{
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  cat->SetVisible(sender.on);
  cat->SaveToKMLFile();
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == m_trackSection)
    return @"Tracks";
  if (section == m_bookmarkSection)
    return @"Bookmarks";
  return nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Framework & fr = GetFramework();
  BookmarkCategory * cat = fr.GetBmCategory(m_categoryIndex);
  if (!cat)
    return nil;

  UITableViewCell * cell = nil;
  // First section, contains info about current set
  if (indexPath.section == 0)
  {
    if (indexPath.row == 0)
    {
      cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCSetNameCell"];
      if (!cell)
      {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCSetNameCell"] autorelease];
        cell.textLabel.text = NSLocalizedString(@"name", nil);
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        // Temporary, to init font and color
        cell.detailTextLabel.text = @"temp string";
        // Called to initialize frames and fonts
        [cell layoutSubviews];
        CGRect const leftR = cell.textLabel.frame;
        CGFloat const padding = leftR.origin.x;
        CGRect r = CGRectMake(padding + leftR.size.width + padding, leftR.origin.y,
                              cell.contentView.frame.size.width - 3 * padding - leftR.size.width, leftR.size.height);
        UITextField * f = [[[UITextField alloc] initWithFrame:r] autorelease];
        f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        f.enablesReturnKeyAutomatically = YES;
        f.returnKeyType = UIReturnKeyDone;
        f.clearButtonMode = UITextFieldViewModeWhileEditing;
        f.autocorrectionType = UITextAutocorrectionTypeNo;
        f.textAlignment = UITextAlignmentRight;
        f.textColor = cell.detailTextLabel.textColor;
        f.font = [cell.detailTextLabel.font fontWithSize:[cell.detailTextLabel.font pointSize]];
        f.tag = TEXTFIELD_TAG;
        f.delegate = self;
        f.autocapitalizationType = UITextAutocapitalizationTypeWords;
        // Reset temporary font
        cell.detailTextLabel.text = nil;
        [cell.contentView addSubview:f];
      }
      ((UITextField *)[cell.contentView viewWithTag:TEXTFIELD_TAG]).text = [NSString stringWithUTF8String:cat->GetName().c_str()];
    }
    else
    {
      cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCSetVisibilityCell"];
      if (!cell)
      {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCSetVisibilityCell"] autorelease];
        cell.textLabel.text = NSLocalizedString(@"visible", nil);
        cell.accessoryView = [[[UISwitch alloc] init] autorelease];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
      }
      UISwitch * sw = (UISwitch *)cell.accessoryView;
      sw.on = cat->IsVisible();
      [sw addTarget:self action:@selector(onVisibilitySwitched:) forControlEvents:UIControlEventValueChanged];
    }
  }

  if (indexPath.section == m_trackSection)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
    if (!cell)
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];
    cell.textLabel.text = @"Track";
    cell.detailTextLabel.text = @"220 km";
    //get track colour
    cell.imageView.image = [CircleView createCircleImageWith:PINDIAMETER andColor:[ColorPickerView colorForName:[NSString stringWithUTF8String:"placemark-blue"]]];
  }
    // Second third, contains bookmarks list
  if (indexPath.section == m_bookmarkSection)
  {
    BookmarkCell * bmCell = (BookmarkCell *)[tableView dequeueReusableCellWithIdentifier:@"BookmarksVCBookmarkItemCell"];
    if (!bmCell)
      bmCell = [[[BookmarkCell alloc] initWithReuseIdentifier:@"BookmarksVCBookmarkItemCell"] autorelease];
    Bookmark const * bm = cat->GetBookmark(indexPath.row);
    if (bm)
    {
      bmCell.bmName.text = [NSString stringWithUTF8String:bm->GetName().c_str()];
      bmCell.imageView.image = [CircleView createCircleImageWith:PINDIAMETER andColor:[ColorPickerView colorForName:[NSString stringWithUTF8String:bm->GetType().c_str()]]];

      // Get current position and compass "north" direction
      double azimut = -1.0;
      double lat, lon;

      if ([m_locationManager getLat:lat Lon:lon])
      {
        double north = -1.0;
        [m_locationManager getNorthRad:north];

        string distance;
        fr.GetDistanceAndAzimut(bm->GetOrg(), lat, lon, north, distance, azimut);

        bmCell.bmDistance.text = [NSString stringWithUTF8String:distance.c_str()];
      }
      else
        bmCell.bmDistance.text = nil;
    }

    cell = bmCell;
  }

  if (indexPath.section == m_shareSection)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksExportCell"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"BookmarksExportCell"] autorelease];
      cell.textLabel.textAlignment = UITextAlignmentCenter;
      cell.textLabel.text = NSLocalizedString(@"share_by_email", nil);
    }
  }

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];

  Framework & f = GetFramework();
    if (indexPath.section == 0)
    {
      if (indexPath.row == 0)
      {
        // Edit name
        // @TODO
      }
    }
    else if (indexPath.section == m_trackSection)
    {
      //show track
      NSLog(@"show track");
    }

    else if (indexPath.section == m_bookmarkSection)
    {
      BookmarkCategory const * cat = f.GetBmCategory(m_categoryIndex);
      if (cat)
      {
        Bookmark const * bm = cat->GetBookmark(indexPath.row);
        if (bm)
        {
          // Same as "Close".
          f.ShowBookmark(BookmarkAndCategory(m_categoryIndex, indexPath.row));
          [self.navigationController.visibleViewController dismissModalViewControllerAnimated:YES];
        }
      }
    }

    else if (indexPath.section == m_shareSection)
    {
      BookmarkCategory const * cat = GetFramework().GetBmCategory(m_categoryIndex);
      if (cat)
      {
        NSString * filePath = [NSString stringWithUTF8String:cat->GetFileName().c_str()];
        NSMutableString * catName = [NSMutableString stringWithUTF8String:cat->GetName().c_str()];
        if (![catName length])
          [catName setString:@"MapsWithMe"];
        NSMutableString * kmzFile = [NSMutableString stringWithString:filePath];
        [kmzFile replaceCharactersInRange:NSMakeRange([filePath length] - 1, 1) withString:@"z"];
        if (CreateZipFromPathDeflatedAndDefaultCompression([filePath UTF8String], [kmzFile UTF8String]))
        {
          [self sendBookmarksWithExtension:@".kmz" andType:@"application/vnd.google-earth.kmz" andFile:kmzFile andCategory:catName];
        }
        else
        {
          [self sendBookmarksWithExtension:@".kml" andType:@"application/vnd.google-earth.kml+xml" andFile:filePath andCategory:catName];
        }
        my::DeleteFileX([kmzFile UTF8String]);
      }
    }
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
  [self dismissModalViewControllerAnimated:YES];
  [[Statistics instance] logEvent:@"KML Export"];
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == m_trackSection || indexPath.section == m_bookmarkSection)
    return YES;
  return NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == m_trackSection || indexPath.section == m_bookmarkSection)
  {
    BookmarkCategory * cat = nil;
    if (indexPath.section == m_trackSection)
    {
      if (editingStyle == UITableViewCellEditingStyleDelete)
      {
        NSLog(@"Delete");
      }
    }
    else
    {
      if (editingStyle == UITableViewCellEditingStyleDelete)
      {
        cat = GetFramework().GetBmCategory(m_categoryIndex);
        if (cat)
          cat->DeleteBookmark(indexPath.row);
      }
    }
    if (cat)
    {
      cat->SaveToKMLFile();
      [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
      // Disable edit mode if no bookmarks are left
      if (cat->GetBookmarksCount() /*+ tracks count*/ == 0)
      {
        self.navigationItem.rightBarButtonItem = nil;
        [self setEditing:NO animated:YES];
      }
    }
    [self calculateSections];
    [self.tableView reloadData];
  }
}

//******************************************************************
//*********** Location manager callbacks ***************************
- (void)onLocationError:(location::TLocationError)errorCode
{
  // Handle location status changes if necessary
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  // Refresh distance
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  if (cat)
  {
    UITableView * table = (UITableView *)self.view;
    NSArray * cells = [table visibleCells];
    for (NSUInteger i = 0; i < cells.count; ++i)
    {
      BookmarkCell * cell = (BookmarkCell *)[cells objectAtIndex:i];
      NSIndexPath * indexPath = [table indexPathForCell:cell];
      if (indexPath.section == 2)
      {
        Bookmark const * bm = cat->GetBookmark(indexPath.row);
        if (bm)
        {
          m2::PointD const center = bm->GetOrg();
          double const metres = ms::DistanceOnEarth(info.m_latitude, info.m_longitude,
              MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
          cell.bmDistance.text = [LocationManager formatDistance:metres];
        }
      }
    }
  }
}

//*********** End of Location manager callbacks ********************
//******************************************************************

- (void)viewWillAppear:(BOOL)animated
{
  [m_locationManager start:self];

  // Display Edit button only if table is not empty
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  if (cat && cat->GetBookmarksCount())
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
  else
    self.navigationItem.rightBarButtonItem = nil;

  [super viewWillAppear:animated];
}

- (void)renameBMCategoryIfChanged:(NSString *)newName
{
  // Update edited category name
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  char const * newCharName = [newName UTF8String];
  if (cat->GetName() != newCharName)
  {
    cat->SetName(newCharName);
    cat->SaveToKMLFile();
    self.navigationController.title = newName;
  }
}

- (void)viewWillDisappear:(BOOL)animated
{
  [m_locationManager stop:self];

  // Save possibly edited set name
  UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  NSString * newName = ((UITextField *)[cell.contentView viewWithTag:TEXTFIELD_TAG]).text;
  if (newName)
    [self renameBMCategoryIfChanged:newName];

  [super viewWillDisappear:animated];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (textField.text.length == 0)
    return YES;

  // Hide keyboard
  [textField resignFirstResponder];
  [self renameBMCategoryIfChanged:textField.text];
  return NO;
}

- (void) sendBookmarksWithExtension:(NSString *) fileExtension andType:(NSString *)mimeType andFile:(NSString *)filePath andCategory:(NSString *)catName
{
  MFMailComposeViewController * mailVC = [[[MFMailComposeViewController alloc] init] autorelease];
  mailVC.mailComposeDelegate = self;
  [mailVC setSubject:NSLocalizedString(@"share_bookmarks_email_subject", nil)];
  NSData * myData = [[[NSData alloc] initWithContentsOfFile:filePath] autorelease];
  [mailVC addAttachmentData:myData mimeType:mimeType fileName:[NSString stringWithFormat:@"%@%@", catName, fileExtension]];
  [mailVC setMessageBody:[NSString stringWithFormat:NSLocalizedString(@"share_bookmarks_email_body", nil), catName] isHTML:NO];
  [self presentModalViewController:mailVC animated:YES];
}

-(void)calculateSections
{
  size_t index = 1;
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  if (YES)///count tracks
    m_trackSection = index++;
  else
    m_trackSection = EMPTY_SECTION;
  if (cat->GetBookmarksCount())
    m_bookmarkSection = index++;
  else
    m_bookmarkSection = EMPTY_SECTION;
  if ([MFMailComposeViewController canSendMail])
    m_shareSection = index++;
  else
    m_bookmarkSection = EMPTY_SECTION;
  m_numberOfSections = index;
}

@end
