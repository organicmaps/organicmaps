#import "MWMTTSLanguageViewController.h"
#import <AVFoundation/AVFoundation.h>
#import "MWMTTSSettingsViewController.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"
#import "TTSTester.h"

#include <string>
#include <utility>
#include <vector>

namespace
{
NSString * const kVoiceControllerIdentifier = @"TTSVoice";
NSString * const kVoicePreviewCellIdentifier = @"MWMTTSVoicePreviewCell";

// Pops the navigation stack back to the TTS settings screen, skipping over the language and voice
// pickers, so selecting a voice returns straight to the settings.
void PopToTTSSettings(UIViewController * controller)
{
  for (UIViewController * vc in controller.navigationController.viewControllers)
  {
    if ([vc isKindOfClass:[MWMTTSSettingsViewController class]])
    {
      [controller.navigationController popToViewController:vc animated:YES];
      return;
    }
  }
  [controller.navigationController popViewControllerAnimated:YES];
}
}  // namespace

#pragma mark - Voice preview player

// Speaks a random sample phrase with a specific voice using its own synthesizer and audio session,
// independent of the navigation TTS engine, so a preview never changes the user's selected voice.
// Reports completion so the UI can flip the button from "stop" back to "play".
@interface MWMTTSVoicePreviewPlayer : NSObject <AVSpeechSynthesizerDelegate>

@property(nonatomic, copy) void (^onFinished)(void);
@property(nonatomic, readonly) NSString * playingVoiceIdentifier;

- (void)playVoice:(AVSpeechSynthesisVoice *)voice;
- (void)stop;

@end

@implementation MWMTTSVoicePreviewPlayer
{
  AVSpeechSynthesizer * _synthesizer;
  AVSpeechUtterance * _currentUtterance;
  TTSTester * _tester;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    _synthesizer = [[AVSpeechSynthesizer alloc] init];
    _synthesizer.delegate = self;
    // Use the app's audio session (configured for Playback by MWMTextToSpeech) so previews sound like
    // the "Test Voice Directions" button. A dedicated synthesizer keeps the user's selected voice intact.
    _tester = [[TTSTester alloc] init];
  }
  return self;
}

- (void)dealloc
{
  // Releasing the shared player ends playback and tears down the speech-service connection once.
  [self stop];
}

- (void)playVoice:(AVSpeechSynthesisVoice *)voice
{
  if (!voice)
    return;

  NSString * phrase = [[_tester getTestStrings:voice.language] firstObject];
  if (phrase.length == 0)
    phrase = voice.name;  // Fallback: speak the voice's own name if no sample phrases exist.
  if (phrase.length == 0)
    return;

  if (_synthesizer.isSpeaking)
    [_synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];

  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:phrase];
  utterance.voice = voice;
  utterance.rate = AVSpeechUtteranceDefaultSpeechRate;
  _currentUtterance = utterance;
  _playingVoiceIdentifier = voice.identifier;
  [_synthesizer speakUtterance:utterance];
}

- (void)stop
{
  _currentUtterance = nil;
  _playingVoiceIdentifier = nil;
  if (_synthesizer.isSpeaking)
    [_synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

- (void)finishUtterance:(AVSpeechUtterance *)utterance
{
  // Ignore callbacks from utterances superseded by a newer playback or by an explicit stop.
  if (utterance != _currentUtterance)
    return;
  _currentUtterance = nil;
  _playingVoiceIdentifier = nil;
  if (self.onFinished)
  {
    auto onFinished = self.onFinished;
    dispatch_async(dispatch_get_main_queue(), ^{ onFinished(); });
  }
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance *)utterance
{
  [self finishUtterance:utterance];
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didCancelSpeechUtterance:(AVSpeechUtterance *)utterance
{
  [self finishUtterance:utterance];
}

@end

#pragma mark - Voice preview cell

@class MWMTTSVoicePreviewCell;

@protocol MWMTTSVoicePreviewCellDelegate <NSObject>
- (void)voicePreviewCellDidTapPlay:(MWMTTSVoicePreviewCell *)cell;
@end

// A table cell with a leading play/stop button (for previewing a voice), a title and an optional
// trailing detail. Mirrors the look of the iOS VoiceOver voice picker.
@interface MWMTTSVoicePreviewCell : MWMTableViewCell

@property(nonatomic, weak) id<MWMTTSVoicePreviewCellDelegate> delegate;
@property(nonatomic, copy) NSString * voiceIdentifier;
@property(nonatomic, getter=isPlaying) BOOL playing;

- (void)configWithTitle:(NSString *)title detail:(NSString *)detail voiceIdentifier:(NSString *)voiceIdentifier;

@end

static UIImage * PreviewIcon(NSString * name)
{
  UIImageSymbolConfiguration * config =
      [UIImageSymbolConfiguration configurationWithPointSize:26 weight:UIImageSymbolWeightRegular];
  return [UIImage systemImageNamed:name withConfiguration:config];
}

@implementation MWMTTSVoicePreviewCell
{
  UIButton * _playButton;
  UILabel * _titleLabel;
  UILabel * _detailLabel;
}

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self)
    [self setupSubviews];
  return self;
}

- (void)setupSubviews
{
  _playButton = [UIButton buttonWithType:UIButtonTypeSystem];
  _playButton.translatesAutoresizingMaskIntoConstraints = NO;
  [_playButton addTarget:self action:@selector(onPlayButtonTapped) forControlEvents:UIControlEventTouchUpInside];
  [self.contentView addSubview:_playButton];

  _titleLabel = [[UILabel alloc] init];
  _titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
  _titleLabel.font = [UIFont systemFontOfSize:17];
  [self.contentView addSubview:_titleLabel];

  _detailLabel = [[UILabel alloc] init];
  _detailLabel.translatesAutoresizingMaskIntoConstraints = NO;
  _detailLabel.font = [UIFont systemFontOfSize:17];
  _detailLabel.textAlignment = NSTextAlignmentRight;
  [self.contentView addSubview:_detailLabel];

  [_titleLabel setContentHuggingPriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
  [_titleLabel setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                               forAxis:UILayoutConstraintAxisHorizontal];
  [_detailLabel setContentHuggingPriority:UILayoutPriorityDefaultHigh forAxis:UILayoutConstraintAxisHorizontal];
  [_detailLabel setContentCompressionResistancePriority:UILayoutPriorityRequired
                                                forAxis:UILayoutConstraintAxisHorizontal];

  UILayoutGuide * margins = self.contentView.layoutMarginsGuide;
  [NSLayoutConstraint activateConstraints:@[
    [_playButton.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor],
    [_playButton.centerYAnchor constraintEqualToAnchor:self.contentView.centerYAnchor],
    [_playButton.widthAnchor constraintEqualToConstant:30],
    [_playButton.heightAnchor constraintEqualToConstant:30],

    [_titleLabel.leadingAnchor constraintEqualToAnchor:_playButton.trailingAnchor constant:12],
    [_titleLabel.topAnchor constraintEqualToAnchor:self.contentView.topAnchor constant:12],
    [_titleLabel.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-12],

    [_detailLabel.leadingAnchor constraintGreaterThanOrEqualToAnchor:_titleLabel.trailingAnchor constant:8],
    [_detailLabel.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor],
    [_detailLabel.centerYAnchor constraintEqualToAnchor:self.contentView.centerYAnchor],
  ]];

  self.playing = NO;
  [self applyColors];
}

- (void)applyColors
{
  _titleLabel.textColor = [UIColor blackPrimaryText];
  _detailLabel.textColor = [UIColor blackSecondaryText];
  _playButton.tintColor = [UIColor linkBlue];
}

- (void)applyTheme
{
  [super applyTheme];
  [self applyColors];
}

- (void)configWithTitle:(NSString *)title detail:(NSString *)detail voiceIdentifier:(NSString *)voiceIdentifier
{
  _titleLabel.text = title;
  _detailLabel.text = detail;
  _detailLabel.hidden = detail.length == 0;
  self.voiceIdentifier = voiceIdentifier;
  [self applyColors];
}

- (void)setPlaying:(BOOL)playing
{
  _playing = playing;
  [_playButton setImage:PreviewIcon(playing ? @"stop.circle.fill" : @"play.circle.fill") forState:UIControlStateNormal];
}

- (void)onPlayButtonTapped
{
  [self.delegate voicePreviewCellDidTapPlay:self];
}

- (void)prepareForReuse
{
  [super prepareForReuse];
  self.delegate = nil;
  self.voiceIdentifier = nil;
  self.playing = NO;
  self.accessoryType = UITableViewCellAccessoryNone;
}

@end

#pragma mark - Base preview controller

@interface MWMTTSPreviewTableViewController () <MWMTTSVoicePreviewCellDelegate>

// The preview player (and its synthesizer) is shared across the pickers: the language picker creates
// it and hands it to the pushed voice picker, so a single synthesizer serves the whole flow and is
// released — cleaned up — once both pickers leave the navigation stack.
@property(nonatomic, strong) MWMTTSVoicePreviewPlayer * previewPlayer;

- (MWMTTSVoicePreviewCell *)dequeuePreviewCellForIndexPath:(NSIndexPath *)indexPath
                                                     voice:(AVSpeechSynthesisVoice *)voice
                                                     title:(NSString *)title
                                                    detail:(NSString *)detail;
- (void)updatePlayButtons;

@end

@implementation MWMTTSPreviewTableViewController

- (MWMTTSVoicePreviewPlayer *)previewPlayer
{
  if (!_previewPlayer)
    _previewPlayer = [[MWMTTSVoicePreviewPlayer alloc] init];
  return _previewPlayer;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.tableView registerClass:[MWMTTSVoicePreviewCell class] forCellReuseIdentifier:kVoicePreviewCellIdentifier];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  // The visible picker owns the finish callback so it refreshes its own buttons when playback ends.
  __weak __typeof(self) weakSelf = self;
  self.previewPlayer.onFinished = ^{ [weakSelf updatePlayButtons]; };
  [self updatePlayButtons];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  // Stop playback when leaving; the shared synthesizer is released when the picker flow ends.
  [_previewPlayer stop];
}

- (MWMTTSVoicePreviewCell *)dequeuePreviewCellForIndexPath:(NSIndexPath *)indexPath
                                                     voice:(AVSpeechSynthesisVoice *)voice
                                                     title:(NSString *)title
                                                    detail:(NSString *)detail
{
  auto cell = static_cast<MWMTTSVoicePreviewCell *>(
      [self.tableView dequeueReusableCellWithIdentifier:kVoicePreviewCellIdentifier forIndexPath:indexPath]);
  cell.delegate = self;
  [cell configWithTitle:title detail:detail voiceIdentifier:voice.identifier];
  cell.playing =
      voice.identifier.length > 0 && [voice.identifier isEqualToString:self.previewPlayer.playingVoiceIdentifier];
  return cell;
}

- (void)updatePlayButtons
{
  NSString * playingIdentifier = self.previewPlayer.playingVoiceIdentifier;
  for (UITableViewCell * cell in self.tableView.visibleCells)
  {
    if (![cell isKindOfClass:[MWMTTSVoicePreviewCell class]])
      continue;
    auto previewCell = static_cast<MWMTTSVoicePreviewCell *>(cell);
    previewCell.playing =
        playingIdentifier.length > 0 && [previewCell.voiceIdentifier isEqualToString:playingIdentifier];
  }
}

#pragma mark - MWMTTSVoicePreviewCellDelegate

- (void)voicePreviewCellDidTapPlay:(MWMTTSVoicePreviewCell *)cell
{
  NSString * voiceIdentifier = cell.voiceIdentifier;
  if (voiceIdentifier.length > 0 && [voiceIdentifier isEqualToString:self.previewPlayer.playingVoiceIdentifier])
    [self.previewPlayer stop];
  else
    [self.previewPlayer playVoice:[AVSpeechSynthesisVoice voiceWithIdentifier:voiceIdentifier]];
  [self updatePlayButtons];
}

@end

#pragma mark - Language picker

@interface MWMTTSLanguageViewController ()
{
  std::vector<std::pair<std::string, std::string>> m_languages;
}
@end

@implementation MWMTTSLanguageViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_tts_language_title");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  // Refresh on every appearance to reflect voices installed via iOS Settings while away.
  m_languages = [[MWMTextToSpeech tts] availableLanguages];
  [self.tableView reloadData];
}

#pragma mark - UITableViewDataSource && UITableViewDelegate

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_languages.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & language = m_languages[indexPath.row];
  NSString * languageName = @(language.second.c_str());
  NSArray<AVSpeechSynthesisVoice *> * voices = [MWMTextToSpeech availableVoicesForLanguage:@(language.first.c_str())];

  if (voices.count == 1)
  {
    // A single voice: selectable row with the voice name on the right and a leading preview button.
    AVSpeechSynthesisVoice * voice = voices.firstObject;
    MWMTTSVoicePreviewCell * cell = [self dequeuePreviewCellForIndexPath:indexPath
                                                                   voice:voice
                                                                   title:languageName
                                                                  detail:[MWMTextToSpeech displayNameForVoice:voice]];
    cell.accessoryType = UITableViewCellAccessoryNone;
    return cell;
  }

  // Several voices (disclosure to the voice picker) or none: a plain link cell.
  Class cls = [SettingsTableViewLinkCell class];
  auto cell = static_cast<SettingsTableViewLinkCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                         indexPath:indexPath]);
  [cell configWithTitle:languageName info:nil];
  cell.accessoryType = voices.count > 1 ? UITableViewCellAccessoryDisclosureIndicator : UITableViewCellAccessoryNone;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  auto const & language = m_languages[indexPath.row];
  NSString * bcp47 = @(language.first.c_str());
  NSArray<AVSpeechSynthesisVoice *> * voices = [MWMTextToSpeech availableVoicesForLanguage:bcp47];

  if (voices.count > 1)
  {
    MWMTTSVoiceViewController * voiceController =
        [self.storyboard instantiateViewControllerWithIdentifier:kVoiceControllerIdentifier];
    voiceController.languageBcp47 = bcp47;
    voiceController.languageName = @(language.second.c_str());
    voiceController.previewPlayer = self.previewPlayer;  // share one synthesizer across the flow
    [self.navigationController pushViewController:voiceController animated:YES];
    return;
  }

  [[MWMTextToSpeech tts] setNotificationsLocale:bcp47];
  if (voices.count == 1)
    [[MWMTextToSpeech tts] setVoiceIdentifier:voices.firstObject.identifier];
  PopToTTSSettings(self);
}

@end

#pragma mark - Voice picker

@implementation MWMTTSVoiceViewController
{
  NSArray<AVSpeechSynthesisVoice *> * m_voices;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = self.languageName.length > 0 ? self.languageName : L(@"pref_tts_voice_title");
  m_voices = [MWMTextToSpeech availableVoicesForLanguage:self.languageBcp47];
}

#pragma mark - UITableViewDataSource && UITableViewDelegate

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return L(@"pref_tts_voice_title");
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_voices.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  AVSpeechSynthesisVoice * voice = m_voices[indexPath.row];
  MWMTTSVoicePreviewCell * cell = [self dequeuePreviewCellForIndexPath:indexPath
                                                                 voice:voice
                                                                 title:[MWMTextToSpeech displayNameForVoice:voice]
                                                                detail:nil];
  AVSpeechSynthesisVoice * currentVoice = [[MWMTextToSpeech tts] currentVoice];
  BOOL const isSelected = currentVoice && [voice.identifier isEqualToString:currentVoice.identifier];
  cell.accessoryType = isSelected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  AVSpeechSynthesisVoice * voice = m_voices[indexPath.row];
  [[MWMTextToSpeech tts] setNotificationsLocale:self.languageBcp47];
  [[MWMTextToSpeech tts] setVoiceIdentifier:voice.identifier];
  PopToTTSSettings(self);
}

@end
