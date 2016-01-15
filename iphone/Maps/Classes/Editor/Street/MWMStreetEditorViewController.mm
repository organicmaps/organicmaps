#import "MWMStreetEditorCommonTableViewCell.h"
#import "MWMStreetEditorEditTableViewCell.h"
#import "MWMStreetEditorViewController.h"

namespace
{
  NSString * const kStreetEditorCommonCell = @"MWMStreetEditorCommonTableViewCell";
  NSString * const kStreetEditorSpacerCell = @"MWMEditorSpacerCell";
  NSString * const kStreetEditorEditCell = @"MWMStreetEditorEditTableViewCell";
} // namespace

@interface MWMStreetEditorViewController ()<UITableViewDelegate, UITableViewDataSource,
                                             MWMStreetEditorCommonTableViewCellProtocol,
                                             MWMStreetEditorEditCellProtocol>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (nonatomic) NSMutableArray<NSString *> * streets;

@property (nonatomic) NSUInteger selectedStreet;
@property (nonatomic) NSUInteger lastSelectedStreet;

@property (nonatomic) NSString * editedStreetName;

@end

@implementation MWMStreetEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configData];
  [self configTable];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"street");
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(onCancel)];
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(onDone)];
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)configData
{
  self.streets = [[self.delegate getNearbyStreets] mutableCopy];
  NSString * currentStreet = [self.delegate getStreet];
  if (currentStreet)
  {
    [self.streets removeObject:currentStreet];
    [self.streets insertObject:currentStreet atIndex:0];
  }
  self.editedStreetName = @"";
  self.selectedStreet = 0;
}

- (void)configTable
{
  [self.tableView registerNib:[UINib nibWithNibName:kStreetEditorCommonCell bundle:nil]
       forCellReuseIdentifier:kStreetEditorCommonCell];
  [self.tableView registerNib:[UINib nibWithNibName:kStreetEditorSpacerCell bundle:nil]
       forCellReuseIdentifier:kStreetEditorSpacerCell];
  [self.tableView registerNib:[UINib nibWithNibName:kStreetEditorEditCell bundle:nil]
       forCellReuseIdentifier:kStreetEditorEditCell];
}

#pragma mark - Actions

- (void)onCancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)onDone
{
  NSString * street = (self.selectedStreet == NSNotFound ? self.editedStreetName : self.streets[self.selectedStreet]);
  [self.delegate setStreet:street];
  [self onCancel];
}

#pragma mark - MWMStreetEditorCommonTableViewCellProtocol

- (void)selectCell:(UITableViewCell *)selectedCell
{
  self.selectedStreet = [self.tableView indexPathForCell:selectedCell].row;
  [self onDone];
}

- (void)fillCell:(UITableViewCell *)cell indexPath:(NSIndexPath *)indexPath
{
  if ([cell isKindOfClass:[MWMStreetEditorEditTableViewCell class]])
  {
    MWMStreetEditorEditTableViewCell * tCell = (MWMStreetEditorEditTableViewCell *)cell;
    [tCell configWithDelegate:self street:self.editedStreetName];
  }
  else if ([cell isKindOfClass:[MWMStreetEditorCommonTableViewCell class]])
  {
    NSUInteger const index = indexPath.row;
    MWMStreetEditorCommonTableViewCell * tCell = (MWMStreetEditorCommonTableViewCell *)cell;
    NSString * street = self.streets[index];
    BOOL const selected = (self.selectedStreet == index);
    [tCell configWithDelegate:self street:street selected:selected];
  }
}

#pragma mark - MWMStreetEditorEditCellProtocol

- (void)editCellTextChanged:(NSString *)text
{
  if (text && text.length != 0)
  {
    self.editedStreetName = text;
    if (self.selectedStreet != NSNotFound)
    {
      self.lastSelectedStreet = self.selectedStreet;
      self.selectedStreet = NSNotFound;
    }
  }
  else
  {
    self.selectedStreet = self.lastSelectedStreet;
  }
  for (UITableViewCell * cell in self.tableView.visibleCells)
  {
    if (![cell isKindOfClass:[MWMStreetEditorCommonTableViewCell class]])
      continue;
    NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
    [self fillCell:cell indexPath:indexPath];
  }
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSUInteger const streetsCount = self.streets.count;
  if (streetsCount == 0)
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorEditCell];
  NSUInteger const index = indexPath.row;
  if (index < streetsCount)
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorCommonCell];
  else if (index == streetsCount)
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorSpacerCell];
  else
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorEditCell];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  NSUInteger count = self.streets.count;
  if (count != 0)
    count++; // Spacer cell;
  count++;
  return count;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(UITableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell indexPath:indexPath];
}

@end
