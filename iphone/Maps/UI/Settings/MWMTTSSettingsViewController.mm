#import "MWMTTSSettingsViewController.h"
#import <AVFoundation/AVFoundation.h>
#import "SwiftBridge.h"

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

struct VoiceInstructionCellStrategy : BaseCellStategy
{
  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * controller) override
  {
    Class cls = [SettingsTableViewSwitchCell class];
    auto cell = static_cast<SettingsTableViewSwitchCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [cell configWithDelegate:static_cast<id<SettingsTableViewSwitchCellDelegate>>(controller)
                       title:L(@"pref_tts_enable_title")
                        isOn:[MWMTextToSpeech isTTSEnabled]];
    return cell;
  }
};

struct LanguageCellStrategy : BaseCellStategy
{
  UITableViewCell * BuildCell(UITableView * tableView, NSIndexPath * indexPath,
                              MWMTTSSettingsViewController * controller) override
  {
    Class cls = [SettingsTableViewLinkCell class];
    auto cell = static_cast<SettingsTableViewLinkCell *>(
                        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);

    NSString* language = NSUserDefaults.standardUserDefaults.ttsLanguage;
    NSLocale *locale = [[NSLocale alloc] initWithLocaleIdentifier: language];
    [cell configWithTitle:@"Language" info: [locale localizedStringForLocaleIdentifier:language]];
    return cell;
  }

  NSString * TitleForHeader() const override { return L(@"pref_tts_language_title"); }

  void SelectCell(UITableView * tableView, NSIndexPath * indexPath,
                  MWMTTSSettingsViewController * controller) override
  {
      [controller performSegueWithIdentifier:kSelectTTSLanguageSegueName sender:nil];
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
    auto cell = static_cast<SettingsTableViewSelectableCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    NSString * title = nil;
    switch (static_cast<SpeedCameraManagerMode>(indexPath.row))
    {
    case SpeedCameraManagerMode::Auto: title = L(@"auto"); break;
    case SpeedCameraManagerMode::Always: title = L(@"always"); break;
    case SpeedCameraManagerMode::Never: title = L(@"never"); break;
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

  NSString * TitleForFooter() const override { return L(@"speedcams_notice_message"); }

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

@interface MWMTTSSettingsViewController ()<SettingsTableViewSwitchCellDelegate>
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
    m_strategies.emplace(Underlying(Section::Language), make_unique<LanguageCellStrategy>());
    m_strategies.emplace(Underlying(Section::SpeedCameras), make_unique<CamerasCellStrategy>());
  }

  return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if ([keyPath isEqualToString:@"ttsLanguage"]) {
      [self.tableView reloadData];
    }
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_enable_title");

  [NSUserDefaults.standardUserDefaults addObserver:self forKeyPath:@"ttsLanguage" options:NSKeyValueObservingOptionNew context:nil];
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

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
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
  [MWMTextToSpeech setTTSEnabled:value];
  auto indexSet = [NSIndexSet
      indexSetWithIndexesInRange:{base::Underlying(Section::Language), base::Underlying(Section::Count) - 1}];
  auto const animation = UITableViewRowAnimationFade;
  if (value)
    [self.tableView insertSections:indexSet withRowAnimation:animation];
  else
    [self.tableView deleteSections:indexSet withRowAnimation:animation];
}

@end
