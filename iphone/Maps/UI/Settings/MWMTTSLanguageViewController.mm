#import "MWMTTSLanguageViewController.h"
#import "MWMTTSSettingsViewController.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"

static NSString * const kUnwingSegueIdentifier = @"UnwindToTTSSettings";

@implementation MWMTTSLanguageViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_other_section_title");
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(SettingsTableViewSelectableCell *)sender
{
  if (![segue.identifier isEqualToString:kUnwingSegueIdentifier])
    return;
  MWMTTSSettingsViewController * dest = segue.destinationViewController;
  UITableViewCell * cell = sender;
  NSUInteger const row = [self.tableView indexPathForCell:cell].row;
  [dest setAdditionalTTSLanguage:[[MWMTextToSpeech tts] availableLanguages][row]];
}

#pragma mark - UITableViewDataSource && UITableViewDelegate

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [[MWMTextToSpeech tts] availableLanguages].size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [SettingsTableViewSelectableCell class];
  auto cell = static_cast<SettingsTableViewSelectableCell *>(
      [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  [cell
      configWithTitle:@([[MWMTextToSpeech tts] availableLanguages][indexPath.row].second.c_str())];
  return cell;
}

@end
