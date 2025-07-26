#import "MWMOpeningHoursEditorViewController.h"
#import "MWMOpeningHoursAddClosedTableViewCell.h"
#import "MWMOpeningHoursAddScheduleTableViewCell.h"
#import "MWMOpeningHoursAllDayTableViewCell.h"
#import "MWMOpeningHoursClosedSpanTableViewCell.h"
#import "MWMOpeningHoursDaysSelectorTableViewCell.h"
#import "MWMOpeningHoursDeleteScheduleTableViewCell.h"
#import "MWMOpeningHoursTimeSelectorTableViewCell.h"
#import "MWMOpeningHoursTimeSpanTableViewCell.h"
#import "SwiftBridge.h"

extern NSDictionary * const kMWMOpeningHoursEditorTableCells = @{
  @(MWMOpeningHoursEditorDaysSelectorCell): [MWMOpeningHoursDaysSelectorTableViewCell class],
  @(MWMOpeningHoursEditorAllDayCell): [MWMOpeningHoursAllDayTableViewCell class],
  @(MWMOpeningHoursEditorTimeSpanCell): [MWMOpeningHoursTimeSpanTableViewCell class],
  @(MWMOpeningHoursEditorTimeSelectorCell): [MWMOpeningHoursTimeSelectorTableViewCell class],
  @(MWMOpeningHoursEditorClosedSpanCell): [MWMOpeningHoursClosedSpanTableViewCell class],
  @(MWMOpeningHoursEditorAddClosedCell): [MWMOpeningHoursAddClosedTableViewCell class],
  @(MWMOpeningHoursEditorDeleteScheduleCell): [MWMOpeningHoursDeleteScheduleTableViewCell class],
  @(MWMOpeningHoursEditorAddScheduleCell): [MWMOpeningHoursAddScheduleTableViewCell class],
};

@interface MWMOpeningHoursEditorViewController () <UITableViewDelegate,
                                                   UITableViewDataSource,
                                                   UITextViewDelegate,
                                                   MWMOpeningHoursModelProtocol>

@property(weak, nonatomic, readwrite) IBOutlet UITableView * tableView;
@property(weak, nonatomic, readwrite) IBOutlet UIView * advancedEditor;
@property(weak, nonatomic, readwrite) IBOutlet MWMTextView * editorView;
@property(weak, nonatomic) IBOutlet UIView * helpView;
@property(weak, nonatomic) IBOutlet WKWebView * help;
@property(weak, nonatomic, readwrite) IBOutlet NSLayoutConstraint * ohTextViewHeight;
@property(weak, nonatomic) IBOutlet UIView * exampleValuesSeparator;
@property(weak, nonatomic) IBOutlet UIImageView * exampleValuesExpandView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * examplesButtonBottomOffset;
@property(weak, nonatomic, readwrite) IBOutlet UIButton * toggleModeButton;

@property(nonatomic) BOOL exampleExpanded;
@property(nonatomic) BOOL isSimpleMode;

@property(nonatomic) MWMOpeningHoursModel * model;

@end

@implementation MWMOpeningHoursEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configTable];
  [self configAdvancedEditor];
  [self configData];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"editor_time_title");
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(onDone)];
}

- (void)configTable
{
  [kMWMOpeningHoursEditorTableCells
      enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, Class cls, BOOL * _Nonnull stop) {
        [self.tableView registerNibWithCellClass:cls];
      }];
}

- (void)configAdvancedEditor
{
  [self.editorView setTextContainerInset:{.top = 12, .left = 10, .bottom = 12, .right = 10}];
  self.editorView.keyboardAppearance = [UIColor isNightMode] ? UIKeyboardAppearanceDark : UIKeyboardAppearanceDefault;
  NSString * path = [NSBundle.mainBundle pathForResource:@"opening_hours_how_to_edit" ofType:@"html"];
  NSString * html = [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  NSURL * baseURL = [NSURL fileURLWithPath:path];
  [self.help loadHTMLString:html baseURL:baseURL];
}

- (void)configData
{
  self.model = [[MWMOpeningHoursModel alloc] initWithDelegate:self];
  self.isSimpleMode = self.model.isSimpleModeCapable;
}

#pragma mark - Actions

- (void)onCancel
{
  [self.navigationController popViewControllerAnimated:YES];
}
- (void)onDone
{
  [self.model storeCachedData];
  [self.model updateOpeningHours];
  [self.delegate setOpeningHours:self.openingHours];
  [self onCancel];
}

#pragma mark - Table

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section < self.model.count)
    return [self.model cellKeyForIndexPath:indexPath];
  else
    return MWMOpeningHoursEditorAddScheduleCell;
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
  if (!self.parentViewController)
    return;
  if (indexPath.section < self.model.count)
    [self.model fillCell:cell atIndexPath:indexPath];
  else if ([cell isKindOfClass:[MWMOpeningHoursAddScheduleTableViewCell class]])
    ((MWMOpeningHoursAddScheduleTableViewCell *)cell).model = self.model;
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView
                  cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  Class cls = kMWMOpeningHoursEditorTableCells[@([self cellKeyForIndexPath:indexPath])];
  auto cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  return cell;
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

- (CGFloat)tableView:(UITableView * _Nonnull)tableView
    estimatedHeightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [self heightForRowAtIndexPath:indexPath];
}

- (void)tableView:(UITableView * _Nonnull)tableView
      willDisplayCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell
    forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell atIndexPath:indexPath];
}

#pragma mark - Advanced mode

- (void)setExampleExpanded:(BOOL)exampleExpanded
{
  _exampleExpanded = exampleExpanded;
  self.help.hidden = !exampleExpanded;
  self.examplesButtonBottomOffset.priority = exampleExpanded ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.exampleValuesSeparator.hidden = !exampleExpanded;
  self.exampleValuesExpandView.image =
      [UIImage imageNamed:exampleExpanded ? @"ic_arrow_gray_up" : @"ic_arrow_gray_down"];
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
