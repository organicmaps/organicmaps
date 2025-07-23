#import "MWMTTSSettingsViewController.h"
#import <AVFoundation/AVFoundation.h>
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"
#import "TTSTester.h"

#include <CoreApi/Framework.h>
#include "LocaleTranslator.h"

using namespace locale_translator;
using namespace routing;
using namespace std;

namespace
{
NSString * kSelectTTSLanguageSegueName = @"TTSLanguage";

enum class Section
{
  VoiceInstructions,
  StreetNames,
  Language,
  SpeedCameras,
  Count
};

using SectionType = underlying_type_t<Section>;

struct BaseCellStategy
{
  virtual UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                                      MWMTTSSettingsViewController * controller) = 0;

  virtual void SelectCell(UITableView * /* tableView */, NSIndexPath * /* indexPath */,
                          MWMTTSSettingsViewController * /* controller */) {};

  virtual NSString * TitleForFooter() const { return nil; }

  virtual NSString * TitleForHeader() const { return nil; }

  virtual size_t NumberOfRows(MWMTTSSettingsViewController * /* controller */) const { return 1; }

  virtual ~BaseCellStategy() {}
};

UITableViewCell * voiceInstructionsCell;
UITableViewCell * streetNamesCell;

struct VoiceInstructionCellStrategy : BaseCellStategy
{
  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * controller) override
  {
    Class cls = [SettingsTableViewSwitchCell class];
    auto cell = static_cast<SettingsTableViewSwitchCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                             indexPath:indexPath]);
    [cell configWithDelegate:static_cast<id<SettingsTableViewSwitchCellDelegate>>(controller)
                       title:L(@"pref_tts_enable_title")
                        isOn:[MWMTextToSpeech isTTSEnabled]];
    voiceInstructionsCell = cell;
    return cell;
  }
};

struct LanguageCellStrategy : BaseCellStategy
{
  TTSTester * ttsTester = [[TTSTester alloc] init];

  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * controller) override
  {
    NSInteger const row = indexPath.row;
    // "Other" cell
    if (row == controller.languages.size())
    {
      Class cls = [SettingsTableViewLinkCell class];
      auto cell = static_cast<SettingsTableViewLinkCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                             indexPath:indexPath]);
      [cell configWithTitle:L(@"pref_tts_other_section_title") info:nil];
      return cell;
    }

    // "Test TTS" cell
    if (row == controller.languages.size() + 1)
    {
      Class cls = [SettingsTableViewSelectableCell class];
      auto cell =
          static_cast<SettingsTableViewSelectableCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                           indexPath:indexPath]);
      [cell configWithTitle:L(@"pref_tts_test_voice_title")];
      cell.accessoryType = UITableViewCellAccessoryNone;
      return cell;
    }

    Class cls = [SettingsTableViewSelectableCell class];
    auto cell = static_cast<SettingsTableViewSelectableCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                                 indexPath:indexPath]);
    pair<string, string> const p = controller.languages[row];
    [cell configWithTitle:@(p.second.c_str())];
    BOOL const isSelected = [@(p.first.c_str()) isEqualToString:[MWMTextToSpeech savedLanguage]];
    if (isSelected)
    {
      m_selectedCell = cell;
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    }
    else
    {
      cell.accessoryType = UITableViewCellAccessoryNone;
    }

    return cell;
  }

  size_t NumberOfRows(MWMTTSSettingsViewController * controller) const override
  {
    return controller.languages.size() + 2;  // Number of languages + "Other" cell + "TTS Test" cell
  }

  NSString * TitleForHeader() const override { return L(@"pref_tts_language_title"); }

  void SelectCell(UITableView * tableView, NSIndexPath * indexPath, MWMTTSSettingsViewController * controller) override
  {
    NSInteger const row = indexPath.row;
    if (row == controller.languages.size())
    {
      [controller performSegueWithIdentifier:kSelectTTSLanguageSegueName sender:nil];
      return;
    }

    if (row == controller.languages.size() + 1)
    {
      [ttsTester playRandomTestString];
      return;
    }

    auto cell = [tableView cellForRowAtIndexPath:indexPath];
    if (m_selectedCell == cell)
      return;

    m_selectedCell.accessoryType = UITableViewCellAccessoryNone;
    [[MWMTextToSpeech tts] setNotificationsLocale:@(controller.languages[row].first.c_str())];
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
    m_selectedCell = cell;
  }

  SettingsTableViewSelectableCell * m_selectedCell = nil;
};

struct CamerasCellStrategy : BaseCellStategy
{
  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * controller) override
  {
    auto const mode = GetFramework().GetRoutingManager().GetSpeedCamManager().GetMode();
    Class cls = [SettingsTableViewSelectableCell class];
    auto cell = static_cast<SettingsTableViewSelectableCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                                 indexPath:indexPath]);
    NSString * title = nil;
    switch (static_cast<SpeedCameraManagerMode>(indexPath.row))
    {
    case SpeedCameraManagerMode::Auto: title = L(@"pref_tts_speedcams_auto"); break;
    case SpeedCameraManagerMode::Always: title = L(@"pref_tts_speedcams_always"); break;
    case SpeedCameraManagerMode::Never: title = L(@"pref_tts_speedcams_never"); break;
    case SpeedCameraManagerMode::MaxValue: CHECK(false, ()); return nil;
    }

    CHECK(title, ());
    [cell configWithTitle:title];
    bool const isSelectedCell = base::Underlying(mode) == indexPath.row;
    if (isSelectedCell)
    {
      m_selectedCell = cell;
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    }
    else
    {
      cell.accessoryType = UITableViewCellAccessoryNone;
    }

    return cell;
  }

  size_t NumberOfRows(MWMTTSSettingsViewController * /* controller */) const override
  {
    return base::Underlying(SpeedCameraManagerMode::MaxValue);
  }

  NSString * TitleForHeader() const override { return L(@"speedcams_alert_title"); }

  NSString * TitleForFooter() const override { return nil; }

  void SelectCell(UITableView * tableView, NSIndexPath * indexPath,
                  MWMTTSSettingsViewController * /* controller */) override
  {
    auto cell = [tableView cellForRowAtIndexPath:indexPath];
    if (cell == m_selectedCell)
      return;

    m_selectedCell.accessoryType = UITableViewCellAccessoryNone;
    auto & scm = GetFramework().GetRoutingManager().GetSpeedCamManager();
    auto const mode = static_cast<SpeedCameraManagerMode>(indexPath.row);
    CHECK_NOT_EQUAL(mode, SpeedCameraManagerMode::MaxValue, ());
    scm.SetMode(mode);
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
    m_selectedCell = cell;
  }

  SettingsTableViewSelectableCell * m_selectedCell = nil;
};

struct StreetNamesCellStrategy : BaseCellStategy
{
  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * controller) override
  {
    Class cls = [SettingsTableViewSwitchCell class];
    auto cell = static_cast<SettingsTableViewSwitchCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                             indexPath:indexPath]);
    [cell configWithDelegate:static_cast<id<SettingsTableViewSwitchCellDelegate>>(controller)
                       title:L(@"pref_tts_street_names_title")
                        isOn:[MWMTextToSpeech isStreetNamesTTSEnabled]];
    streetNamesCell = cell;
    return cell;
  }

  NSString * TitleForFooter() const override { return L(@"pref_tts_street_names_description"); }
};
}  // namespace

@interface MWMTTSSettingsViewController () <SettingsTableViewSwitchCellDelegate>
{
  pair<string, string> m_additionalTTSLanguage;
  vector<pair<string, string>> m_languages;
  unordered_map<SectionType, unique_ptr<BaseCellStategy>> m_strategies;
}

@property(nonatomic) BOOL isLocaleLanguageAbsent;

@end

@implementation MWMTTSSettingsViewController

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    using base::Underlying;
    m_strategies.emplace(Underlying(Section::VoiceInstructions), make_unique<VoiceInstructionCellStrategy>());
    m_strategies.emplace(Underlying(Section::StreetNames), make_unique<StreetNamesCellStrategy>());
    m_strategies.emplace(Underlying(Section::Language), make_unique<LanguageCellStrategy>());
    m_strategies.emplace(Underlying(Section::SpeedCameras), make_unique<CamerasCellStrategy>());
  }

  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_enable_title");
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];

  m_languages.reserve(3);
  pair<string, string> const standard = tts.standardLanguage;
  m_languages.push_back(standard);

  using namespace tts;
  NSString * currentBcp47 = [AVSpeechSynthesisVoice currentLanguageCode];
  string const currentBcp47Str = currentBcp47.UTF8String;
  if (currentBcp47Str != standard.first && !currentBcp47Str.empty())
  {
    auto const & v = tts.availableLanguages;
    NSAssert(!v.empty(), @"Vector can't be empty!");
    std::string const translated = translateLocale(currentBcp47Str);
    auto cur = std::make_pair(currentBcp47Str, translated);
    if (translated.empty() || find(v.begin(), v.end(), cur) != v.end())
      m_languages.push_back(std::move(cur));
    else
      self.isLocaleLanguageAbsent = YES;
  }

  NSString * nsSavedLanguage = [MWMTextToSpeech savedLanguage];
  if (nsSavedLanguage.length)
  {
    std::string const savedLanguage = nsSavedLanguage.UTF8String;
    if (savedLanguage != currentBcp47Str && savedLanguage != standard.first && !savedLanguage.empty())
      m_languages.emplace_back(savedLanguage, translateLocale(savedLanguage));
  }
}

- (IBAction)unwind:(id)sender
{
  size_t const size = m_languages.size();
  if (find(m_languages.begin(), m_languages.end(), m_additionalTTSLanguage) != m_languages.end())
  {
    [self.tableView reloadData];
    return;
  }
  switch (size)
  {
  case 1: m_languages.push_back(m_additionalTTSLanguage); break;
  case 2:
    if (self.isLocaleLanguageAbsent)
      m_languages[size - 1] = m_additionalTTSLanguage;
    else
      m_languages.push_back(m_additionalTTSLanguage);
    break;
  case 3: m_languages[size - 1] = m_additionalTTSLanguage; break;
  default: NSAssert(false, @"Incorrect language's count"); break;
  }
  [self.tableView reloadData];
}

- (void)setAdditionalTTSLanguage:(pair<string, string> const &)l
{
  [[MWMTextToSpeech tts] setNotificationsLocale:@(l.first.c_str())];
  m_additionalTTSLanguage = l;
}

- (vector<pair<string, string>> const &)languages
{
  return m_languages;
}

#pragma mark - UITableViewDataSource

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  auto const & strategy = m_strategies[static_cast<SectionType>(section)];
  CHECK(strategy, ());
  return strategy->TitleForHeader();
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  auto const & strategy = m_strategies[static_cast<SectionType>(section)];
  CHECK(strategy, ());
  return strategy->TitleForFooter();
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [MWMTextToSpeech isTTSEnabled] ? base::Underlying(Section::Count) : 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  auto const & strategy = m_strategies[static_cast<SectionType>(section)];
  CHECK(strategy, ());
  return strategy->NumberOfRows(self);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & strategy = m_strategies[static_cast<SectionType>(indexPath.section)];
  CHECK(strategy, ());
  return strategy->BuildCell(tableView, indexPath, self);
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  auto const & strategy = m_strategies[static_cast<SectionType>(indexPath.section)];
  CHECK(strategy, ());
  return strategy->SelectCell(tableView, indexPath, self);
}

#pragma mark - SettingsTableViewSwitchCellDelegate

- (void)switchCell:(SettingsTableViewSwitchCell *)cell didChangeValue:(BOOL)value
{
  if (cell == voiceInstructionsCell)
  {
    [MWMTextToSpeech setTTSEnabled:value];
    auto indexSet = [NSIndexSet
        indexSetWithIndexesInRange:{base::Underlying(Section::StreetNames), base::Underlying(Section::Count) - 1}];
    auto const animation = UITableViewRowAnimationFade;
    if (value)
      [self.tableView insertSections:indexSet withRowAnimation:animation];
    else
      [self.tableView deleteSections:indexSet withRowAnimation:animation];
  }
  else if (cell == streetNamesCell)
  {
    [MWMTextToSpeech setStreetNamesTTSEnabled:value];
  }
}

@end
