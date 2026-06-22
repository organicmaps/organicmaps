#import "MWMMapTilesSettingsViewController.h"

#import <CoreApi/MWMFrameworkHelper.h>
#import "SwiftBridge.h"

#include <cmath>

namespace
{
// One section per control so the section header doubles as the field label.
NSInteger constexpr kSectionEnable = 0;
NSInteger constexpr kSectionURL = 1;
NSInteger constexpr kSectionSize = 2;
NSInteger constexpr kSectionOpacity = 3;
NSInteger constexpr kSectionsCount = 4;

int constexpr kMinCacheSizeMB = 1;
int constexpr kMaxCacheSizeMB = 1000;
int constexpr kMinOpacityPct = 0;
int constexpr kMaxOpacityPct = 100;
}  // namespace

@interface MWMMapTilesSettingsViewController () <UITextFieldDelegate>

@property(nonatomic) UISwitch * enableSwitch;
@property(nonatomic) UITextField * urlField;
@property(nonatomic) UISlider * cacheSizeSlider;
@property(nonatomic) UILabel * cacheSizeValueLabel;
@property(nonatomic) UISlider * opacitySlider;
@property(nonatomic) UILabel * opacityValueLabel;
@property(nonatomic) BOOL backgroundTilesEnabled;
@property(nonatomic) NSString * backgroundTilesURL;
@property(nonatomic) int cacheSizeMB;
@property(nonatomic) int areaOpacityPct;

@end

@implementation MWMMapTilesSettingsViewController

- (instancetype)init
{
  return [super initWithStyle:UITableViewStyleGrouped];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_bg_tiles_title");
  [self loadSettings];
  [self updateURLFieldStyle];
}

#pragma mark - Persisting

- (int)clampValue:(int)value min:(int)min max:(int)max
{
  return value < min ? min : (value > max ? max : value);
}

- (NSString *)trimmedURL:(NSString *)url
{
  return [url stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
}

- (void)loadSettings
{
  self.backgroundTilesEnabled = [MWMFrameworkHelper isBackgroundTilesEnabled];
  self.backgroundTilesURL = [self trimmedURL:[MWMFrameworkHelper backgroundTilesURL]];
  self.cacheSizeMB = [self clampValue:[MWMFrameworkHelper backgroundTilesCacheSizeMB]
                                  min:kMinCacheSizeMB
                                  max:kMaxCacheSizeMB];
  self.areaOpacityPct = [self clampValue:[MWMFrameworkHelper backgroundTilesAreaOpacityPct]
                                     min:kMinOpacityPct
                                     max:kMaxOpacityPct];
}

- (void)syncSettingsFromControls
{
  self.backgroundTilesEnabled = self.enableSwitch ? self.enableSwitch.isOn : self.backgroundTilesEnabled;
  if (self.urlField)
    self.backgroundTilesURL = [self trimmedURL:self.urlField.text ?: @""];
  if (self.cacheSizeSlider)
    self.cacheSizeMB = [self clampValue:(int)std::lround(self.cacheSizeSlider.value)
                                    min:kMinCacheSizeMB
                                    max:kMaxCacheSizeMB];
  if (self.opacitySlider)
    self.areaOpacityPct = [self clampValue:(int)std::lround(self.opacitySlider.value)
                                       min:kMinOpacityPct
                                       max:kMaxOpacityPct];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];

  [self.view endEditing:YES];
  [self syncSettingsFromControls];
  if (![self isConfigValid])
    return;
  [MWMFrameworkHelper setBackgroundTiles:self.backgroundTilesEnabled
                                     url:self.backgroundTilesURL
                             cacheSizeMB:self.cacheSizeMB
                          areaOpacityPct:self.areaOpacityPct];
}

- (void)onEnableChanged:(UISwitch *)sender
{
  self.backgroundTilesEnabled = sender.isOn;
  [self updateFieldsEnabled];
  [self updateURLSectionLayout];
}

- (void)onCacheSizeChanged:(UISlider *)sender
{
  self.cacheSizeMB = [self clampValue:(int)std::lround(sender.value) min:kMinCacheSizeMB max:kMaxCacheSizeMB];
  sender.value = self.cacheSizeMB;
  self.cacheSizeValueLabel.text = [NSString stringWithFormat:@"%d", self.cacheSizeMB];
}

- (void)onOpacityChanged:(UISlider *)sender
{
  self.areaOpacityPct = [self clampValue:(int)std::lround(sender.value) min:kMinOpacityPct max:kMaxOpacityPct];
  sender.value = self.areaOpacityPct;
  self.opacityValueLabel.text = [NSString stringWithFormat:@"%d%%", self.areaOpacityPct];
}

- (void)updateFieldsEnabled
{
  BOOL const enabled = self.backgroundTilesEnabled;
  UIColor * const color = enabled ? UIColor.labelColor : UIColor.tertiaryLabelColor;
  self.urlField.enabled = enabled;
  self.urlField.textColor = color;
  self.cacheSizeSlider.enabled = enabled;
  self.cacheSizeValueLabel.textColor = color;
  self.opacitySlider.enabled = enabled;
  self.opacityValueLabel.textColor = color;
  [self updateURLFieldStyle];
}

#pragma mark - Validation

- (NSString *)titleForURLFooter
{
  return [self isConfigValid] ? L(@"pref_bg_tiles_disclaimer") : L(@"pref_bg_tiles_url_error");
}

- (NSString *)styleForURLCell
{
  return [self isConfigValid] ? @"Background" : @"ErrorBackground";
}

// The URL is validated only when the layer is enabled (a disabled layer is never rendered, so the URL
// may stay empty). Closing is always allowed: an invalid config simply isn't applied on close (see
// viewWillDisappear), and the field is highlighted in red as feedback.
- (BOOL)isConfigValid
{
  [self syncSettingsFromControls];
  return !self.backgroundTilesEnabled || [MWMFrameworkHelper isWellFormedBackgroundTilesURL:self.backgroundTilesURL];
}

- (void)updateURLFieldStyle
{
  [self.urlField.superview setStyleNameAndApply:[self styleForURLCell]];
}

- (void)updateURLSectionLayout
{
  [self updateURLFieldStyle];
  [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:kSectionURL]
                withRowAnimation:UITableViewRowAnimationNone];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  if (textField == self.urlField)
  {
    self.backgroundTilesURL = [self trimmedURL:textField.text ?: @""];
    textField.text = self.backgroundTilesURL;
    [self updateURLSectionLayout];
  }
}

#pragma mark - Cell builders

- (UITextField *)addFieldToCell:(UITableViewCell *)cell
{
  UITextField * field = [[UITextField alloc] initWithFrame:CGRectZero];
  field.translatesAutoresizingMaskIntoConstraints = NO;
  field.delegate = self;
  field.returnKeyType = UIReturnKeyDone;
  field.clearButtonMode = UITextFieldViewModeWhileEditing;
  [cell.contentView addSubview:field];
  UILayoutGuide * margins = cell.contentView.layoutMarginsGuide;
  [NSLayoutConstraint activateConstraints:@[
    [field.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor],
    [field.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor],
    [field.topAnchor constraintEqualToAnchor:margins.topAnchor],
    [field.bottomAnchor constraintEqualToAnchor:margins.bottomAnchor],
  ]];
  return field;
}

- (void)addCacheSizeToCell:(UITableViewCell *)cell
{
  self.cacheSizeValueLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  self.cacheSizeValueLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [self.cacheSizeValueLabel setContentHuggingPriority:UILayoutPriorityRequired
                                              forAxis:UILayoutConstraintAxisHorizontal];

  self.cacheSizeSlider = [[UISlider alloc] initWithFrame:CGRectZero];
  self.cacheSizeSlider.translatesAutoresizingMaskIntoConstraints = NO;
  self.cacheSizeSlider.minimumValue = kMinCacheSizeMB;
  self.cacheSizeSlider.maximumValue = kMaxCacheSizeMB;
  self.cacheSizeSlider.value = self.cacheSizeMB;
  [self.cacheSizeSlider addTarget:self
                           action:@selector(onCacheSizeChanged:)
                 forControlEvents:UIControlEventValueChanged];
  self.cacheSizeValueLabel.text = [NSString stringWithFormat:@"%d", self.cacheSizeMB];

  [cell.contentView addSubview:self.cacheSizeSlider];
  [cell.contentView addSubview:self.cacheSizeValueLabel];
  UILayoutGuide * margins = cell.contentView.layoutMarginsGuide;
  [NSLayoutConstraint activateConstraints:@[
    [self.cacheSizeSlider.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor],
    [self.cacheSizeSlider.topAnchor constraintEqualToAnchor:margins.topAnchor],
    [self.cacheSizeSlider.bottomAnchor constraintEqualToAnchor:margins.bottomAnchor],
    [self.cacheSizeValueLabel.leadingAnchor constraintEqualToAnchor:self.cacheSizeSlider.trailingAnchor constant:8],
    [self.cacheSizeValueLabel.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor],
    [self.cacheSizeValueLabel.centerYAnchor constraintEqualToAnchor:self.cacheSizeSlider.centerYAnchor],
  ]];
}

- (void)addOpacityToCell:(UITableViewCell *)cell
{
  self.opacityValueLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  self.opacityValueLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [self.opacityValueLabel setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];

  self.opacitySlider = [[UISlider alloc] initWithFrame:CGRectZero];
  self.opacitySlider.translatesAutoresizingMaskIntoConstraints = NO;
  self.opacitySlider.minimumValue = kMinOpacityPct;
  self.opacitySlider.maximumValue = kMaxOpacityPct;
  self.opacitySlider.value = self.areaOpacityPct;
  [self.opacitySlider addTarget:self action:@selector(onOpacityChanged:) forControlEvents:UIControlEventValueChanged];
  self.opacityValueLabel.text = [NSString stringWithFormat:@"%d%%", self.areaOpacityPct];

  [cell.contentView addSubview:self.opacitySlider];
  [cell.contentView addSubview:self.opacityValueLabel];
  UILayoutGuide * margins = cell.contentView.layoutMarginsGuide;
  [NSLayoutConstraint activateConstraints:@[
    [self.opacitySlider.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor],
    [self.opacitySlider.topAnchor constraintEqualToAnchor:margins.topAnchor],
    [self.opacitySlider.bottomAnchor constraintEqualToAnchor:margins.bottomAnchor],
    [self.opacityValueLabel.leadingAnchor constraintEqualToAnchor:self.opacitySlider.trailingAnchor constant:8],
    [self.opacityValueLabel.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor],
    [self.opacityValueLabel.centerYAnchor constraintEqualToAnchor:self.opacitySlider.centerYAnchor],
  ]];
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return kSectionsCount;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 1;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (section)
  {
  case kSectionURL: return L(@"pref_bg_tiles_url_title");
  case kSectionSize: return L(@"pref_bg_tiles_size_title");
  case kSectionOpacity: return L(@"pref_bg_tiles_opacity_title");
  default: return nil;
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  switch (section)
  {
  case kSectionURL: return [self titleForURLFooter]; ;
  default: return nil;
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
  cell.selectionStyle = UITableViewCellSelectionStyleNone;

  switch (indexPath.section)
  {
  case kSectionEnable:
  {
    cell.textLabel.text = L(@"pref_bg_tiles_title");
    self.enableSwitch = [[UISwitch alloc] initWithFrame:CGRectZero];
    self.enableSwitch.on = self.backgroundTilesEnabled;
    [self.enableSwitch addTarget:self action:@selector(onEnableChanged:) forControlEvents:UIControlEventValueChanged];
    cell.accessoryView = self.enableSwitch;
    break;
  }
  case kSectionURL:
  {
    self.urlField = [self addFieldToCell:cell];
    self.urlField.text = self.backgroundTilesURL;
    [self.urlField addTarget:self action:@selector(updateURLFieldStyle) forControlEvents:UIControlEventEditingChanged];
    self.urlField.placeholder = @"https://xxx.yyy/{z}/{x}/{y}.png";
    // Default (not URL) keyboard: the URL keyboard makes it hard to enter the "{z}/{x}/{y}" template.
    self.urlField.keyboardType = UIKeyboardTypeDefault;
    self.urlField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    self.urlField.autocorrectionType = UITextAutocorrectionTypeNo;
    break;
  }
  case kSectionSize:
  {
    [self addCacheSizeToCell:cell];
    break;
  }
  case kSectionOpacity:
  {
    [self addOpacityToCell:cell];
    break;
  }
  default: break;
  }

  [self updateFieldsEnabled];
  if (indexPath.section == kSectionURL)
    [cell.contentView setStyleNameAndApply:[self styleForURLCell]];
  return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  switch (section)
  {
  case kSectionURL: return UITableViewAutomaticDimension;
  default: return 0;
  }
}

@end
