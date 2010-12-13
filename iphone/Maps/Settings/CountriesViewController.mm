#import "CountriesViewController.h"
#import "SettingsManager.h"

#include <sys/socket.h>
#include <netinet/in.h>
#import <SystemConfiguration/SCNetworkReachability.h>

#include "../../../map/storage.hpp"
#include <boost/bind.hpp>

#define NAVIGATION_BAR_HEIGHT	44
#define MAX_3G_MEGABYTES 100


/////////////////////////////////////////////////////////////////
// needed for trick with back button
@interface NotAnimatedNavigationBar : UINavigationBar
@end;
@implementation NotAnimatedNavigationBar
- (UINavigationItem *) popNavigationItemAnimated: (BOOL)animated
{
	return [super popNavigationItemAnimated:NO];
}
@end
/////////////////////////////////////////////////////////////////

@implementation CountriesViewController

	mapinfo::Storage * g_pStorage = 0;

- (id) initWithStorage: (mapinfo::Storage &)storage
{
	g_pStorage = &storage;
  if ((self = [super initWithNibName:@"CountriesViewController" bundle:nil]))
  {
  	// tricky boost::bind for objC class methods
  	typedef void (*TFinishFunc)(id, SEL, mapinfo::TIndex const &);
    SEL finishSel = @selector(OnDownloadFinished:);
  	TFinishFunc finishImpl = (TFinishFunc)[self methodForSelector:finishSel];
    
  	typedef void (*TProgressFunc)(id, SEL, mapinfo::TIndex const &, TDownloadProgress const &);
    SEL progressSel = @selector(OnDownload:withProgress:);
  	TProgressFunc progressImpl = (TProgressFunc)[self methodForSelector:progressSel];
    
  	storage.Subscribe(boost::bind(finishImpl, self, finishSel, _1),
    		boost::bind(progressImpl, self, progressSel, _1, _2));
  }
	return self;
}

- (void) dealloc
{
	g_pStorage->Unsubscribe();
  [super dealloc];
}

// called on Map button click
- (void) navigationBar: (UINavigationBar *)navigationBar didPopItem: (UINavigationItem *)item
{
	[SettingsManager Hide];
}

- (void) loadView
{
	CGRect appRect = [UIScreen mainScreen].applicationFrame;
	UIView * rootView = [[UIView alloc] initWithFrame:appRect];
  rootView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  
  CGRect navBarRect = CGRectMake(0, 0, appRect.size.width, NAVIGATION_BAR_HEIGHT);
  NotAnimatedNavigationBar * bar = [[NotAnimatedNavigationBar alloc] initWithFrame:navBarRect];
  bar.delegate = self;
  
  UINavigationItem * item1 = [[UINavigationItem alloc] initWithTitle:@"Map"];	// title for Back button
  [item1.backBarButtonItem setAction:@selector(OnBackClick:)];
  [bar pushNavigationItem:item1 animated:NO];
	[item1 release];
  UINavigationItem * item2 = [[UINavigationItem alloc] initWithTitle:@"Download Manager"];
  [item2 setHidesBackButton:NO animated:NO];
  [bar pushNavigationItem:item2 animated:NO];
  [item2 release];
    
  [rootView addSubview:bar];
  [bar release];
  
  CGRect tableRect = CGRectMake(0, navBarRect.size.height, navBarRect.size.width, appRect.size.height - navBarRect.size.height);
  UITableView * countriesTableView = [[UITableView alloc] initWithFrame:tableRect style:UITableViewStylePlain];
  countriesTableView.delegate = self;
  countriesTableView.dataSource = self;
  
  [rootView addSubview:countriesTableView];
  [countriesTableView release];
  
  self.view = rootView;
	[rootView release];
}

// Override to allow orientations other than the default portrait orientation.
- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES;
}

- (NSInteger) numberOfSectionsInTableView: (UITableView *)tableView
{
	return g_pStorage->GroupsCount();
}

- (NSString *) tableView: (UITableView *)tableView titleForHeaderInSection: (NSInteger)section
{	
	return [NSString stringWithUTF8String: g_pStorage->GroupName(section).c_str()];
}

- (NSInteger) tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section
{
	return g_pStorage->CountriesCountInGroup(section);
}

- (void) UpdateCell: (UITableViewCell *) cell forCountry: (mapinfo::TIndex const &) countryIndex
{
//  uint64_t size = g_pStorage->CountrySizeInBytes(countryIndex);
//  // convert size to human readable values
//  uint64_t const GB = 1000 * 1000 * 1000;
//  uint64_t const MB = 1000 * 1000;
//  char const * sizeStr = "kB";
//  if (size > GB)
//  {
//  	sizeStr = "GB";
//    size /= GB;
//  }
//  else if (size > MB)
//  {
//    sizeStr = "MB";
//    size /= MB;
//  }
//  else
//  {
//    sizeStr = "kB";
//    size = (size + 999) / 1000;
//  }

  UIActivityIndicatorView * indicator = (UIActivityIndicatorView *)cell.accessoryView;

	switch (g_pStorage->CountryStatus(countryIndex))
  {
  case mapinfo::EOnDisk:
  	{
  		cell.textLabel.textColor = [UIColor greenColor];
//    	cell.detailTextLabel.text = [NSString stringWithFormat: @"Takes %qu %s on disk", size, kBOrMB];
    	cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloaded, touch to delete"];
      cell.accessoryView = nil;
    }
    break;
  case mapinfo::EDownloading:
  	{
    	cell.textLabel.textColor = [UIColor blueColor];
      if (!indicator)
      {
      	indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];
        indicator.hidesWhenStopped = NO;
				cell.accessoryView = indicator;
        [indicator release];
      }
      [indicator startAnimating];
    }
    break;
  case mapinfo::EDownloadFailed:
  	cell.textLabel.textColor = [UIColor redColor];  
    cell.detailTextLabel.text = @"Download has failed :(";
    cell.accessoryView = nil;
    break;
  case mapinfo::EInQueue:
  	{
  		cell.textLabel.textColor = [UIColor lightGrayColor];
//    	cell.detailTextLabel.text = [NSString stringWithFormat: @"Waiting to download %qu %s", size, kBOrMB];
			cell.detailTextLabel.text = [NSString stringWithFormat: @"Marked for downloading, touch to cancel"];
    	if (!indicator)
      {
      	indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];
        indicator.hidesWhenStopped = NO;
				cell.accessoryView = indicator;
        [indicator release];
      }
      [indicator stopAnimating];
    }
    break;
  case mapinfo::ENotDownloaded:
  	cell.textLabel.textColor = [UIColor blackColor];
//    cell.detailTextLabel.text = [NSString stringWithFormat: @"Click to download %qu %s", size, kBOrMB];
    cell.detailTextLabel.text = [NSString stringWithFormat: @"Touch to download"];
    cell.accessoryView = nil;
    break;
  }
}

// Customize the appearance of table view cells.
- (UITableViewCell *) tableView: (UITableView *)tableView cellForRowAtIndexPath: (NSIndexPath *)indexPath
{ 
	static NSString * cellId = @"CountryCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier: cellId];
  if (cell == nil)
  	cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleSubtitle reuseIdentifier:cellId] autorelease];
  mapinfo::TIndex countryIndex(indexPath.section, indexPath.row);
  cell.textLabel.text = [NSString stringWithUTF8String:g_pStorage->CountryName(countryIndex).c_str()];
  cell.accessoryType = UITableViewCellAccessoryNone;
  [self UpdateCell: cell forCountry: countryIndex];
  return cell;
}

// stores clicked country index when confirmation dialog is displayed
mapinfo::TIndex g_clickedIndex;

// User confirmation after touching country
- (void) actionSheet: (UIActionSheet *) actionSheet clickedButtonAtIndex: (NSInteger) buttonIndex
{
    if (buttonIndex == 0)
    {	// Delete country
    	switch (g_pStorage->CountryStatus(g_clickedIndex))
      {
      case mapinfo::ENotDownloaded:
      case mapinfo::EDownloadFailed:
      	g_pStorage->DownloadCountry(g_clickedIndex);
        break;
      default:
      	g_pStorage->DeleteCountry(g_clickedIndex);
      }
    }
}

// return NO if not connected or using 3G
+ (BOOL) IsUsingWIFI
{
	// Create zero addy
	struct sockaddr_in zeroAddress;
	bzero(&zeroAddress, sizeof(zeroAddress));
	zeroAddress.sin_len = sizeof(zeroAddress);
	zeroAddress.sin_family = AF_INET;
 
	// Recover reachability flags
	SCNetworkReachabilityRef defaultRouteReachability = SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
	SCNetworkReachabilityFlags flags;
	BOOL didRetrieveFlags = SCNetworkReachabilityGetFlags(defaultRouteReachability, &flags);
	CFRelease(defaultRouteReachability);
	if (!didRetrieveFlags)
		return NO;
 
	BOOL isReachable = flags & kSCNetworkFlagsReachable;
  BOOL isWifi = !(flags & kSCNetworkReachabilityFlagsIsWWAN);
	BOOL needsConnection = flags & kSCNetworkFlagsConnectionRequired;
	BOOL isConnected = isReachable && !needsConnection;
  return isWifi && isConnected;
}

- (void) tableView: (UITableView *) tableView didSelectRowAtIndexPath: (NSIndexPath *) indexPath
{
	// deselect the current row (don't keep the table selection persistent)
	[tableView deselectRowAtIndexPath: indexPath animated:YES];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath: indexPath];
	NSString * countryName = [[cell textLabel] text];

	g_clickedIndex = mapinfo::TIndex(indexPath.section, indexPath.row);
	switch (g_pStorage->CountryStatus(g_clickedIndex))
  {
  	case mapinfo::EOnDisk:
    {	// display confirmation popup
    	UIActionSheet * popupQuery = [[UIActionSheet alloc]
      		initWithTitle: countryName
        	delegate: self
        	cancelButtonTitle: @"Cancel"
        	destructiveButtonTitle: @"Delete"
        	otherButtonTitles: nil];
    	[popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
    	[popupQuery release];
    }
  	break;
  	case mapinfo::ENotDownloaded:
  	case mapinfo::EDownloadFailed:
  	{	// display confirmation popup with country size
    	BOOL isWifiConnected = [CountriesViewController IsUsingWIFI];
      
    	uint64_t size = g_pStorage->CountrySizeInBytes(g_clickedIndex);
  		// convert size to human readable values
  		uint64_t const GB = 1000 * 1000 * 1000;
  		uint64_t const MB = 1000 * 1000;
      NSString * strTitle = nil;
      NSString * strDownload = nil;
  		if (size > GB)
  		{
    		size /= GB;
				if (isWifiConnected)
        	strTitle = [NSString stringWithFormat:@"%@", countryName];
        else
        	strTitle = [NSString stringWithFormat:@"We strongly recommend using WIFI for downloading %@", countryName];
        strDownload = [NSString stringWithFormat:@"Download %qu GB", size];
    	}
  		else if (size > MB)
  		{
    		size /= MB;
				if (isWifiConnected || size < MAX_3G_MEGABYTES)
        	strTitle = [NSString stringWithFormat:@"%@", countryName];
        else
        	strTitle = [NSString stringWithFormat:@"We strongly recommend using WIFI for downloading %@", countryName];
        strDownload = [NSString stringWithFormat:@"Download %qu MB", size];
  		}
  		else
  		{
    		size = (size + 999) / 1000;
        strTitle = [NSString stringWithFormat:@"%@", countryName];
        strDownload = [NSString stringWithFormat:@"Download %qu kB", size];
  		}

    	UIActionSheet * popupQuery = [[UIActionSheet alloc]
      		initWithTitle: strTitle
        	delegate: self
        	cancelButtonTitle: @"Cancel"
        	destructiveButtonTitle: strDownload
        	otherButtonTitles: nil];
    	[popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
    	[popupQuery release];    	
//  	g_pStorage->DownloadCountry(g_clickedIndex);
		}
  	break;
  	case mapinfo::EDownloading:
    { // display confirmation popup
    	UIActionSheet * popupQuery = [[UIActionSheet alloc]
      		initWithTitle: countryName
        	delegate: self
        	cancelButtonTitle: @"Do Nothing"
        	destructiveButtonTitle: @"Cancel Download"
        	otherButtonTitles: nil];
    	[popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
    	[popupQuery release];
    }
    break;
  	case mapinfo::EInQueue:
  	// cancel download
    g_pStorage->DeleteCountry(g_clickedIndex);
		break;
  }
}

- (void) OnDownloadFinished: (mapinfo::TIndex const &) index
{
  UITableView * tableView = (UITableView *)[self.view.subviews objectAtIndex: 1];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: index.second inSection: index.first]];
  if (cell)
		[self UpdateCell: cell forCountry: index];
}

- (void) OnDownload: (mapinfo::TIndex const &) index withProgress: (TDownloadProgress const &) progress
{
  UITableView * tableView = (UITableView *)[self.view.subviews objectAtIndex: 1];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: index.second inSection: index.first]];
  if (cell)
		cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloading %qu%%, touch to cancel", progress.first * 100 / progress.second];
}

@end

