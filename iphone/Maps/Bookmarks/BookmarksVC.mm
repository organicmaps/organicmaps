#import "BookmarkCell.h"
#import "BookmarksVC.h"
#import "CircleView.h"
#import "ColorPickerView.h"
#import "Common.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapViewControlsManager.h"
#import "Statistics.h"
#import <MessageUI/MFMailComposeViewController.h>

#include "Framework.h"

#include "platform/measurement_utils.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "coding/zip_creator.hpp"
#include "coding/internal/file_data.hpp"


#define TEXTFIELD_TAG 999
#define PINDIAMETER 18

#define EMPTY_SECTION -666

extern NSString * const kBookmarksChangedNotification = @"BookmarksChangedNotification";

@interface BookmarksVC() <MFMailComposeViewControllerDelegate>
{
  int m_trackSection;
  int m_bookmarkSection;
  int m_shareSection;
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
    self.title = @(GetFramework().GetBmCategory(index)->GetName().c_str());
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
  return m_numberOfSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == 0)
    return 2;
  else if (section == m_trackSection)
    return GetFramework().GetBmCategory(m_categoryIndex)->GetTracksCount();
  else if (section == m_bookmarkSection)
    return GetFramework().GetBmCategory(m_categoryIndex)->GetUserMarkCount();
  else if (section == m_shareSection)
    return 1;
  else
    return 0;
}

- (void)onVisibilitySwitched:(UISwitch *)sender
{
  [[Statistics instance] logEvent:kStatEventName(kStatBookmarks, kStatToggleVisibility)
                   withParameters:@{kStatValue : sender.on ? kStatVisible : kStatHidden}];
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  {
    BookmarkCategory::Guard guard(*cat);
    guard.m_controller.SetIsVisible(sender.on);
  }
  cat->SaveToKMLFile();
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == m_trackSection)
    return L(@"tracks");
  if (section == m_bookmarkSection)
    return L(@"bookmarks");
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
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCSetNameCell"];
        cell.textLabel.text = L(@"name");
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        // Temporary, to init font and color
        cell.detailTextLabel.text = @"temp string";
        // Called to initialize frames and fonts
        [cell layoutIfNeeded];
        CGRect const leftR = cell.textLabel.frame;
        CGFloat const padding = leftR.origin.x;

        UITextField * f = [[UITextField alloc] initWithFrame:{{padding + leftR.size.width + padding + leftR.origin.x,
                                                              leftR.origin.y},
                                                              {cell.contentView.frame.size.width - 3 * padding - leftR.size.width,
                                                              leftR.size.height}}];
        f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        f.enablesReturnKeyAutomatically = YES;
        f.returnKeyType = UIReturnKeyDone;
        f.clearButtonMode = UITextFieldViewModeWhileEditing;
        f.autocorrectionType = UITextAutocorrectionTypeNo;
        f.textAlignment = NSTextAlignmentRight;
        f.textColor = cell.detailTextLabel.textColor;
        f.font = [cell.detailTextLabel.font fontWithSize:[cell.detailTextLabel.font pointSize]];
        f.tag = TEXTFIELD_TAG;
        f.delegate = self;
        f.autocapitalizationType = UITextAutocapitalizationTypeWords;
        // Reset temporary font
        cell.detailTextLabel.text = nil;
        [cell.contentView addSubview:f];
      }
      ((UITextField *)[cell.contentView viewWithTag:TEXTFIELD_TAG]).text = @(cat->GetName().c_str());
    }
    else
    {
      cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCSetVisibilityCell"];
      if (!cell)
      {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCSetVisibilityCell"];
        cell.textLabel.text = L(@"visible");
        cell.accessoryView = [[UISwitch alloc] init];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
      }
      UISwitch * sw = (UISwitch *)cell.accessoryView;
      sw.on = cat->IsVisible();
      [sw addTarget:self action:@selector(onVisibilitySwitched:) forControlEvents:UIControlEventValueChanged];
    }
  }

  else if (indexPath.section == m_trackSection)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
    if (!cell)
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];
    Track const * tr = cat->GetTrack(indexPath.row);
    cell.textLabel.text = @(tr->GetName().c_str());
    string dist;
    if (MeasurementUtils::FormatDistance(tr->GetLengthMeters(), dist))
      //Change Length before release!!!
      cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"length"), [NSString  stringWithUTF8String:dist.c_str()]];
    else
      cell.detailTextLabel.text = nil;
    const dp::Color c = tr->GetColor(0);
    cell.imageView.image = [CircleView createCircleImageWith:PINDIAMETER andColor:[UIColor colorWithRed:c.GetRed()/255.f green:c.GetGreen()/255.f
                                                                                                   blue:c.GetBlue()/255.f alpha:1.f]];
  }
  // Contains bookmarks list
  else if (indexPath.section == m_bookmarkSection)
  {
    BookmarkCell * bmCell = (BookmarkCell *)[tableView dequeueReusableCellWithIdentifier:@"BookmarksVCBookmarkItemCell"];
    if (!bmCell)
      bmCell = [[BookmarkCell alloc] initWithReuseIdentifier:@"BookmarksVCBookmarkItemCell"];
    Bookmark const * bm = static_cast<Bookmark const *>(cat->GetUserMark(indexPath.row));
    if (bm)
    {
      bmCell.bmName.text = @(bm->GetName().c_str());
      bmCell.imageView.image = [CircleView createCircleImageWith:PINDIAMETER andColor:[ColorPickerView colorForName:@(bm->GetType().c_str())]];

      // Get current position and compass "north" direction
      double azimut = -1.0;
      double lat, lon;

      if ([m_locationManager getLat:lat Lon:lon])
      {
        double north = -1.0;
        [m_locationManager getNorthRad:north];

        string distance;
        fr.GetDistanceAndAzimut(bm->GetPivot(), lat, lon, north, distance, azimut);

        bmCell.bmDistance.text = @(distance.c_str());
      }
      else
        bmCell.bmDistance.text = nil;
    }
    else
      ASSERT(false, ("NULL bookmark"));

    cell = bmCell;
  }

  else if (indexPath.section == m_shareSection)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksExportCell"];
    if (!cell)
    {
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"BookmarksExportCell"];
      cell.textLabel.textAlignment = NSTextAlignmentCenter;
      cell.textLabel.text = L(@"share_by_email");
    }
  }

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];

  Framework & f = GetFramework();
  BookmarkCategory const * cat = f.GetBmCategory(m_categoryIndex);
  ASSERT(cat, ("NULL category"));
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
    if (cat)
    {
      Track const * tr = cat->GetTrack(indexPath.row);
      ASSERT(tr, ("NULL track"));
      if (tr)
      {
        f.ShowTrack(*tr);
        [self.navigationController popToRootViewControllerAnimated:YES];
      }
    }
  }
  else if (indexPath.section == m_bookmarkSection)
  {
    if (cat)
    {
      Bookmark const * bm = static_cast<Bookmark const *>(cat->GetUserMark(indexPath.row));
      ASSERT(bm, ("NULL bookmark"));
      if (bm)
      {
        [[Statistics instance] logEvent:kStatEventName(kStatBookmarks, kStatShowOnMap)];
        // Same as "Close".
        MapViewController * mapVC = self.navigationController.viewControllers.firstObject;
        mapVC.controlsManager.searchHidden = YES;
        f.ShowBookmark(BookmarkAndCategory(m_categoryIndex, indexPath.row));
        [self.navigationController popToRootViewControllerAnimated:YES];
      }
    }
  }
  else if (indexPath.section == m_shareSection)
  {
    BookmarkCategory const * cat = GetFramework().GetBmCategory(m_categoryIndex);
    if (cat)
    {
      [[Statistics instance] logEvent:kStatEventName(kStatBookmarks, kStatExport)];
      NSMutableString * catName = [NSMutableString stringWithUTF8String:cat->GetName().c_str()];
      if (![catName length])
        [catName setString:@"MapsMe"];

      NSString * filePath = @(cat->GetFileName().c_str());
      NSMutableString * kmzFile = [NSMutableString stringWithString:filePath];
      [kmzFile replaceCharactersInRange:NSMakeRange([filePath length] - 1, 1) withString:@"z"];

      if (CreateZipFromPathDeflatedAndDefaultCompression([filePath UTF8String], [kmzFile UTF8String]))
        [self sendBookmarksWithExtension:@".kmz" andType:@"application/vnd.google-earth.kmz" andFile:kmzFile andCategory:catName];
      else
        [self sendBookmarksWithExtension:@".kml" andType:@"application/vnd.google-earth.kml+xml" andFile:filePath andCategory:catName];

      (void)my::DeleteFileX([kmzFile UTF8String]);
    }
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [[Statistics instance] logEvent:kStatEventName(kStatBookmarks, kStatExport)
                   withParameters:@{kStatValue : kStatKML}];
  [self dismissViewControllerAnimated:YES completion:nil];
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
    BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
    if (cat)
    {
      if (editingStyle == UITableViewCellEditingStyleDelete)
      {
        if (indexPath.section == m_trackSection)
        {
          cat->DeleteTrack(indexPath.row);
        }
        else
        {
          BookmarkAndCategory bookmarkAndCategory = BookmarkAndCategory(m_categoryIndex, indexPath.row);
          NSValue * value = [NSValue valueWithBytes:&bookmarkAndCategory objCType:@encode(BookmarkAndCategory)];
          [[NSNotificationCenter defaultCenter] postNotificationName:BOOKMARK_DELETED_NOTIFICATION object:value];
          BookmarkCategory::Guard guard(*cat);
          guard.m_controller.DeleteUserMark(indexPath.row);
          [NSNotificationCenter.defaultCenter postNotificationName:kBookmarksChangedNotification
                                                            object:nil
                                                          userInfo:nil];
        }
      }
      cat->SaveToKMLFile();
      size_t previousNumberOfSections  = m_numberOfSections;
      [self calculateSections];
      //We can delete the row with animation, if number of sections stay the same.
      if (previousNumberOfSections == m_numberOfSections)
        [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
      else
        [self.tableView reloadData];
      if (cat->GetUserMarkCount() + cat->GetTracksCount() == 0)
      {
        self.navigationItem.rightBarButtonItem = nil;
        [self setEditing:NO animated:YES];
      }
    }
  }
}

//******************************************************************
//*********** Location manager callbacks ***************************
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
      if (indexPath.section == m_bookmarkSection)
      {
        Bookmark const * bm = static_cast<Bookmark const *>(cat->GetUserMark(indexPath.row));
        if (bm)
        {
          m2::PointD const center = bm->GetPivot();
          double const metres = ms::DistanceOnEarth(info.m_latitude, info.m_longitude,
              MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
          cell.bmDistance.text = [LocationManager formattedDistance:metres];
        }
      }
    }
  }
}

//*********** End of Location manager callbacks ********************
//******************************************************************

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.tableView.backgroundView = nil;
  self.tableView.backgroundColor = [UIColor applicationBackgroundColor];
}

- (void)viewWillAppear:(BOOL)animated
{
  [m_locationManager start:self];

  // Display Edit button only if table is not empty
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  if (cat && (cat->GetUserMarkCount() + cat->GetTracksCount()))
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

- (void)sendBookmarksWithExtension:(NSString *)fileExtension andType:(NSString *)mimeType andFile:(NSString *)filePath andCategory:(NSString *)catName
{
  MFMailComposeViewController * mailVC = [[MFMailComposeViewController alloc] init];
  mailVC.mailComposeDelegate = self;
  [mailVC setSubject:L(@"share_bookmarks_email_subject")];
  NSData * myData = [[NSData alloc] initWithContentsOfFile:filePath];
  [mailVC addAttachmentData:myData mimeType:mimeType fileName:[NSString stringWithFormat:@"%@%@", catName, fileExtension]];
  [mailVC setMessageBody:[NSString stringWithFormat:L(@"share_bookmarks_email_body"), catName] isHTML:NO];
  [self presentViewController:mailVC animated:YES completion:nil];
}

- (void)calculateSections
{
  int index = 1;
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  if (cat->GetTracksCount())
    m_trackSection = index++;
  else
    m_trackSection = EMPTY_SECTION;
  if (cat->GetUserMarkCount())
    m_bookmarkSection = index++;
  else
    m_bookmarkSection = EMPTY_SECTION;
  if ([MFMailComposeViewController canSendMail])
    m_shareSection = index++;
  else
    m_shareSection = EMPTY_SECTION;
  m_numberOfSections = index;
}

@end
