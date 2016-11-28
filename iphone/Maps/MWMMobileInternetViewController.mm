#import "MWMMobileInternetViewController.h"
#import "SelectableCell.h"
#import "Statistics.h"

@interface MWMMobileInternetViewController ()

@property(weak, nonatomic) IBOutlet SelectableCell * always;
@property(weak, nonatomic) IBOutlet SelectableCell * ask;
@property(weak, nonatomic) IBOutlet SelectableCell * never;
@property(weak, nonatomic) SelectableCell * selected;

@end

@implementation MWMMobileInternetViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_mobile_internet");

  self.never.accessoryType = UITableViewCellAccessoryCheckmark;
  _selected = self.never;
}

- (void)setSelected:(SelectableCell *)selected
{
  if ([_selected isEqual:selected])
    return;

  _selected = selected;
  NSString * statValue = nil;
  if ([selected isEqual:self.always])
  {
    statValue = kStatAlways;
  }
  else if ([selected isEqual:self.ask])
  {
    statValue = kStatAsk;
  }
  else if ([selected isEqual:self.never])
  {
    statValue = kStatNever;
  }

  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : statValue}];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  SelectableCell * selected = self.selected;
  selected.accessoryType = UITableViewCellAccessoryNone;
  selected = [tableView cellForRowAtIndexPath:indexPath];
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  selected.selected = NO;
  self.selected = selected;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return L(@"pref_mobile_internet_hint");
}

@end
