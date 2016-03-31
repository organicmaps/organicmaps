#import "MWMStreetEditorEditTableViewCell.h"
#import "MWMStreetEditorViewController.h"

namespace
{
  NSString * const kStreetEditorEditCell = @"MWMStreetEditorEditTableViewCell";
} // namespace

@interface MWMStreetEditorViewController () <MWMStreetEditorEditCellProtocol>

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
  self.title = L(@"choose_street").capitalizedString;
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
  BOOL const haveCurrentStreet = (currentStreet && currentStreet.length != 0);
  if (haveCurrentStreet)
  {
    [self.streets removeObject:currentStreet];
    [self.streets insertObject:currentStreet atIndex:0];
  }
  self.editedStreetName = @"";
  self.selectedStreet = haveCurrentStreet ? 0 : NSNotFound;
  self.lastSelectedStreet = NSNotFound;
  self.navigationItem.rightBarButtonItem.enabled = haveCurrentStreet;
}

- (void)configTable
{
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

- (void)fillCell:(UITableViewCell *)cell indexPath:(NSIndexPath *)indexPath
{
  if ([cell isKindOfClass:[MWMStreetEditorEditTableViewCell class]])
  {
    MWMStreetEditorEditTableViewCell * tCell = (MWMStreetEditorEditTableViewCell *)cell;
    [tCell configWithDelegate:self street:self.editedStreetName];
  }
  else
  {
    NSUInteger const index = indexPath.row;
    NSString * street = self.streets[index];
    BOOL const selected = (self.selectedStreet == index);
    cell.textLabel.text = street;
    cell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
  }
}

#pragma mark - MWMStreetEditorEditCellProtocol

- (void)editCellTextChanged:(NSString *)text
{
  if (text && text.length != 0)
  {
    self.navigationItem.rightBarButtonItem.enabled = YES;
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
    self.navigationItem.rightBarButtonItem.enabled = (self.selectedStreet != NSNotFound);
  }
  for (UITableViewCell * cell in self.tableView.visibleCells)
  {
    if ([cell isKindOfClass:[MWMStreetEditorEditTableViewCell class]])
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
  if (indexPath.section == 0)
    return [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
  else
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorEditCell];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  NSUInteger const count = self.streets.count;
  if ((section == 0 && count == 0) || section != 0)
    return 1;
  return count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.streets.count > 0 ? 2 : 1;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  if ([cell isKindOfClass:[MWMStreetEditorEditTableViewCell class]])
    return;

  self.selectedStreet = indexPath.row;
  [self onDone];
}

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(UITableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell indexPath:indexPath];
}

@end
