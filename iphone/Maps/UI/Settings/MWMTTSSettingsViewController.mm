#import "MWMTTSSettingsViewController.h"
#import <AVFoundation/AVFoundation.h>
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"
#import "TTSTester.h"

#include <CoreApi/Framework.h>

#include <memory>
#include <type_traits>
#include <unordered_map>

using namespace routing;
namespace
{
NSString * kSelectTTSLanguageSegueName = @"TTSLanguage";

enum class Section
{
  VoiceInstructions,
  StreetNames,
  Language,
  TestVoice,
  SpeedCameras,
  Count
};

using SectionType = std::underlying_type_t<Section>;

struct BaseCellStategy
{
  virtual UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                                      MWMTTSSettingsViewController * controller) = 0;

  virtual void SelectCell(UITableView * /* tableView */, NSIndexPath * /* indexPath */,
                          MWMTTSSettingsViewController * /* controller */)
  {}

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

// Single cell showing the selected language on the left and its voice on the right. Tapping it opens
// the language picker (MWMTTSLanguageViewController).
struct LanguageCellStrategy : BaseCellStategy
{
  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * /* controller */) override
  {
    Class cls = [SettingsTableViewLinkCell class];
    auto cell = static_cast<SettingsTableViewLinkCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                           indexPath:indexPath]);
    NSString * savedLanguage = [MWMTextToSpeech savedLanguage];
    if (savedLanguage.length == 0)
      savedLanguage = [AVSpeechSynthesisVoice currentLanguageCode];
    NSString * languageName =
        savedLanguage.length > 0 ? @(tts::translateLocale(savedLanguage.UTF8String).c_str()) : @"";
    AVSpeechSynthesisVoice * voice = [[MWMTextToSpeech tts] currentVoice];
    [cell configWithTitle:languageName info:[MWMTextToSpeech displayNameForVoice:voice]];
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    return cell;
  }

  NSString * TitleForHeader() const override { return L(@"pref_tts_language_title"); }

  NSString * TitleForFooter() const override { return L(@"pref_tts_download_voices_description"); }

  void SelectCell(UITableView * /* tableView */, NSIndexPath * /* indexPath */,
                  MWMTTSSettingsViewController * controller) override
  {
    [controller performSegueWithIdentifier:kSelectTTSLanguageSegueName sender:nil];
  }
};

struct TestVoiceCellStrategy : BaseCellStategy
{
  TTSTester * ttsTester = [[TTSTester alloc] init];

  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * /* controller */) override
  {
    Class cls = [SettingsTableViewSelectableCell class];
    auto cell = static_cast<SettingsTableViewSelectableCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                                 indexPath:indexPath]);
    [cell configWithTitle:L(@"pref_tts_test_voice_title")];
    cell.accessoryType = UITableViewCellAccessoryNone;
    return cell;
  }

  void SelectCell(UITableView * /* tableView */, NSIndexPath * /* indexPath */,
                  MWMTTSSettingsViewController * /* controller */) override
  {
    [ttsTester playRandomTestString];
  }
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
}  // namespace

@interface MWMTTSSettingsViewController () <SettingsTableViewSwitchCellDelegate>
{
  std::unordered_map<SectionType, std::unique_ptr<BaseCellStategy>> m_strategies;
}

@end

@implementation MWMTTSSettingsViewController

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    using base::Underlying;
    m_strategies.emplace(Underlying(Section::VoiceInstructions), std::make_unique<VoiceInstructionCellStrategy>());
    m_strategies.emplace(Underlying(Section::StreetNames), std::make_unique<StreetNamesCellStrategy>());
    m_strategies.emplace(Underlying(Section::Language), std::make_unique<LanguageCellStrategy>());
    m_strategies.emplace(Underlying(Section::TestVoice), std::make_unique<TestVoiceCellStrategy>());
    m_strategies.emplace(Underlying(Section::SpeedCameras), std::make_unique<CamerasCellStrategy>());
  }

  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_enable_title");

  // Refresh the selected language/voice when returning from iOS Settings (e.g. after installing a
  // new voice) or from the language/voice pickers.
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(reloadLanguageSection)
                                               name:UIApplicationDidBecomeActiveNotification
                                             object:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self reloadLanguageSection];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)reloadLanguageSection
{
  if ([MWMTextToSpeech isTTSEnabled])
    [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:base::Underlying(Section::Language)]
                  withRowAnimation:UITableViewRowAnimationNone];
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
