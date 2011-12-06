#import "CountriesViewController.h"
#import "SettingsManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "WebViewController.h"
#import "CustomAlertView.h"
#import "DiskFreeSpace.h"

#include "GetActiveConnectionType.h"

#include "../../platform/platform.hpp"

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

- (void) onAboutButton:(id)sender
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
    [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:NSLocalizedString(@"About", @"Settings/Downloader - About window title")];
  [self.navigationController pushViewController:aboutViewController animated:YES];
  [aboutViewController release];
}

- (void) onCloseButton:(id)sender
{
  [[[MapsAppDelegate theApp] settingsManager] hide];
}

- (id) initWithStorage: (Storage &)storage andIndex: (TIndex const &) index andHeader: (NSString *)header
{
	m_storage = &storage;
  m_index = index;
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    self.navigationItem.title = header;
    // Show About button only on the first page
    if ([header compare:NSLocalizedString(@"Download", @"Settings/Downloader - Main downloader window title")] == NSOrderedSame)
    {
      UIBarButtonItem * aboutButton = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"About", @"Settings/Downloader - About the program button") style: UIBarButtonItemStylePlain
                                    target:self action:@selector(onAboutButton:)] autorelease];
      self.navigationItem.leftBarButtonItem = aboutButton;
    }
    UIBarButtonItem * closeButton = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Done", @"Settings/Downloader - Close downloader button") style: UIBarButtonItemStyleDone
                                                                     target:self action:@selector(onCloseButton:)] autorelease];
    self.navigationItem.rightBarButtonItem = closeButton;
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

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
  TIndex const index = CalculateIndex(m_index, indexPath);
  if (m_storage->CountryStatus(index) == EOnDisk)
  {
    m2::RectD const bounds = m_storage->CountryBounds(index);
    [[[MapsAppDelegate theApp] settingsManager] hide];
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
        LocalAndRemoteSizeT::first_type size = m_storage->CountrySizeInBytes(countryIndex).first;
        // convert size to human readable values
        // @TODO fix localization
        // NSLocalizedString(@"kB", @"Settings/Downloader - size string")
        // NSLocalizedString(@"MB", @"Settings/Downloader - size string")
        // NSLocalizedString(@"kB/sec", @"Settings/Downloader - speed string")
        // NSLocalizedString(@"MB/sec", @"Settings/Downloader - speed string")
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
        cell.detailTextLabel.text = [NSString stringWithFormat:NSLocalizedString(@"Downloaded (%qu %s), touch to delete", @"Settings/Downloader - info for downloaded country"),
                                                                                 size, kBorMB];
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
        cell.detailTextLabel.text = NSLocalizedString(@"Downloading...", @"Settings/Downloader - info for country which started downloading");
        UIActivityIndicatorView * indicator = [[UIActivityIndicatorView alloc] 
                                               initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];
        cell.accessoryView = indicator;
        [indicator startAnimating];
        [indicator release];
      }
      break;

    case EDownloadFailed:
      cell.textLabel.textColor = [UIColor redColor];
      cell.detailTextLabel.text = NSLocalizedString(@"Download has failed, touch again for one more try", @"Settings/Downloader - info for country when download fails");
      break;

    case EInQueue:
      {
        cell.textLabel.textColor = [UIColor colorWithRed:91.f/255.f
                                                   green:148.f/255.f
                                                    blue:222.f/255.f
                                                   alpha:1.f];
        cell.detailTextLabel.text = [NSString stringWithFormat: NSLocalizedString(@"Marked for downloading, touch to cancel", @"Settings/Downloader - info for country in the download queue")];
      }
      break;

    case ENotDownloaded:
      cell.textLabel.textColor = [UIColor blackColor];
      cell.detailTextLabel.text = [NSString stringWithFormat: NSLocalizedString(@"Touch to download", @"Settings/Downloader - info for not downloaded country")];
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
    	cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
  	else
  		cell = [[[UITableViewCell alloc] initWithStyle: UITableViewCellStyleSubtitle reuseIdentifier:cellId] autorelease];
	}

  if (hasChildren)
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  else
    cell.accessoryType = UITableViewCellAccessoryNone;

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

- (void) showDownloadCountryConfirmation:(NSString *)countryName withSize:(LocalAndRemoteSizeT::first_type)size fromRect:(CGRect)rect
{
  // display confirmation popup with country size
  // convert size to human readable values
  NSString * strDownload = nil;
  if (size > MB)
  {
    size /= MB;
    strDownload = [NSString stringWithFormat:NSLocalizedString(@"Download %qu MB", @"Settings/Downloader - Download confirmation button"), size];
  }
  else
  {
    size = (size + 999) / 1000;
    strDownload = [NSString stringWithFormat:NSLocalizedString(@"Download %qu kB", @"Settings/Downloader - Download confirmation button"), size];
  }
  
  UIActionSheet * popupQuery = [[UIActionSheet alloc]
                                initWithTitle: countryName
                                delegate: self
                                cancelButtonTitle: NSLocalizedString(@"Cancel", @"Settings/Downloader - Download confirmation Cancel button")
                                destructiveButtonTitle: nil
                                otherButtonTitles: strDownload, nil];
  [popupQuery showFromRect: rect inView: self.view animated: YES];
  [popupQuery release];
}

// 3G warning confirmation handler
- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    LocalAndRemoteSizeT const sizePair = m_storage->CountrySizeInBytes(g_clickedIndex);
    LocalAndRemoteSizeT::first_type const size = sizePair.second - sizePair.first;

    [self showDownloadCountryConfirmation:[[g_clickedCell textLabel] text] withSize:size fromRect:[g_clickedCell frame]];
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

    // pass parameters to dialog handlers
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
                                       cancelButtonTitle: NSLocalizedString(@"Cancel", @"Settings/Downloader - Delete country dialog - Cancel deletion button")
                                       destructiveButtonTitle: NSLocalizedString(@"Delete", @"Settings/Downloader - Delete country dialog - Confirm deletion button")
                                       otherButtonTitles: nil] autorelease];
        [popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
    	}
  		break;
      case ENotDownloaded:
      case EDownloadFailed:
      {
        LocalAndRemoteSizeT const sizePair = m_storage->CountrySizeInBytes(index);
        LocalAndRemoteSizeT::first_type const size = sizePair.second - sizePair.first;

        // check for disk free space first
        if (FreeDiskSpaceInBytes() < (size + 1024*1024))
        { // display warning dialog about not enough free disk space
          [[[[CustomAlertView alloc] initWithTitle:NSLocalizedString(@"There is not enough free disk space", @"Settings/Downloader - No free space dialog title")
                                                                   message:[NSString stringWithFormat:NSLocalizedString(@"Please free some space on your device first in order to download %@", @"Settings/Downloader - No free space dialog message"), countryName]
                                                                  delegate:nil
                                                         cancelButtonTitle:NSLocalizedString(@"OK", @"Settings/Downloader - No free space dialog close button")
                                                         otherButtonTitles:nil] autorelease] show];
          break;
        }

        TActiveConnectionType const connType = GetActiveConnectionType();
        if (connType == ENotConnected)
        { // do not initiate any download
          [[[[CustomAlertView alloc] initWithTitle:NSLocalizedString(@"No Internet connection detected", @"Settings/Downloader - No internet connection dialog title")
                                           message:NSLocalizedString(@"We recommend using WiFi to download large maps", @"Settings/Downloader - No internet connection dialog message")
                                          delegate:nil
                                 cancelButtonTitle:NSLocalizedString(@"OK", @"Settings/Downloader - No internet connection dialog close button")
                                 otherButtonTitles:nil] autorelease] show];
        }
        else
        {
          if (connType == EConnectedBy3G && size > MAX_3G_MEGABYTES * MB)
          { // If user uses 3G, show warning to him before downloading country
            [[[[CustomAlertView alloc] initWithTitle:[NSString stringWithFormat:NSLocalizedString(@"No WiFi connection detected. Would you like to use cellular data (GPRS, EDGE or 3G) to download %@?", @"Settings/Downloader - 3G download warning dialog title"), countryName]
                                            message:nil
                                           delegate:self
                                  cancelButtonTitle:NSLocalizedString(@"Cancel", @"Settings/Downloader - 3G download warning dialog cancel button")
                                  otherButtonTitles:NSLocalizedString(@"Use cellular data", @"Settings/Downloader - 3G download warning dialog confirm button"), nil] autorelease] show];
          }
          else
            [self showDownloadCountryConfirmation:countryName withSize:size fromRect:[cell frame]];
        }
			}
  		break;
  		case EDownloading:
    	{ // display confirmation popup
    		UIActionSheet * popupQuery = [[UIActionSheet alloc]
      			initWithTitle: countryName
        		delegate: self
			cancelButtonTitle: NSLocalizedString(@"Do nothing", @"Settings/Downloader - Cancel active download dialog - Do not cancel button")
			destructiveButtonTitle: NSLocalizedString(@"Cancel download", @"Settings/Downloader - Cancel active download dialog - Interrupt country download button")
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

- (void) OnDownload: (TIndex const &) index withProgress: (pair<int64_t, int64_t> const &) progress
{
  if (IsOurIndex(index, m_index))
  {
    UITableView * tableView = (UITableView *)self.view;
    UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: RowFromIndex(index) inSection: 0]];
    if (cell)
      cell.detailTextLabel.text = [NSString stringWithFormat: NSLocalizedString(@"Downloading %qu%%, touch to cancel", @"Settings/Downloader - country info current download progress"),
                                   progress.first * 100 / progress.second];
  }
}

@end

