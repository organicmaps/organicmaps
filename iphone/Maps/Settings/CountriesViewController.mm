#import "CountriesViewController.h"
#import "SettingsManager.h"

#include <sys/socket.h>
#include <netinet/in.h>
#import <SystemConfiguration/SCNetworkReachability.h>

#define NAVIGATION_BAR_HEIGHT	44
#define MAX_3G_MEGABYTES 100

#define GB 1000*1000*1000
#define MB 1000*1000    

using namespace storage;

TIndex CalculateIndex(TIndex const & parentIndex, NSIndexPath * indexPath)
{
  TIndex index = parentIndex;
  if (index.m_group == -1)
  	index.m_group = indexPath.row;
  else if (index.m_country == -1)
  	index.m_country = indexPath.row;
  else
  	index.m_region = indexPath.row;
  return index;
}

@implementation CountriesViewController

- (void) OnCloseButton: (id) sender
{
	[SettingsManager Hide];
}

- (id) initWithStorage: (Storage &)storage andIndex: (TIndex const &) index andHeader: (NSString *)header
{
	m_storage = &storage;
  m_index = index;
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
  	UIBarButtonItem * button = [[UIBarButtonItem alloc] initWithTitle:@"Close" style: UIBarButtonItemStyleDone
				target:self action:@selector(OnCloseButton:)];
  	self.navigationItem.rightBarButtonItem = button;
    self.navigationItem.title = header;
  }
	return self;
}

- (void) loadView
{
	CGRect appRect = [UIScreen mainScreen].applicationFrame;
  UITableView * countriesTableView = [[UITableView alloc] initWithFrame:appRect style:UITableViewStylePlain];
  countriesTableView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  countriesTableView.delegate = self;
	countriesTableView.dataSource = self;
  self.view = countriesTableView;
  [countriesTableView release];  
}

// Override to allow orientations other than the default portrait orientation.
- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES;
}

//- (NSInteger) numberOfSectionsInTableView: (UITableView *)tableView
//{
//	return 0;
//}
//
//- (NSString *) tableView: (UITableView *)tableView titleForHeaderInSection: (NSInteger)section
//{	
//	return nil;
//}

- (NSInteger) tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section
{
	return m_storage->CountriesCount(m_index);
}

- (void) UpdateCell: (UITableViewCell *) cell forCountry: (TIndex const &) countryIndex
{
  UIActivityIndicatorView * indicator = (UIActivityIndicatorView *)cell.accessoryView;

	switch (m_storage->CountryStatus(countryIndex))
  {
  case EOnDisk:
  	{
    	uint64_t size = 0;//g_pStorage->CountrySizeInBytes(countryIndex);
  		// convert size to human readable values
			char const * kBorMBorGB = "kB";
      if (size > GB)
      {
      	kBorMBorGB = "GB";
        size /= GB;
      }
      else if (size > MB)
      {
      	kBorMBorGB = "MB";
        size /= MB;
      }
      else
      {
      	kBorMBorGB = "kB";
        size = (size + 999) / 1000;  
      }
    
  		cell.textLabel.textColor = [UIColor greenColor];
    	cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloaded (%qu %s), touch to delete", size, kBorMBorGB];
      cell.accessoryView = nil;
    }
    break;
  case EDownloading:
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
  case EDownloadFailed:
  	cell.textLabel.textColor = [UIColor redColor];  
    cell.detailTextLabel.text = @"Download has failed :(";
    cell.accessoryView = nil;
    break;
  case EInQueue:
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
  case ENotDownloaded:
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
  TIndex index = CalculateIndex(m_index, indexPath);
	cell.textLabel.text = [NSString stringWithUTF8String:m_storage->CountryName(index).c_str()];
  if (m_storage->CountriesCount(index))
  	cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  else
  	cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
//  [self UpdateCell: cell forCountry: countryIndex];
  return cell;
}

// stores clicked country index when confirmation dialog is displayed
TIndex g_clickedIndex;

// User confirmation after touching country
- (void) actionSheet: (UIActionSheet *) actionSheet clickedButtonAtIndex: (NSInteger) buttonIndex
{
    if (buttonIndex == 0)
    {	// Delete country
    	switch (m_storage->CountryStatus(g_clickedIndex))
      {
      case ENotDownloaded:
      case EDownloadFailed:
      	m_storage->DownloadCountry(g_clickedIndex);
        break;
      default:
      	m_storage->DeleteCountry(g_clickedIndex);
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
  // Push the new table view on the stack
	TIndex index = CalculateIndex(m_index, indexPath);
	CountriesViewController * newController = [[CountriesViewController alloc] initWithStorage:*m_storage
  		andIndex: index andHeader: cell.textLabel.text];
	[self.navigationController pushViewController:newController animated:YES];
//	NSString * countryName = [[cell textLabel] text];
//
//	g_clickedIndex = TIndex(indexPath.section, indexPath.row);
//	switch (g_pStorage->CountryStatus(g_clickedIndex))
//  {
//  	case EOnDisk:
//    {	// display confirmation popup
//    	UIActionSheet * popupQuery = [[UIActionSheet alloc]
//      		initWithTitle: countryName
//        	delegate: self
//        	cancelButtonTitle: @"Cancel"
//        	destructiveButtonTitle: @"Delete"
//        	otherButtonTitles: nil];
//    	[popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
//    	[popupQuery release];
//    }
//  	break;
//  	case ENotDownloaded:
//  	case EDownloadFailed:
//  	{	// display confirmation popup with country size
//    	BOOL isWifiConnected = [CountriesViewController IsUsingWIFI];
//      
//    	uint64_t size = 0;//g_pStorage->CountrySizeInBytes(g_clickedIndex);
//  		// convert size to human readable values
//      NSString * strTitle = nil;
//      NSString * strDownload = nil;
//  		if (size > GB)
//  		{
//    		size /= GB;
//				if (isWifiConnected)
//        	strTitle = [NSString stringWithFormat:@"%@", countryName];
//        else
//        	strTitle = [NSString stringWithFormat:@"We strongly recommend using WIFI for downloading %@", countryName];
//        strDownload = [NSString stringWithFormat:@"Download %qu GB", size];
//    	}
//  		else if (size > MB)
//  		{
//    		size /= MB;
//				if (isWifiConnected || size < MAX_3G_MEGABYTES)
//        	strTitle = [NSString stringWithFormat:@"%@", countryName];
//        else
//        	strTitle = [NSString stringWithFormat:@"We strongly recommend using WIFI for downloading %@", countryName];
//        strDownload = [NSString stringWithFormat:@"Download %qu MB", size];
//  		}
//  		else
//  		{
//    		size = (size + 999) / 1000;
//        strTitle = [NSString stringWithFormat:@"%@", countryName];
//        strDownload = [NSString stringWithFormat:@"Download %qu kB", size];
//  		}
//
//    	UIActionSheet * popupQuery = [[UIActionSheet alloc]
//      		initWithTitle: strTitle
//        	delegate: self
//        	cancelButtonTitle: @"Cancel"
//        	destructiveButtonTitle: strDownload
//        	otherButtonTitles: nil];
//    	[popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
//    	[popupQuery release];    	
////  	g_pStorage->DownloadCountry(g_clickedIndex);
//		}
//  	break;
//  	case EDownloading:
//    { // display confirmation popup
//    	UIActionSheet * popupQuery = [[UIActionSheet alloc]
//      		initWithTitle: countryName
//        	delegate: self
//        	cancelButtonTitle: @"Do Nothing"
//        	destructiveButtonTitle: @"Cancel Download"
//        	otherButtonTitles: nil];
//    	[popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
//    	[popupQuery release];
//    }
//    break;
//  	case EInQueue:
//  	// cancel download
//    g_pStorage->DeleteCountry(g_clickedIndex);
//		break;
//  }
}

- (void) OnDownloadFinished: (TIndex const &) index
{
  UITableView * tableView = (UITableView *)[self.view.subviews objectAtIndex: 1];
//  UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: index.second inSection: index.first]];
//  if (cell)
//		[self UpdateCell: cell forCountry: index];
}

- (void) OnDownload: (TIndex const &) index withProgress: (TDownloadProgress const &) progress
{
  UITableView * tableView = (UITableView *)[self.view.subviews objectAtIndex: 1];
//  UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: index.second inSection: index.first]];
//  if (cell)
//		cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloading %qu%%, touch to cancel", progress.first * 100 / progress.second];
}

@end

