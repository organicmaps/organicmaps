#import "MWMTTSVoiceViewController.h"
#import "MWMTTSSettingsViewController.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"
#import "TTSTester.h"

static NSString * const kUnwingSegueIdentifier = @"UnwindToTTSSettings";

@implementation MWMTTSVoiceViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_voices_section_title");
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto voice = [[MWMTextToSpeech tts] availableVoicesForCurrentLanguage][indexPath.row];
  [[MWMTextToSpeech tts] setVoice:@(voice.first.c_str())];
  TTSTester * ttsTester = [[TTSTester alloc] init];
  [ttsTester playRandomTestString];
  [self.tableView reloadData];
}

#pragma mark - UITableViewDataSource && UITableViewDelegate

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [[MWMTextToSpeech tts] availableVoicesForCurrentLanguage].size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [SettingsTableViewSelectableCell class];
  auto cell = static_cast<SettingsTableViewSelectableCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                               indexPath:indexPath]);

  NSString * defaultVoiceIdentifier = [[MWMTextToSpeech tts] getDefaultVoice];
  NSString * currentVoiceIdentifier =
      @([[MWMTextToSpeech tts] availableVoicesForCurrentLanguage][indexPath.row].second.c_str());

  if ([defaultVoiceIdentifier isEqualToString:currentVoiceIdentifier])
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  else
    cell.accessoryType = UITableViewCellAccessoryNone;
  [cell configWithTitle:@([[MWMTextToSpeech tts] availableVoicesForCurrentLanguage][indexPath.row].second.c_str())];
  return cell;
}

@end
