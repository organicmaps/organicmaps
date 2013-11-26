
#import "SettingsViewController.h"
#import "SwitchCell.h"
#import "SelectableCell.h"
#import "LinkCell.h"
#import "WebViewController.h"
#include "../../../platform/settings.hpp"
#include "../../platform/platform.hpp"
#include "../../platform/preferred_languages.hpp"
#import "MapViewController.h"
#import "MapsAppDelegate.h"

typedef NS_ENUM(NSUInteger, Section)
{
  SectionMetrics,
  SectionStatistics,
  SectionZoomButtons,
  SectionAbout,
  SectionCount
};

@interface SettingsViewController () <SwitchCellDelegate>

@end

@implementation SettingsViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = NSLocalizedString(@"settings", nil);
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return SectionCount;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == SectionMetrics)
    return 2;
  else
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  if (indexPath.section == SectionMetrics)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:NSStringFromClass([SelectableCell class])];
    Settings::Units units;
    if (!Settings::Get("Units", units))
      units = Settings::Metric;
    BOOL selected = units == unitsForIndex(indexPath.row);

    SelectableCell * customCell = (SelectableCell *)cell;
    customCell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    customCell.titleLabel.text = indexPath.row == 0 ? NSLocalizedString(@"kilometres", nil) : NSLocalizedString(@"miles", nil);
  }
  else if (indexPath.section == SectionStatistics)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:NSStringFromClass([SwitchCell class])];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on;
    if (!Settings::Get("StatisticsEnabled", on))
      on = true;
    customCell.switchButton.on = on;
    customCell.titleLabel.text = NSLocalizedString(@"allow_statistics", nil);
    customCell.delegate = self;
  }
  else if (indexPath.section == SectionZoomButtons)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:NSStringFromClass([SwitchCell class])];
    SwitchCell * customCell = (SwitchCell *)cell;
    bool on;
    if (!Settings::Get("ZoomButtonsEnabled", on))
      on = false;
    customCell.switchButton.on = on;
    customCell.titleLabel.text = NSLocalizedString(@"pref_zoom_title", nil);
    customCell.delegate = self;
  }
  else if (indexPath.section == SectionAbout)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:NSStringFromClass([LinkCell class])];
    LinkCell * customCell = (LinkCell *)cell;
    customCell.titleLabel.text = NSLocalizedString(@"about_menu_title", nil);
  }

  return cell;
}

- (void)switchCell:(SwitchCell *)cell didChangeValue:(BOOL)value
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  if (indexPath.section == SectionStatistics)
  {
    Settings::Set("StatisticsEnabled", (bool)value);
  }
  else if (indexPath.section == SectionZoomButtons)
  {
    Settings::Set("ZoomButtonsEnabled", (bool)value);
    [MapsAppDelegate theApp].m_mapViewController.zoomButtonsView.hidden = !value;
  }
}

Settings::Units unitsForIndex(NSInteger index)
{
  if (index == 0)
    return Settings::Metric;
  else
    return Settings::Foot;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == SectionMetrics)
  {
    Settings::Units units = unitsForIndex(indexPath.row);
    Settings::Set("Units", units);
    [tableView reloadSections:[NSIndexSet indexSetWithIndex:SectionMetrics] withRowAnimation:UITableViewRowAnimationNone];
    [[MapsAppDelegate theApp].m_mapViewController SetupMeasurementSystem];
  }
  else if (indexPath.section == SectionAbout)
  {
    ReaderPtr<Reader> r = GetPlatform().GetReader("about.html");
    string s;
    r.ReadAsString(s);
    NSString * str = [NSString stringWithFormat:@"Version: %@ \n", [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"]];
    NSString * text = [NSString stringWithFormat:@"%@%@", str, [NSString stringWithUTF8String:s.c_str()]];
    WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:NSLocalizedString(@"about", nil)];
    [self.navigationController pushViewController:aboutViewController animated:YES];
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == SectionMetrics)
    return NSLocalizedString(@"measurement_units", nil);
  else
    return nil;
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

- (void)viewDidUnload
{
  [super viewDidUnload];
}

@end
