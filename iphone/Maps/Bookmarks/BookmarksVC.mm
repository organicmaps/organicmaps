#import "BookmarksVC.h"
#import "CustomNavigationView.h"
#import "BalloonView.h"
#import "MapsAppDelegate.h"
#import "SelectSetVC.h"
#import "CompassView.h"

#include "Framework.h"
#include "../../../geometry/distance_on_sphere.hpp"


@implementation BookmarksVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_locationManager = [MapsAppDelegate theApp].m_locationManager;
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

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

// Returns bookmarks count in the active bookmark set (category)
- (size_t) getBookmarksCount
{
  BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
  if (cat)
    return cat->GetBookmarksCount();
  return 0;
}


// Used to display bookmarks hint when no any bookmarks are added
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  if (section == 1)
  {
    // Do not display any hint if bookmarks are present
    if ([self getBookmarksCount])
      return 0.;
    return tableView.bounds.size.height / 2.;
  }
  return 0.;
}

// Used to display bookmarks hint when no any bookmarks are added
- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (section == 1)
  {
    // Do not display any hint if bookmarks are present
    if ([self getBookmarksCount])
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
  return nil;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  switch (section)
  {
  case 0: return 1;
  case 1: return [self getBookmarksCount];
  default: return 0;
  }
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
    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarkItemCell"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarkItemCell"] autorelease];
    }

    BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
    if (cat)
    {
      Bookmark const * bm = cat->GetBookmark(indexPath.row);
      if (bm)
      {
        cell.textLabel.text = [NSString stringWithUTF8String:bm->GetName().c_str()];

        CompassView * compass;
        // Try to reuse existing compass view
        if ([cell.accessoryView isKindOfClass:[CompassView class]])
          compass = (CompassView *)cell.accessoryView;
        else
        {
          // Create compass view
          float const h = (int)(tableView.rowHeight * 0.6);
          compass = [[[CompassView alloc] initWithFrame:CGRectMake(0, 0, h, h)] autorelease];
          cell.accessoryView = compass;
        }

        double lat, lon, northR;
        if ([m_locationManager getLat:lat Lon:lon])
        {
          m2::PointD const center = bm->GetOrg();
          double const metres = ms::DistanceOnEarth(lat, lon,
              MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
          cell.detailTextLabel.text = [LocationManager formatDistance:metres];

          if ([m_locationManager getNorthRad:northR])
          {
            compass.angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon),
                                                    MercatorBounds::LatToY(lat)), center) + northR;
            compass.showArrow = YES;
          }
          else
            compass.showArrow = NO;
        }
        else
        {
          compass.showArrow = NO;
          cell.detailTextLabel.text = nil;
        }
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

//******************************************************************
//*********** Location manager callbacks ***************************
- (void)onLocationStatusChanged:(location::TLocationStatus)newStatus
{
  // Handle location status changes if necessary
}

- (void)onGpsUpdate:(location::GpsInfo const &)info
{
  // Refresh distance
  BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
  if (cat)
  {
    UITableView * table = (UITableView *)self.view;
    NSArray * cells = [table visibleCells];
    for (NSUInteger i = 0; i < cells.count; ++i)
    {
      UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
      NSIndexPath * indexPath = [table indexPathForCell:cell];
      if (indexPath.section == 1)
      {
        Bookmark const * bm = cat->GetBookmark(indexPath.row);
        if (bm)
        {
          m2::PointD const center = bm->GetOrg();
          double const metres = ms::DistanceOnEarth(info.m_latitude, info.m_longitude,
              MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
          cell.detailTextLabel.text = [LocationManager formatDistance:metres];
        }
      }
    }
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  double lat, lon;
  if (![m_locationManager getLat:lat Lon:lon])
    return;
  // Refresh compass arrow
  BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
  if (cat)
  {
    double const northRad = (info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading;
    UITableView * table = (UITableView *)self.view;
    NSArray * cells = [table visibleCells];
    for (NSUInteger i = 0; i < cells.count; ++i)
    {
      UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
      NSIndexPath * indexPath = [table indexPathForCell:cell];
      if (indexPath.section == 1)
      {
        Bookmark const * bm = cat->GetBookmark(indexPath.row);
        if (bm)
        {
          CompassView * compass = (CompassView *)cell.accessoryView;
          m2::PointD const center = bm->GetOrg();
          compass.angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon),
                                                    MercatorBounds::LatToY(lat)), center) + northRad;
          compass.showArrow = YES;
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
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [m_locationManager stop:self];
  [super viewWillDisappear:animated];
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [m_locationManager setOrientation:self.interfaceOrientation];
}

@end
