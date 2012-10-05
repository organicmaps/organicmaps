#import "BookmarksVC.h"
#import "CustomNavigationView.h"
#import "BalloonView.h"
#import "MapsAppDelegate.h"
#import "SelectSetVC.h"
#import "CompassView.h"

#include "Framework.h"
#include "../../../geometry/distance_on_sphere.hpp"
#include "../../../platform/platform.hpp"


@implementation BookmarksVC

- (id) initWithBalloonView:(BalloonView *)view andCategory:(size_t)categoryIndex
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_locationManager = [MapsAppDelegate theApp].m_locationManager;
    m_balloon = view;
    m_categoryIndex = categoryIndex;
    self.title = [NSString stringWithUTF8String:GetFramework().GetBmCategory(categoryIndex)->GetName().c_str()];
  }
  return self;
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
  switch (section)
  {
  case 0: return 2;
  case 1: return GetFramework().GetBmCategory(m_categoryIndex)->GetBookmarksCount();
  default: return 0;
  }
}

- (void)onVisibilitySwitched:(UISwitch *)sender
{
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  cat->SetVisible(sender.on);
  cat->SaveToKMLFileAtPath(GetPlatform().WritableDir());
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
  if (!cat)
    return nil;

  UITableViewCell * cell = nil;
  if (indexPath.section == 0)
  {
    if (indexPath.row == 0)
    {
      cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCSetNameCell"];
      if (!cell)
      {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCSetNameCell"] autorelease];
        cell.textLabel.text = NSLocalizedString(@"name", @"Bookmarks dialog - Bookmark set cell");
        // @TODO insert text editor
      }
      cell.detailTextLabel.text = [NSString stringWithUTF8String:cat->GetName().c_str()];
    }
    else
    {
      cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCSetVisibilityCell"];
      if (!cell)
      {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCSetVisibilityCell"] autorelease];
        cell.textLabel.text = NSLocalizedString(@"visible", @"Bookmarks dialog - Bookmark set cell");
        cell.accessoryView = [[[UISwitch alloc] init] autorelease];
      }
      UISwitch * sw = (UISwitch *)cell.accessoryView;
      sw.on = cat->IsVisible();
      [sw addTarget:self action:@selector(onVisibilitySwitched:) forControlEvents:UIControlEventValueChanged];
    }
  }
  else
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCBookmarkItemCell"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"BookmarksVCBookmarkItemCell"] autorelease];
    }

    Bookmark const * bm = cat->GetBookmark(indexPath.row);
    if (bm)
    {
      cell.textLabel.text = [NSString stringWithUTF8String:bm->GetName().c_str()];
      cell.imageView.image = [UIImage imageNamed:[NSString stringWithUTF8String:bm->GetType().c_str()]];

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
        double const metres = ms::DistanceOnEarth(lat, lon, MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
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

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];

  if (indexPath.section == 0)
  {
    if (indexPath.row == 0)
    {
      // Edit name
      // @TODO
    }
  }
  else
  {
    Framework & f = GetFramework();
    BookmarkCategory * cat = f.GetBmCategory(m_categoryIndex);
    if (cat)
    {
      Bookmark const * bm = cat->GetBookmark(indexPath.row);
      if (bm)
      {
        // Same as "Close".
        [self dismissModalViewControllerAnimated:YES];
        [self.navigationController.visibleViewController dismissModalViewControllerAnimated:YES];
        f.ShowRect(bm->GetViewport());
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
      BookmarkCategory * cat = GetFramework().GetBmCategory(m_categoryIndex);
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

  // Display Edit button only if table is not empty
  BookmarkCategory * cat = GetFramework().GetBmCategory([m_balloon.setName UTF8String]);
  if (cat && cat->GetBookmarksCount())
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
  else
    self.navigationItem.rightBarButtonItem = nil;

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
