#import "MWMAuthorizationCommon.h"
#import "MWMOpeningHoursAddScheduleTableViewCell.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMOpeningHoursModel.h"
#import "MWMOpeningHoursSection.h"
#import "MWMTextView.h"
#import "Statistics.h"

extern NSDictionary * const kMWMOpeningHoursEditorTableCells = @{
  @(MWMOpeningHoursEditorDaysSelectorCell) : @"MWMOpeningHoursDaysSelectorTableViewCell",
  @(MWMOpeningHoursEditorAllDayCell) : @"MWMOpeningHoursAllDayTableViewCell",
  @(MWMOpeningHoursEditorTimeSpanCell) : @"MWMOpeningHoursTimeSpanTableViewCell",
  @(MWMOpeningHoursEditorTimeSelectorCell) : @"MWMOpeningHoursTimeSelectorTableViewCell",
  @(MWMOpeningHoursEditorClosedSpanCell) : @"MWMOpeningHoursClosedSpanTableViewCell",
  @(MWMOpeningHoursEditorAddClosedCell) : @"MWMOpeningHoursAddClosedTableViewCell",
  @(MWMOpeningHoursEditorDeleteScheduleCell) : @"MWMOpeningHoursDeleteScheduleTableViewCell",
  @(MWMOpeningHoursEditorSpacerCell) : @"MWMOpeningHoursSpacerTableViewCell",
  @(MWMOpeningHoursEditorAddScheduleCell) : @"MWMOpeningHoursAddScheduleTableViewCell",
};

@interface MWMOpeningHoursEditorViewController ()<UITableViewDelegate, UITableViewDataSource,
                                                  UITextViewDelegate, MWMOpeningHoursModelProtocol>

@property (weak, nonatomic, readwrite) IBOutlet UITableView * tableView;
@property (weak, nonatomic, readwrite) IBOutlet UIView * advancedEditor;
@property (weak, nonatomic, readwrite) IBOutlet MWMTextView * editorView;
@property (weak, nonatomic) IBOutlet UIView * helpView;
@property (weak, nonatomic) IBOutlet UITextView * help;
@property (weak, nonatomic, readwrite) IBOutlet NSLayoutConstraint * ohTextViewHeight;
@property (weak, nonatomic) IBOutlet UIView * exampleValuesSeparator;
@property (weak, nonatomic) IBOutlet UIImageView * exampleValuesExpandView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * exampesButtonBottomOffset;
@property (weak, nonatomic, readwrite) IBOutlet UIButton * toggleModeButton;

@property (nonatomic) BOOL exampleExpanded;
@property (nonatomic) BOOL isSimpleMode;

@property (nonatomic) MWMOpeningHoursModel * model;

@end

@implementation MWMOpeningHoursEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self checkAuthorization];
  [self configNavBar];
  [self configTable];
  [self configAdvancedEditor];
  [self configData];
}

- (void)checkAuthorization
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  NSString * requestToken = [ud stringForKey:kOSMRequestToken];
  NSString * requestSecret = [ud stringForKey:kOSMRequestSecret];
  if (!requestToken || !requestSecret)
  {
    [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatEditTime)
                     withParameters:@{kStatValue : kStatAuthorization}];
    [self performSegueWithIdentifier:@"OpeningHoursEditor2Authorization" sender:nil];
  }
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"opening_hours");
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(onDone)];
}

- (void)configTable
{
  [kMWMOpeningHoursEditorTableCells
      enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, NSString * identifier,
                                          BOOL * _Nonnull stop)
  {
    [self.tableView registerNib:[UINib nibWithNibName:identifier bundle:nil]
         forCellReuseIdentifier:identifier];
  }];
}

- (void)configAdvancedEditor
{
  [self.editorView setTextContainerInset:{.top = 12, .left = 10, .bottom = 12, .right = 10}];
}

- (void)configData
{
  self.model = [[MWMOpeningHoursModel alloc] initWithDelegate:self];
  self.isSimpleMode = self.model.isSimpleModeCapable;
}

#pragma mark - Actions

- (void)onDone
{
  [self.model updateOpeningHours];
  std::string str = self.openingHours.UTF8String;
  // TODO (AlexZ): Store edited opening time
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - Table

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section < self.model.count)
    return [self.model cellKeyForIndexPath:indexPath];
  else
    return MWMOpeningHoursEditorAddScheduleCell;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  NSString * identifier = kMWMOpeningHoursEditorTableCells[@([self cellKeyForIndexPath:indexPath])];
  NSAssert(identifier, @"Identifier can not be nil");
  return identifier;
}

- (CGFloat)heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  CGFloat const width = self.view.width;
  if (indexPath.section < self.model.count)
    return [self.model heightForIndexPath:indexPath withWidth:width];
  else
    return [MWMOpeningHoursAddScheduleTableViewCell height];
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  if (indexPath.section < self.model.count)
    [self.model fillCell:cell atIndexPath:indexPath];
  else if ([cell isKindOfClass:[MWMOpeningHoursAddScheduleTableViewCell class]])
    ((MWMOpeningHoursAddScheduleTableViewCell *)cell).model = self.model;
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [tableView dequeueReusableCellWithIdentifier:[self cellIdentifierForIndexPath:indexPath]];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  if (!self.model.isSimpleMode)
    return 0;
  return self.model.count + (self.model.canAddSection ? 1 : 0);
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return (section < self.model.count ? [self.model numberOfRowsInSection:section] : 1);
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [self heightForRowAtIndexPath:indexPath];
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [self heightForRowAtIndexPath:indexPath];
}

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell atIndexPath:indexPath];
}

#pragma mark - Advanced mode

- (void)setExampleExpanded:(BOOL)exampleExpanded
{
  _exampleExpanded = exampleExpanded;
  self.help.hidden = !exampleExpanded;
  self.exampesButtonBottomOffset.priority = exampleExpanded ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.exampleValuesSeparator.hidden = !exampleExpanded;
  self.exampleValuesExpandView.image = [UIImage imageNamed:exampleExpanded ? @"ic_arrow_gray_up" : @"ic_arrow_gray_down"];
  if (exampleExpanded)
    [self.editorView resignFirstResponder];
  else
    [self.editorView becomeFirstResponder];
}

- (IBAction)toggleExample
{
  self.exampleExpanded = !self.exampleExpanded;
}

- (IBAction)toggleMode
{
  self.isSimpleMode = !self.isSimpleMode;
}

#pragma mark - UITextViewDelegate

- (void)textViewDidChange:(UITextView *)textView
{
  self.openingHours = textView.text;
  self.navigationItem.rightBarButtonItem.enabled = self.model.isValid;
  self.toggleModeButton.enabled = self.model.isSimpleModeCapable;
}

#pragma mark - Properties

- (void)setIsSimpleMode:(BOOL)isSimpleMode
{
  self.model.isSimpleMode = isSimpleMode;
  if (!isSimpleMode)
    self.exampleExpanded = NO;
  self.toggleModeButton.enabled = self.model.isSimpleModeCapable;
}

- (BOOL)isSimpleMode
{
  return self.model.isSimpleMode;
}

@end
