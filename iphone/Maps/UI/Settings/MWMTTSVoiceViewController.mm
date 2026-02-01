#import "MWMTTSVoiceViewController.h"
#import <AVFoundation/AVFoundation.h>
#import "MWMTTSSettingsViewController.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"

static NSString * const kUnwindSegueIdentifier = @"UnwindToTTSSettings";

@implementation MWMTTSVoiceViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_voice_title");
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(SettingsTableViewSelectableCell *)sender
{
  if (![segue.identifier isEqualToString:kUnwindSegueIdentifier])
    return;
  MWMTTSSettingsViewController * dest = segue.destinationViewController;
  UITableViewCell * cell = sender;
  NSUInteger const row = [self.tableView indexPathForCell:cell].row;

  NSArray<AVSpeechSynthesisVoice *> * voices = [MWMTextToSpeech availableVoicesForLanguage:self.language];
  if (row < voices.count)
  {
    AVSpeechSynthesisVoice * selectedVoice = voices[row];
    [[MWMTextToSpeech tts] setVoiceIdentifier:selectedVoice.identifier];
    [dest onVoiceSelected];
  }
}

#pragma mark - UITableViewDataSource && UITableViewDelegate

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (!self.language)
    return 0;
  NSArray<AVSpeechSynthesisVoice *> * voices = [MWMTextToSpeech availableVoicesForLanguage:self.language];
  return voices.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [SettingsTableViewSelectableCell class];
  auto cell = static_cast<SettingsTableViewSelectableCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                               indexPath:indexPath]);

  NSArray<AVSpeechSynthesisVoice *> * voices = [MWMTextToSpeech availableVoicesForLanguage:self.language];
  if (indexPath.row < voices.count)
  {
    AVSpeechSynthesisVoice * voice = voices[indexPath.row];
    NSString * voiceName = voice.name;
    [cell configWithTitle:voiceName];

    // Show checkmark if this is the currently selected voice
    NSString * savedVoiceIdentifier = [MWMTextToSpeech savedVoiceIdentifier];
    if (savedVoiceIdentifier && [voice.identifier isEqualToString:savedVoiceIdentifier])
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
      cell.accessoryType = UITableViewCellAccessoryNone;
  }

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  [self performSegueWithIdentifier:kUnwindSegueIdentifier sender:static_cast<SettingsTableViewSelectableCell *>(cell)];
}

@end
