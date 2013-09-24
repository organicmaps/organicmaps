#import "CountriesViewController.h"
#import "SettingsManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "WebViewController.h"
#import "CustomAlertView.h"
#import "DiskFreeSpace.h"

#include "Framework.h"
#include "GetActiveConnectionType.h"

#include "../../platform/platform.hpp"


#define MAX_3G_MEGABYTES (50)
#define MB (1024*1024)


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
    ReaderPtr<Reader> r = GetPlatform().GetReader("about.html");
    string s;
    r.ReadAsString(s);
    NSString * str = [NSString stringWithFormat:@"Version: %@ \n", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]];
    text = [NSString stringWithFormat:@"%@%@", str, [NSString stringWithUTF8String:s.c_str()] ];
  }

  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:NSLocalizedString(@"about", nil)];
  [self.navigationController pushViewController:aboutViewController animated:YES];
  [aboutViewController release];
}

- (void) onCloseButton:(id)sender
{
  [[[MapsAppDelegate theApp] settingsManager] hide];
}

- (id) initWithIndex: (TIndex const &) index andHeader: (NSString *)header
{
  m_index = index;
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    self.navigationItem.title = header;
    // Show Close button only on the first page
    if ([header compare:NSLocalizedString(@"download_maps", nil)] == NSOrderedSame)
    {
      UIBarButtonItem * closeButton = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"maps", nil) style: UIBarButtonItemStyleDone
                                      target:self action:@selector(onCloseButton:)] autorelease];
      self.navigationItem.leftBarButtonItem = closeButton;
    }
    UIBarButtonItem * aboutButton = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"about", nil) style: UIBarButtonItemStylePlain target:self action:@selector(onAboutButton:)] autorelease];
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

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
  TIndex const index = CalculateIndex(m_index, indexPath);  
  Framework & frm = GetFramework();

  storage::TStatus const status = frm.GetCountryStatus(index);
  if (status == EOnDisk || status == EOnDiskOutOfDate)
  {
    frm.ShowCountry(index);

    [[[MapsAppDelegate theApp] settingsManager] hide];
  }
}

- (NSInteger) tableView: (UITableView *)tableView numberOfRowsInSection: (NSInteger)section
{
	return GetFramework().Storage().CountriesCount(m_index);
}

- (NSString *) GetStringForSize: (size_t)size
{
  if (size > MB)
  {
    // do the correct rounding of Mb
    return [NSString stringWithFormat:@"%ld %@", (size + 512 * 1024) / MB, NSLocalizedString(@"mb", nil)];
  }
  else
  {
    // get upper bound size for Kb
    return [NSString stringWithFormat:@"%ld %@", (size + 1023) / 1024, NSLocalizedString(@"kb", nil)];
  }
}

// @TODO Refactor UI to use good icon for "zoom to the country" action
- (UITableViewCellAccessoryType) getZoomIconType
{
  static const UITableViewCellAccessoryType iconType =
      [UIDevice currentDevice].systemVersion.floatValue < 7.0 ? UITableViewCellAccessoryDetailDisclosureButton
      : UITableViewCellAccessoryDetailButton;
  return iconType;
}

- (void) UpdateCell: (UITableViewCell *)cell forCountry: (TIndex const &)countryIndex
{
  cell.accessoryView = nil;

  Framework & frm = GetFramework();
  Storage & s = frm.Storage();
  
  string const & flag = s.CountryFlag(countryIndex);
  guides::GuideInfo info;
  if ((s.CountriesCount(countryIndex) == 0) && frm.GetGuideInfo(countryIndex, info))
  {
    UIImageView * iv = cell.imageView;
    if (iv.gestureRecognizers.count == 0)
    {
      UITapGestureRecognizer * gr = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(showGuideAdvertise:)] autorelease];
      [iv addGestureRecognizer:gr];
    }
    iv.tag = RowFromIndex(countryIndex);
    iv.image = [UIImage imageNamed:@"guide_bag"];
    iv.userInteractionEnabled = YES;
  }
  else
  {
    cell.imageView.userInteractionEnabled = NO;
    if (!flag.empty())
      cell.imageView.image = [UIImage imageNamed:[NSString stringWithFormat:@"%s.png", flag.c_str()]];
  }

  // do not show status for parent categories
  if (![cell.reuseIdentifier isEqual: @"ParentCell"])
  {
    storage::TStatus const st = frm.GetCountryStatus(countryIndex);
    switch (st)
    {
    case EOnDisk:
      cell.textLabel.textColor = [UIColor colorWithRed:0.f/255.f
                                                  green:161.f/255.f
                                                  blue:68.f/255.f
                                                  alpha:1.f];
      cell.detailTextLabel.text = [NSString stringWithFormat:NSLocalizedString(@"downloaded_touch_to_delete", nil), [self GetStringForSize: s.CountrySizeInBytes(countryIndex).first]];

      // also add "sight" icon for centering on the country
      cell.accessoryType = [self getZoomIconType];
      break;

    case EOnDiskOutOfDate:
        cell.textLabel.textColor = [UIColor colorWithRed:1.f
                                                    green:105.f/255.f
                                                    blue:180.f/255.f
                                                    alpha:1.f];
        cell.detailTextLabel.text = [NSString stringWithFormat:NSLocalizedString(@"downloaded_touch_to_update", nil), [self GetStringForSize: s.CountrySizeInBytes(countryIndex).first]];

        // also add "sight" icon for centering on the country
        cell.accessoryType = [self getZoomIconType];
        break;

    case EDownloading:
      {
        cell.textLabel.textColor = [UIColor colorWithRed:52.f/255.f
                                                    green:43.f/255.f
                                                    blue:182.f/255.f
                                                    alpha:1.f];
        cell.detailTextLabel.text = NSLocalizedString(@"downloading", nil);
        UIActivityIndicatorView * indicator = [[UIActivityIndicatorView alloc] 
                                               initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray];
        cell.accessoryView = indicator;
        [indicator startAnimating];
        [indicator release];
        break;
      }

    case EDownloadFailed:
      cell.textLabel.textColor = [UIColor redColor];
      cell.detailTextLabel.text = NSLocalizedString(@"download_has_failed", nil);
      break;

    case EInQueue:
      {
        cell.textLabel.textColor = [UIColor colorWithRed:91.f/255.f
                                                    green:148.f/255.f
                                                    blue:222.f/255.f
                                                    alpha:1.f];
        cell.detailTextLabel.text = NSLocalizedString(@"marked_for_downloading", nil);
      }
      break;

    case ENotDownloaded:
      cell.textLabel.textColor = [UIColor blackColor];
      cell.detailTextLabel.text = NSLocalizedString(@"touch_to_download", nil);
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
  storage::Storage & s = GetFramework().Storage();
	bool const hasChildren = s.CountriesCount(index) != 0;

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

  cell.textLabel.text = [NSString stringWithUTF8String:s.CountryName(index).c_str()];

  [self UpdateCell: cell forCountry: index];
  return cell;
}

// User confirmation after touching country
- (void) actionSheet: (UIActionSheet *) actionSheet clickedButtonAtIndex: (NSInteger) buttonIndex
{
  if (buttonIndex != actionSheet.cancelButtonIndex)
  {
    Framework & frm = GetFramework();
    switch (m_countryStatus)
    {
    case EOnDiskOutOfDate:
      if (buttonIndex != 0)
      {
        [self TryDownloadCountry];
        return;
      }
      break;

    case ENotDownloaded:
    case EDownloadFailed:
      frm.Storage().DownloadCountry(m_clickedIndex);
      return;

    case EOnDisk:
    case EDownloading:
    case EInQueue:
    case EUnknown:
        break;
    }

    frm.DeleteCountry(m_clickedIndex);
    // remove "zoom to country" icon
    m_clickedCell.accessoryType = UITableViewCellAccessoryNone;
  }
}

- (void) DoDownloadCountry
{
  if (m_countryStatus == EOnDiskOutOfDate)
  {
    GetFramework().Storage().DownloadCountry(m_clickedIndex);
  }
  else
  {
    NSString * countryName = [[m_clickedCell textLabel] text];
    NSString * strDownload = [NSString stringWithFormat:NSLocalizedString(@"download_mb_or_kb", nil), [self GetStringForSize: m_downloadSize]];

    UIActionSheet * popupQuery = [[UIActionSheet alloc]
                                  initWithTitle: countryName
                                  delegate: self
                                  cancelButtonTitle: NSLocalizedString(@"cancel", nil)
                                  destructiveButtonTitle: nil
                                  otherButtonTitles: strDownload, nil];

    [popupQuery showFromRect: [m_clickedCell frame] inView: self.view animated: YES];
    [popupQuery release];
  }
}

// 3G warning confirmation handler
- (void) alertView: (UIAlertView *)alertView didDismissWithButtonIndex: (NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    [self DoDownloadCountry];
  }
}

- (void) TryDownloadCountry
{
  NSString * countryName = [[m_clickedCell textLabel] text];

  if (FreeDiskSpaceInBytes() < (m_downloadSize + MB))
  {
    // No enough disk space - display warning dialog
    [[[[CustomAlertView alloc] initWithTitle:NSLocalizedString(@"not_enough_disk_space", nil)
                                     message:[NSString stringWithFormat:NSLocalizedString(@"free_space_for_country", nil), [self GetStringForSize: m_downloadSize], countryName]
                                    delegate:nil
                           cancelButtonTitle:NSLocalizedString(@"ok", nil)
                           otherButtonTitles:nil] autorelease] show];
  }
  else
  {
    TActiveConnectionType const connType = GetActiveConnectionType();
    if (connType == ENotConnected)
    {
      // No any connection - skip downloading
      [[[[CustomAlertView alloc] initWithTitle:NSLocalizedString(@"no_internet_connection_detected", nil)
                                     message:NSLocalizedString(@"use_wifi_recommendation_text", nil)
                                    delegate:nil
                           cancelButtonTitle:NSLocalizedString(@"ok", nil)
                           otherButtonTitles:nil] autorelease] show];
    }
    else
    {
      if (connType == EConnectedBy3G && m_downloadSize > MAX_3G_MEGABYTES * MB)
      {
        // If user uses 3G, show warning before downloading
        [[[[CustomAlertView alloc] initWithTitle:[NSString stringWithFormat:NSLocalizedString(@"no_wifi_ask_cellular_download", nil), countryName]
                                       message:nil
                                      delegate:self
                             cancelButtonTitle:NSLocalizedString(@"cancel", nil)
                             otherButtonTitles:NSLocalizedString(@"use_cellular_data", nil),
         nil] autorelease] show];
      }
      else
        [self DoDownloadCountry];
    }
  }
}

- (void) tableView: (UITableView *)tableView didSelectRowAtIndexPath: (NSIndexPath *)indexPath
{
	// deselect the current row (don't keep the table selection persistent)
	[tableView deselectRowAtIndexPath: indexPath animated:YES];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath: indexPath];
  
  // Push the new table view on the stack
	TIndex const index = CalculateIndex(m_index, indexPath);
  Framework & frm = GetFramework();
  Storage & s = frm.Storage();
  
  if (s.CountriesCount(index))
  {
    CountriesViewController * newController = [[CountriesViewController alloc] initWithIndex: index andHeader: cell.textLabel.text];
    [self.navigationController pushViewController:newController animated:YES];
    [newController release];
  }
  else
  {
		NSString * countryName = [[cell textLabel] text];

    // pass parameters to dialog handlers
    m_clickedIndex = index;
    m_clickedCell = cell;
    m_countryStatus = frm.GetCountryStatus(index);

		switch (m_countryStatus)
  	{
  		case EOnDisk:
    	{
        // display confirmation popup
    		UIActionSheet * popupQuery = [[[UIActionSheet alloc]
                                       initWithTitle: countryName
                                       delegate: self
                                       cancelButtonTitle: NSLocalizedString(@"cancel", nil)
                                       destructiveButtonTitle: NSLocalizedString(@"delete", nil)
                                       otherButtonTitles: nil] autorelease];
        [popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
        break;
    	}

      case EOnDiskOutOfDate:
      {
        // advise to update or delete country
        m_downloadSize = s.CountrySizeInBytes(index).second;

        NSString * strUpdate = [NSString stringWithFormat:NSLocalizedString(@"update_mb_or_kb", nil), [self GetStringForSize: m_downloadSize]];

        UIActionSheet * popupQuery = [[UIActionSheet alloc]
                                      initWithTitle: countryName
                                      delegate: self
                                      cancelButtonTitle: NSLocalizedString(@"cancel", nil)
                                      destructiveButtonTitle: NSLocalizedString(@"delete", nil)
                                      otherButtonTitles: strUpdate, nil];

        [popupQuery showFromRect: [cell frame] inView: self.view animated: YES];
        [popupQuery release];
        break;
      }

      case ENotDownloaded:
      case EDownloadFailed:
        // advise to download country
        m_downloadSize = s.CountrySizeInBytes(index).second;
        [self TryDownloadCountry];
        break;

  		case EDownloading:
	    {
        // advise to stop download and delete country
		UIActionSheet * popupQuery = [[UIActionSheet alloc] initWithTitle: countryName delegate: self
            cancelButtonTitle: NSLocalizedString(@"do_nothing", nil)
            destructiveButtonTitle: NSLocalizedString(@"cancel_download", nil)
        		otherButtonTitles: nil];

        [popupQuery showFromRect: [cell frame] inView: tableView animated: YES];
    		[popupQuery release];
        break;
    	}

      case EInQueue:
        // cancel download
        frm.DeleteCountry(index);
        break;

      default:
        ASSERT ( false, () );
  	}
  }
}

- (void) OnCountryChange: (TIndex const &)index
{
  if (IsOurIndex(index, m_index))
  {
    UITableView * tableView = (UITableView *)self.view;
    UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: RowFromIndex(index) inSection: 0]];

    if (cell)
      [self UpdateCell: cell forCountry: index];
  }
}

- (void) OnDownload: (TIndex const &)index withProgress: (pair<int64_t, int64_t> const &)progress
{
  if (IsOurIndex(index, m_index))
  {
    UITableView * tableView = (UITableView *)self.view;
    UITableViewCell * cell = [tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: RowFromIndex(index) inSection: 0]];

    if (cell)
      cell.detailTextLabel.text = [NSString stringWithFormat: NSLocalizedString(@"downloading_touch_to_cancel", nil), progress.first * 100 / progress.second];
  }
}

- (void)showGuideAdvertise:(UITapGestureRecognizer *)gr
{
  NSIndexPath * indexPath = [NSIndexPath indexPathForRow:gr.view.tag inSection:0];
  TIndex const index = CalculateIndex(m_index, indexPath);
  guides::GuideInfo info;
  if (!GetFramework().GetGuideInfo(index, info))
    return;
  NSURL * guideUrl = [NSURL URLWithString:[NSString stringWithUTF8String:info.GetAppID().c_str()]];
  UIApplication * app = APP;
  if ([app canOpenURL:guideUrl])
    [app openURL:guideUrl];
  else
    [app openURL:[NSURL URLWithString:[NSString stringWithUTF8String:info.GetURL().c_str()]]];
}

@end
