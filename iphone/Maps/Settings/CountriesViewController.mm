#import "CountriesViewController.h"
#import "SettingsManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "WebViewController.h"
#import "CustomAlertView.h"
#import "DiskFreeSpace.h"
#import "Statistics.h"

#include "Framework.h"
#include "GetActiveConnectionType.h"

#include "../../platform/platform.hpp"
#include "../../platform/preferred_languages.hpp"


#define MAX_3G_MEGABYTES (50)
#define MB (1024*1024)
#define BAG_TAG 664


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

static bool getGuideName(string & name, storage::TIndex const & index)
{
  guides::GuideInfo info;
  Framework & f = GetFramework();
  if ((f.Storage().CountriesCount(index) == 0) && f.GetGuideInfo(index, info))
  {
    string const lang = languages::CurrentLanguage();
    name = info.GetAdTitle(lang);
    return true;
  }
  return false;
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
    [[Statistics instance] logEvent:@"Show Map From Download Countries Screen"];
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

- (void) UpdateCell:(UITableViewCell *)cell forCountry:(TIndex const &)countryIndex
{
  cell.accessoryView = nil;

  Framework & frm = GetFramework();
  Storage & s = frm.Storage();
  
  string const & flag = s.CountryFlag(countryIndex);
  guides::GuideInfo info;

  if (!flag.empty())
  {
    cell.imageView.image = [UIImage imageNamed:[NSString stringWithFormat:@"%s.png", flag.c_str()]];
    [cell layoutSubviews];
    for (UIView * v in cell.imageView.subviews)
      if (v.tag == BAG_TAG)
        [v removeFromSuperview];
    if ((s.CountriesCount(countryIndex) == 0) && frm.GetGuideInfo(countryIndex, info))
    {
      UIImageView * im = [[[UIImageView alloc]  initWithImage:[UIImage imageNamed:@"ic_guide_mark"]] autorelease];
      UIView * v = [[[UIView alloc] initWithFrame:im.frame] autorelease];
      [v addSubview:im];
      v.tag = BAG_TAG;
      CGRect r = v.frame;
      r = CGRectMake(0, cell.imageView.frame.size.height - v.frame.size.height, v.frame.size.width, v.frame.size.height);
      v.frame = r;
      [cell.imageView addSubview:v];
    }
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
    	cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
  	else
  		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:cellId] autorelease];
	}

  if (hasChildren)
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  else
    cell.accessoryType = UITableViewCellAccessoryNone;

  cell.textLabel.text = [NSString stringWithUTF8String:s.CountryName(index).c_str()];

  [self UpdateCell:cell forCountry:index];
  return cell;
}

- (void) actionSheet: (UIActionSheet *) actionSheet clickedButtonAtIndex: (NSInteger) buttonIndex
{
  if (buttonIndex != actionSheet.cancelButtonIndex)
  {
    Framework & f = GetFramework();
    NSString * title = [actionSheet buttonTitleAtIndex:buttonIndex];
    guides::GuideInfo info;
    BOOL isGuideAvailable = f.GetGuideInfo(m_clickedIndex, info);
    string const lang = languages::CurrentLanguage();
    if (isGuideAvailable && [title isEqualToString:[NSString stringWithUTF8String:info.GetAdTitle(lang).c_str()]])
    {
      NSURL * guideUrl = [NSURL URLWithString:[NSString stringWithUTF8String:info.GetAppID().c_str()]];
      NSString * countryName = [NSString stringWithUTF8String:f.Storage().CountryName(m_clickedIndex).c_str()];
      [[Statistics instance] logEvent:@"Open Guide Country" withParameters:@{@"Country Name" : countryName}];
      if ([APP canOpenURL:guideUrl])
      {
        [APP openURL:guideUrl];
        [[Statistics instance] logEvent:@"Open Guide Button" withParameters:@{@"Guide downloaded" : @"YES"}];
      }
      else
      {
        [APP openURL:[NSURL URLWithString:[NSString stringWithUTF8String:info.GetURL().c_str()]]];
        [[Statistics instance] logEvent:@"Open Guide Button" withParameters:@{@"Guide downloaded" : @"NO"}];
      }
    }
    else if ([title hasPrefix:[NSString stringWithFormat:NSLocalizedString(@"download_mb_or_kb", nil), @""]]
             || [title hasPrefix:[NSString stringWithFormat:NSLocalizedString(@"update_mb_or_kb", nil),@""]])
      [self TryDownloadCountry];
    else if ([title isEqualToString:NSLocalizedString(@"cancel_download", nil)] || [title isEqualToString:NSLocalizedString(@"delete", nil)])
    {
      f.DeleteCountry(m_clickedIndex);
      m_clickedCell.accessoryType = UITableViewCellAccessoryNone;
    }
    else
      ASSERT ( false, () );
  }
}

- (void) DoDownloadCountry
{
  GetFramework().Storage().DownloadCountry(m_clickedIndex);
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
    [self createActionSheetForCountry:indexPath];
}

- (void) OnCountryChange: (TIndex const &)index
{
  if (IsOurIndex(index, m_index))
  {
    UITableView * tableView = (UITableView *)self.view;
    UITableViewCell * cell = [tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:RowFromIndex(index) inSection:0]];

    if (cell)
      [self UpdateCell:cell forCountry:index];
  }
}

- (void) OnDownload:(TIndex const &)index withProgress:(pair<int64_t, int64_t> const &)progress
{
  if (IsOurIndex(index, m_index))
  {
    UITableView * tableView = (UITableView *)self.view;
    UITableViewCell * cell = [tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:RowFromIndex(index) inSection:0]];

    if (cell)
      cell.detailTextLabel.text = [NSString stringWithFormat:NSLocalizedString(@"downloading_touch_to_cancel", nil), progress.first * 100 / progress.second];
  }
}

-(void)createActionSheetForCountry:(NSIndexPath *)indexPath
{
  UITableView * table = (UITableView *)(self.view);
  UITableViewCell * cell = [table cellForRowAtIndexPath:indexPath];

  Framework & frm = GetFramework();
  m_clickedIndex = CalculateIndex(m_index, indexPath);
  m_countryStatus = frm.GetCountryStatus(m_clickedIndex);
  m_clickedCell = cell;
  storage::Storage & s = GetFramework().Storage();
  m_downloadSize = s.CountrySizeInBytes(m_clickedIndex).second;

  NSMutableArray * buttonNames = [[[NSMutableArray alloc] init] autorelease];

  bool canDelete = NO;

  switch (m_countryStatus)
  {
    case EOnDisk:
    {
      canDelete = YES;
      break;
    }

    case EOnDiskOutOfDate:
      canDelete = YES;
      [buttonNames addObject:[NSString stringWithFormat:NSLocalizedString(@"update_mb_or_kb", nil), [self GetStringForSize: m_downloadSize]]];
      break;
    case ENotDownloaded:
      [buttonNames addObject:[NSString stringWithFormat:NSLocalizedString(@"download_mb_or_kb", nil), [self GetStringForSize: m_downloadSize]]];
      break;

    case EDownloadFailed:
      [self DoDownloadCountry];
      return;

    case EDownloading:
    {
      // special one, with destructive button
      string guideAdevertiseString;
      NSString * guideAd = nil;
      if (getGuideName(guideAdevertiseString, m_clickedIndex))
        guideAd = [NSString stringWithUTF8String:guideAdevertiseString.c_str()];
      UIActionSheet * actionSheet = [[[UIActionSheet alloc] initWithTitle:cell.textLabel.text delegate:self
                                                        cancelButtonTitle: NSLocalizedString(@"do_nothing", nil)
                                                   destructiveButtonTitle: NSLocalizedString(@"cancel_download", nil)
                                                        otherButtonTitles: guideAd, nil] autorelease];
      [actionSheet showFromRect:[cell frame] inView:table animated:YES];
      return;
    }

    case EInQueue:
    {
      frm.DeleteCountry(m_clickedIndex);
      return;
    }

    default:
      ASSERT ( false, () );
  }

  UIActionSheet * as = [[[UIActionSheet alloc]
                         initWithTitle: cell.textLabel.text
                         delegate: self
                         cancelButtonTitle: nil
                         destructiveButtonTitle: nil
                         otherButtonTitles: nil] autorelease];

  for (NSString * str in buttonNames)
    [as addButtonWithTitle:str];
  size_t numOfButtons = [buttonNames count];
  if (canDelete)
  {
    [as addButtonWithTitle:NSLocalizedString(@"delete", nil)];
    as.destructiveButtonIndex = numOfButtons++;
  }

  string guideAdevertiseString;
  if (getGuideName(guideAdevertiseString, m_clickedIndex))
  {
    string const lang = languages::CurrentLanguage();
    [as addButtonWithTitle:[NSString stringWithUTF8String:guideAdevertiseString.c_str()]];
    ++numOfButtons;
  }
  if (isIPhone)
  {
    [as addButtonWithTitle:NSLocalizedString(@"cancel", nil)];
    as.cancelButtonIndex = numOfButtons;
  }
  [as showFromRect: [cell frame] inView:(UITableView *)self.view animated: YES];
}

@end
