#import "CountriesViewController.h"
#import "SettingsManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "WebViewController.h"
#import "CustomAlertView.h"

#include "GetActiveConnectionType.h"
#include "IPhonePlatform.hpp"

#define NAVIGATION_BAR_HEIGHT	44
#define MAX_3G_MEGABYTES 20

#define GB 1000*1000*1000
#define MB 1000*1000

using namespace storage;

static TIndex CalculateIndex(TIndex const & parentIndex, NSIndexPath * indexPath)
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

static NSInteger RowFromIndex(TIndex const & index)
{
	if (index.m_region != -1)
  	return index.m_region;
  else if (index.m_country != -1)
  	return index.m_country;
  else
  	return index.m_group;
}

static bool IsOurIndex(TIndex const & theirs, TIndex const & ours)
{
	TIndex theirsFixed = theirs;
  if (theirsFixed.m_region != -1)
  	theirsFixed.m_region = -1;
  else if (theirsFixed.m_country != -1)
  	theirsFixed.m_country = -1;
  else
		theirsFixed.m_group = -1;

  return ours == theirsFixed;
}

@implementation CountriesViewController

- (void) OnCloseButton:(id)sender
{
  [[[MapsAppDelegate theApp] settingsManager] Hide];
}

- (void) OnAboutButton:(id)sender
{
  // display WebView with About text

  NSString * filePath = [NSString stringWithUTF8String:GetPlatform().ReadPathForFile("about.html").c_str()];
  NSURL * url = [NSURL fileURLWithPath:filePath];

  WebViewController * aboutViewController = 
      [[WebViewController alloc] initWithUrl:url andTitleOrNil:@"About"];
  [self.navigationController pushViewController:aboutViewController animated:YES];
  [aboutViewController release];
}

- (id) initWithStorage: (Storage &)storage andIndex: (TIndex const &) index andHeader: (NSString *)header
{
	m_storage = &storage;
  m_index = index;
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
  	UIBarButtonItem * closeButton = [[UIBarButtonItem alloc] initWithTitle:@"Close" style: UIBarButtonItemStyleDone
				                            target:self action:@selector(OnCloseButton:)];
  	self.navigationItem.rightBarButtonItem = closeButton;
    [closeButton release];
    
    self.navigationItem.title = header;

    // About button is displayed only on first view in hierarchy
    if (index.m_group == TIndex::INVALID)
    {
      UIBarButtonItem * aboutButton = [[UIBarButtonItem alloc] initWithTitle:@"About" style: UIBarButtonItemStylePlain
                               target:self action:@selector(OnAboutButton:)];
      self.navigationItem.leftBarButtonItem = aboutButton;
      [aboutButton release];
    }
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

	// do not show status for parent categories
  if (cell.reuseIdentifier != @"ParentCell")
  {
    switch (m_storage->CountryStatus(countryIndex))
    {
    case EOnDisk:
      {
        TLocalAndRemoteSize::first_type size = m_storage->CountrySizeInBytes(countryIndex).first;
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
        cell.detailTextLabel.text = @"Downloading...";
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
      cell.detailTextLabel.text = @"Download has failed, touch again for one more try";
      cell.accessoryView = nil;
      break;
    case EInQueue:
      {
        cell.textLabel.textColor = [UIColor lightGrayColor];
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
      cell.detailTextLabel.text = [NSString stringWithFormat: @"Touch to download"];
      cell.accessoryView = nil;
      break;
    default:
      break;
    }
  }
}

// Customize the appearance of table view cells.
- (UITableViewCell *) tableView: (UITableView *)tableView cellForRowAtIndexPath: (NSIndexPath *)indexPath
{
  TIndex index = CalculateIndex(m_index, indexPath);
	bool hasChildren = m_storage->CountriesCount(index) != 0;

	NSString * cellId = hasChildren ? @"ParentCell" : @"DetailCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier: cellId];
  if (cell == nil)
  {
  	if (hasChildren)
    	cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
  	else
  		cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleSubtitle reuseIdentifier:cellId] autorelease];
	}
  cell.textLabel.text = [NSString stringWithUTF8String:m_storage->CountryName(index).c_str()];
  if (hasChildren)
  	cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  else
  	cell.accessoryType = UITableViewCellAccessoryNone;
  [self UpdateCell: cell forCountry: index];
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

- (void) tableView: (UITableView *) tableView didSelectRowAtIndexPath: (NSIndexPath *) indexPath
{
	// deselect the current row (don't keep the table selection persistent)
	[tableView deselectRowAtIndexPath: indexPath animated:YES];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath: indexPath];
  // Push the new table view on the stack
	TIndex index = CalculateIndex(m_index, indexPath);
  if (m_storage->CountriesCount(index))
  {
		CountriesViewController * newController = [[CountriesViewController alloc] initWithStorage:*m_storage
  			andIndex: index andHeader: cell.textLabel.text];
		[self.navigationController pushViewController:newController animated:YES];
  }
  else
  {
		NSString * countryName = [[cell textLabel] text];

		g_clickedIndex = index;
		switch (m_storage->CountryStatus(g_clickedIndex))
  	{
  		case EOnDisk:
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
  		case ENotDownloaded:
  		case EDownloadFailed:
  		{
        TActiveConnectionType connType = GetActiveConnectionType();
        if (connType == ENotConnected)
        { // do not initiate any download
          CustomAlertView * alert = [[CustomAlertView alloc] initWithTitle:@"No Internet connection detected"
              message:@"We recommend to download large countries by using WiFi"
              delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
          [alert show];
          [alert release];
        }
        else
        {
          TLocalAndRemoteSize sizePair = m_storage->CountrySizeInBytes(g_clickedIndex);
          TLocalAndRemoteSize::first_type size = sizePair.second - sizePair.first;
          if (connType == EConnectedBy3G && size > MAX_3G_MEGABYTES * MB)
          { // If user uses 3G, do not allow him to download large countries
            CustomAlertView * alert = [[CustomAlertView alloc] initWithTitle:[NSString stringWithFormat:@"%@ is too large for 3G download", countryName]
                                                                     message:@"Please, use WiFi to download large countries"
                                                                    delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
            [alert show];
            [alert release];            
          }
          else
          {
            // display confirmation popup with country size
            // convert size to human readable values
            NSString * strTitle = nil;
            NSString * strDownload = nil;
            if (size > GB)
            {
              size /= GB;
              strTitle = [NSString stringWithFormat:@"%@", countryName];
              strDownload = [NSString stringWithFormat:@"Download %qu GB", size];
            }
            else if (size > MB)
            {
              size /= MB;
              strTitle = [NSString stringWithFormat:@"%@", countryName];
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
                                          destructiveButtonTitle: nil
                                          otherButtonTitles: strDownload, nil];
            [popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
            [popupQuery release];
          }
        }
			}
  		break;
  		case EDownloading:
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
  		case EInQueue:
  		// cancel download
    	m_storage->DeleteCountry(g_clickedIndex);
			break;
      default:
      break;
  	}
  }
}

- (void) OnCountryChange: (TIndex const &) index
{
  if (IsOurIndex(index, m_index))
  {
  	UITableView * tableView = (UITableView *)self.view;
  	UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: RowFromIndex(index) inSection: 0]];
  	if (cell)
			[self UpdateCell: cell forCountry: index];
	}
}

- (void) OnDownload: (TIndex const &) index withProgress: (TDownloadProgress const &) progress
{
	if (IsOurIndex(index, m_index))
  {
  	UITableView * tableView = (UITableView *)self.view;
  	UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: RowFromIndex(index) inSection: 0]];
  	if (cell)
			cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloading %qu%%, touch to cancel", progress.first * 100 / progress.second];
  }
}

@end

