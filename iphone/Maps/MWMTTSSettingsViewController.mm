#import "LinkCell.h"
#import "MWMTextToSpeech.h"
#import "MWMTTSSettingsViewController.h"
#import "SelectableCell.h"
#import "Statistics.h"
#import <AVFoundation/AVFoundation.h>

static NSString * kSelectTTSLanguageSegueName = @"TTSLanguage";

using namespace std;

@interface MWMTTSSettingsViewController ()
{
  pair<string, string> _additionalTTSLanguage;
  vector<pair<string, string>> _languages;
}

@property (nonatomic) BOOL isLocaleLanguageAbsent;

@end

@implementation MWMTTSSettingsViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_language_title");
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];

  _languages.reserve(3);
  auto const & v = tts.availableLanguages;
  NSAssert(!v.empty(), @"Vector can't be empty!");
  pair<string, string> const standart = v.front();
  _languages.push_back(standart);

  using namespace tts;
  string const current = bcp47ToTwineLanguage([AVSpeechSynthesisVoice currentLanguageCode]);
  if (current != standart.first && !current.empty())
  {
    string const translated = translatedTwine(current);
    pair<string, string> const cur {current, translated};
    if (translated.empty() || find(v.begin(), v.end(), cur) != v.end())
      _languages.push_back(cur);
    else
      self.isLocaleLanguageAbsent = YES;
  }
  string const savedLanguage = tts.savedLanguage.UTF8String;
  if (savedLanguage != current && savedLanguage != standart.first && !savedLanguage.empty())
    _languages.push_back({savedLanguage, translatedTwine(savedLanguage)});
}

- (IBAction)unwind:(id)sender
{
  size_t const size = _languages.size();
  switch (size)
  {
    case 1:
      _languages.push_back(_additionalTTSLanguage);
      break;
    case 2:
      if (self.isLocaleLanguageAbsent)
        _languages[size - 1] = _additionalTTSLanguage;
      else
        _languages.push_back(_additionalTTSLanguage);
      break;
    case 3:
      _languages[size - 1] = _additionalTTSLanguage;
      break;
    default:
      NSAssert(false, @"Incorrect language's count");
      break;
  }
  [self.tableView reloadData];
}

- (void)setAdditionalTTSLanguage:(pair<string, string> const &)l
{
  [[MWMTextToSpeech tts] setNotificationsLocale:@(l.first.c_str())];
  _additionalTTSLanguage = l;
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == 0)
    return _languages.size() + 1;
  else
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 0 && indexPath.row != _languages.size())
  {
    SelectableCell * cell = (SelectableCell *)[tableView dequeueReusableCellWithIdentifier:[SelectableCell className]];
    pair<string, string> const p = _languages[indexPath.row];
    cell.titleLabel.text = @(p.second.c_str());
    BOOL const isSelected = [@(p.first.c_str()) isEqualToString:[[MWMTextToSpeech tts] savedLanguage]];
    cell.accessoryType = isSelected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    return cell;
  }
  else
  {
    LinkCell * cell = (LinkCell *)[tableView dequeueReusableCellWithIdentifier:[LinkCell className]];
    cell.titleLabel.text = indexPath.section == 0 ? L(@"pref_tts_other_section_title") : L(@"pref_tts_how_to_set_up_voice");
    return cell;
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 0)
  {
    if (indexPath.row == _languages.size())
    {
      [[Statistics instance] logEvent:kStatEventName(kStatTTSSettings, kStatChangeLanguage)
                       withParameters:@{kStatValue : kStatOther}];
      [self performSegueWithIdentifier:kSelectTTSLanguageSegueName sender:nil];
    }
    else
    {
      [[MWMTextToSpeech tts] setNotificationsLocale:@(_languages[indexPath.row].first.c_str())];
      [tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationFade];
    }
  }
#warning need to add help
//  else
//  {
//  }
}

@end
