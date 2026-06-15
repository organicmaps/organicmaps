#import "MWMMapTilesSettingsViewController.h"

#import <CoreApi/MWMFrameworkHelper.h>

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
@property(nonatomic) UITextField * sizeField;
@property(nonatomic) UISlider * opacitySlider;
@property(nonatomic) UILabel * opacityValueLabel;

@end

@implementation MWMMapTilesSettingsViewController

- (instancetype)init
{
  return [super initWithStyle:UITableViewStyleGrouped];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = NSLocalizedString(@"pref_bg_tiles_title", nil);
}

#pragma mark - Persisting

// Apply all tile settings together when leaving this screen.
- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];

  int size = self.sizeField.text.intValue;
  size = MAX(kMinCacheSizeMB, MIN(kMaxCacheSizeMB, size));
  int const opacity = (int)std::lround(self.opacitySlider.value);
  [MWMFrameworkHelper setBackgroundTilesEnabled:self.enableSwitch.isOn
                                            url:self.urlField.text ?: @""
                                    cacheSizeMB:size
                                 areaOpacityPct:opacity];
}

- (void)onEnableChanged:(UISwitch *)sender
{
  [self updateFieldsEnabled];
}

- (void)onOpacityChanged:(UISlider *)sender
{
  self.opacityValueLabel.text = [NSString stringWithFormat:@"%d%%", (int)std::lround(sender.value)];
}

// Grey out URL / size while disabled; their stored values are kept. Messaging nil fields is a no-op,
// so this is safe to call while cells are still being built.
- (void)updateFieldsEnabled
{
  BOOL const enabled = self.enableSwitch.isOn;
  UIColor * const color = enabled ? UIColor.labelColor : UIColor.tertiaryLabelColor;
  self.urlField.enabled = enabled;
  self.urlField.textColor = color;
  self.sizeField.enabled = enabled;
  self.sizeField.textColor = color;
  self.opacitySlider.enabled = enabled;
  self.opacityValueLabel.textColor = color;
}

#pragma mark - UITextFieldDelegate

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return YES;
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

// Builds the opacity row: a slider (0..100 %) filling the cell with a live "NN%" label on the trailing side.
- (void)addOpacityToCell:(UITableViewCell *)cell
{
  self.opacityValueLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  self.opacityValueLabel.translatesAutoresizingMaskIntoConstraints = NO;
  [self.opacityValueLabel setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];

  self.opacitySlider = [[UISlider alloc] initWithFrame:CGRectZero];
  self.opacitySlider.translatesAutoresizingMaskIntoConstraints = NO;
  self.opacitySlider.minimumValue = kMinOpacityPct;
  self.opacitySlider.maximumValue = kMaxOpacityPct;
  self.opacitySlider.value = [MWMFrameworkHelper backgroundTilesAreaOpacityPct];
  [self.opacitySlider addTarget:self action:@selector(onOpacityChanged:) forControlEvents:UIControlEventValueChanged];
  self.opacityValueLabel.text = [NSString stringWithFormat:@"%d%%", (int)std::lround(self.opacitySlider.value)];

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
  case kSectionURL: return NSLocalizedString(@"pref_bg_tiles_url_title", nil);
  case kSectionSize: return NSLocalizedString(@"pref_bg_tiles_size_title", nil);
  case kSectionOpacity: return NSLocalizedString(@"pref_bg_tiles_opacity_title", nil);
  default: return nil;
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return section == kSectionOpacity ? NSLocalizedString(@"pref_bg_tiles_disclaimer", nil) : nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
  cell.selectionStyle = UITableViewCellSelectionStyleNone;

  switch (indexPath.section)
  {
  case kSectionEnable:
  {
    cell.textLabel.text = NSLocalizedString(@"pref_bg_tiles_enable_title", nil);
    self.enableSwitch = [[UISwitch alloc] initWithFrame:CGRectZero];
    self.enableSwitch.on = [MWMFrameworkHelper isBackgroundTilesEnabled];
    [self.enableSwitch addTarget:self action:@selector(onEnableChanged:) forControlEvents:UIControlEventValueChanged];
    cell.accessoryView = self.enableSwitch;
    break;
  }
  case kSectionURL:
  {
    self.urlField = [self addFieldToCell:cell];
    self.urlField.text = [MWMFrameworkHelper backgroundTilesURL];
    self.urlField.placeholder = NSLocalizedString(@"pref_bg_tiles_url_hint", nil);
    // Default (not URL) keyboard: the URL keyboard makes it hard to enter the "{z}/{x}/{y}" template.
    self.urlField.keyboardType = UIKeyboardTypeDefault;
    self.urlField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    self.urlField.autocorrectionType = UITextAutocorrectionTypeNo;
    break;
  }
  case kSectionSize:
  {
    self.sizeField = [self addFieldToCell:cell];
    self.sizeField.text = [NSString stringWithFormat:@"%d", [MWMFrameworkHelper backgroundTilesCacheSizeMB]];
    self.sizeField.keyboardType = UIKeyboardTypeNumberPad;
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
  return cell;
}

@end
