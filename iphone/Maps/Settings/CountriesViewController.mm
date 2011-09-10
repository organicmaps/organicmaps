#import "CountriesViewController.h"
#import "SettingsManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "WebViewController.h"
#import "CustomAlertView.h"
#import "DiskFreeSpace.h"

#include "GetActiveConnectionType.h"
#include "IPhonePlatform.hpp"

#define MAX_3G_MEGABYTES 20

#define MB 1000*1000

using namespace storage;

static TIndex CalculateIndex(TIndex const & parentIndex, NSIndexPath * indexPath)
{
  TIndex index = parentIndex;
  if (index.m_group == TIndex::INVALID)
  	index.m_group = indexPath.row;
  else if (index.m_country == TIndex::INVALID)
  	index.m_country = indexPath.row;
  else
  	index.m_region = indexPath.row;
  return index;
}

static NSInteger RowFromIndex(TIndex const & index)
{
	if (index.m_region != TIndex::INVALID)
  	return index.m_region;
  else if (index.m_country != TIndex::INVALID)
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

- (void) OnAboutButton:(id)sender
{
  // display WebView with About text
  NSString * text;
  {
    ReaderPtr<Reader> r = GetPlatform().GetReader("about-travelguide-iphone.html");
    string s;
    r.ReadAsString(s);
    text = [NSString stringWithUTF8String:s.c_str()];
  }

  WebViewController * aboutViewController =
    [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:@"About"];
  [self.navigationController pushViewController:aboutViewController animated:YES];
  [aboutViewController release];
}

- (id) initWithStorage: (Storage &)storage andIndex: (TIndex const &) index andHeader: (NSString *)header
{
	m_storage = &storage;
  m_index = index;
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    self.navigationItem.title = header;

    UIBarButtonItem * aboutButton = [[[UIBarButtonItem alloc] initWithTitle:@"About" style: UIBarButtonItemStylePlain
                                    target:self action:@selector(OnAboutButton:)] autorelease];
    self.navigationItem.rightBarButtonItem = aboutButton;
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

// correctly pass rotation event up to the root mapViewController
// to fix rotation bug when other controller is above the root
- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [[self.navigationController.viewControllers objectAtIndex:0] didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
  TIndex const index = CalculateIndex(m_index, indexPath);
  if (m_storage->CountryStatus(index) == EOnDisk)
  {
    m2::RectD const bounds = m_storage->CountryBounds(index);
    [[[MapsAppDelegate theApp] settingsManager] Hide];
    [[MapsAppDelegate theApp].m_mapViewController ZoomToRect:bounds];
  }
}

- (NSInteger) tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section
{
	return m_storage->CountriesCount(m_index);
}

- (void) UpdateCell: (UITableViewCell *) cell forCountry: (TIndex const &) countryIndex
{
  cell.accessoryView = nil;

  string const & flag = m_storage->CountryFlag(countryIndex);
  if (!flag.empty())
    cell.imageView.image = [UIImage imageNamed:[NSString stringWithFormat:@"%s.png", flag.c_str()]];

  // do not show status for parent categories
  if (cell.reuseIdentifier != @"ParentCell")
  {
    switch (m_storage->CountryStatus(countryIndex))
    {
    case EOnDisk:
      {
        TLocalAndRemoteSize::first_type size = m_storage->CountrySizeInBytes(countryIndex).first;
        // convert size to human readable values
        char const * kBorMB = "kB";
        if (size > MB)
        {
          kBorMB = "MB";
          size /= MB;
        }
        else
        {
          kBorMB = "kB";
          size = (size + 999) / 1000;
        }

        cell.textLabel.textColor = [UIColor colorWithRed:0.f/255.f
                                                   green:161.f/255.f
                                                    blue:68.f/255.f
                                                   alpha:1.f];
        cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloaded (%qu %s), touch to delete", size, kBorMB];
        // also add "sight" icon for centering on the country
        cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
      }
      break;

    case EDownloading:
      {
        cell.textLabel.textColor = [UIColor colorWithRed:52.f/255.f
                                                   green:43.f/255.f
                                                    blue:182.f/255.f
                                                   alpha:1.f];
        cell.detailTextLabel.text = @"Downloading...";
        UIActivityIndicatorView * indicator = [[UIActivityIndicatorView alloc] 
                                               initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];
        cell.accessoryView = indicator;
        [indicator startAnimating];
        [indicator release];
      }
      break;

    case EDownloadFailed:
      cell.textLabel.textColor = [UIColor redColor];
      cell.detailTextLabel.text = @"Download has failed, touch again for one more try";
      break;

    case EInQueue:
      {
        cell.textLabel.textColor = [UIColor colorWithRed:91.f/255.f
                                                   green:148.f/255.f
                                                    blue:222.f/255.f
                                                   alpha:1.f];
        cell.detailTextLabel.text = [NSString stringWithFormat: @"Marked for downloading, touch to cancel"];
      }
      break;

    case ENotDownloaded:
      cell.textLabel.textColor = [UIColor blackColor];
      cell.detailTextLabel.text = [NSString stringWithFormat: @"Touch to download"];
      break;

    case EUnknown:
      break;
    }
  }
}

// Customize the appearance of table view cells.
- (UITableViewCell *) tableView: (UITableView *)tableView cellForRowAtIndexPath: (NSIndexPath *)indexPath
{
  TIndex index = CalculateIndex(m_index, indexPath);
	bool const hasChildren = m_storage->CountriesCount(index) != 0;

	NSString * cellId = hasChildren ? @"ParentCell" : @"DetailCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier: cellId];
  if (cell == nil)
  {
  	if (hasChildren)
    {
    	cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
  	else
    {
  		cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleSubtitle reuseIdentifier:cellId] autorelease];
      cell.accessoryType = UITableViewCellAccessoryNone;
    }
	}
  cell.textLabel.text = [NSString stringWithUTF8String:m_storage->CountryName(index).c_str()];

  [self UpdateCell: cell forCountry: index];
  return cell;
}

// stores clicked country index when confirmation dialog is displayed
TIndex g_clickedIndex;
UITableViewCell * g_clickedCell = nil;

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
      // remove "zoom to country" icon
      g_clickedCell.accessoryType = UITableViewCellAccessoryNone;
    }
  }
}

- (void) tableView: (UITableView *) tableView didSelectRowAtIndexPath: (NSIndexPath *) indexPath
{
	// deselect the current row (don't keep the table selection persistent)
	[tableView deselectRowAtIndexPath: indexPath animated:YES];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath: indexPath];
  // Push the new table view on the stack
	TIndex const index = CalculateIndex(m_index, indexPath);
  if (m_storage->CountriesCount(index))
  {
		CountriesViewController * newController = [[CountriesViewController alloc] initWithStorage:*m_storage
  			andIndex: index andHeader: cell.textLabel.text];
		[self.navigationController pushViewController:newController animated:YES];
  }
  else
  {
		NSString * countryName = [[cell textLabel] text];

    // pass parameters to dialog handler
    g_clickedIndex = index;
    g_clickedCell = cell;

		switch (m_storage->CountryStatus(index))
  	{
  		case EOnDisk:
    	{
        // display confirmation popup
    		UIActionSheet * popupQuery = [[[UIActionSheet alloc]
      			initWithTitle: countryName
        		delegate: self
        		cancelButtonTitle: @"Cancel"
        		destructiveButtonTitle: @"Delete"
        		otherButtonTitles: nil] autorelease];
        [popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
    	}
  		break;
      case ENotDownloaded:
      case EDownloadFailed:
      {
        TLocalAndRemoteSize const sizePair = m_storage->CountrySizeInBytes(index);
        TLocalAndRemoteSize::first_type size = sizePair.second - sizePair.first;

        // check for disk free space first
        if (FreeDiskSpaceInBytes() < (size + 1024*1024))
        { // display warning dialog about not enough free disk space
          [[[[CustomAlertView alloc] initWithTitle:@"There is not enough free disk space"
                                                                   message:[NSString stringWithFormat:@"Please, free some space on your device first to download %@", countryName] 
                                                                  delegate:nil
                                                         cancelButtonTitle:@"OK"
                                                         otherButtonTitles:nil] autorelease] show];
          break;
        }

        TActiveConnectionType const connType = GetActiveConnectionType();
        if (connType == ENotConnected)
        { // do not initiate any download
          [[[[CustomAlertView alloc] initWithTitle:@"No Internet connection detected"
                                           message:@"Please, use WiFi to download large countries"
                                          delegate:nil
                                 cancelButtonTitle:@"OK"
                                 otherButtonTitles:nil] autorelease] show];
        }
        else
        {
          if (connType == EConnectedBy3G && size > MAX_3G_MEGABYTES * MB)
          { // If user uses 3G, do not allow him to download large countries
            [[[[CustomAlertView alloc] initWithTitle:[NSString stringWithFormat:@"%@ is too large to download over 3G", countryName]
                                            message:@"Please, use WiFi connection to download large countries"
                                          delegate:nil
                                  cancelButtonTitle:@"OK"
                                  otherButtonTitles:nil] autorelease] show];
          }
          else
          {
            // display confirmation popup with country size
            // convert size to human readable values
            NSString * strDownload = nil;
            if (size > MB)
            {
              size /= MB;
              strDownload = [NSString stringWithFormat:@"Download %qu MB", size];
            }
            else
            {
              size = (size + 999) / 1000;
              strDownload = [NSString stringWithFormat:@"Download %qu kB", size];
            }

            UIActionSheet * popupQuery = [[UIActionSheet alloc]
                                          initWithTitle: countryName
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
    	m_storage->DeleteCountry(index);
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

- (void) OnDownload: (TIndex const &) index withProgress: (HttpProgressT const &) progress
{
  if (IsOurIndex(index, m_index))
  {
    UITableView * tableView = (UITableView *)self.view;
    UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: RowFromIndex(index) inSection: 0]];
    if (cell)
      cell.detailTextLabel.text = [NSString stringWithFormat: @"Downloading %qu%%, touch to cancel",
                                   progress.m_current * 100 / progress.m_total];
  }
}

@end

