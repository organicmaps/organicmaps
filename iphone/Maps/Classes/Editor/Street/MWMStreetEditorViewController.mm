#import "MWMStreetEditorEditTableViewCell.h"
#import "MWMStreetEditorViewController.h"

namespace
{
  NSString * const kStreetEditorEditCell = @"MWMStreetEditorEditTableViewCell";
} // namespace

using namespace osm;

@interface MWMStreetEditorViewController () <MWMStreetEditorEditCellProtocol>
{
  vector<osm::LocalizedStreet> m_streets;
  string m_editedStreetName;
}

@property (nonatomic) NSUInteger selectedStreet;
@property (nonatomic) NSUInteger lastSelectedStreet;

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
  m_streets = self.delegate.nearbyStreets;
  auto const & currentStreet = self.delegate.currentStreet;

  BOOL const haveCurrentStreet = !currentStreet.m_defaultName.empty();
  if (haveCurrentStreet)
  {
    auto const it = find(m_streets.begin(), m_streets.end(), currentStreet);
    self.selectedStreet = it - m_streets.begin();
  }
  else
  {
    self.selectedStreet = NSNotFound;
  }
  self.navigationItem.rightBarButtonItem.enabled = haveCurrentStreet;
  self.lastSelectedStreet = NSNotFound;

  m_editedStreetName = "";
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
  if (self.selectedStreet == NSNotFound)
    [self.delegate setNearbyStreet:{m_editedStreetName, ""}];
  else
    [self.delegate setNearbyStreet:m_streets[self.selectedStreet]];

  [self onCancel];
}

- (void)fillCell:(UITableViewCell *)cell indexPath:(NSIndexPath *)indexPath
{
  if ([cell isKindOfClass:[MWMStreetEditorEditTableViewCell class]])
  {
    MWMStreetEditorEditTableViewCell * tCell = (MWMStreetEditorEditTableViewCell *)cell;
    [tCell configWithDelegate:self street:@(m_editedStreetName.c_str())];
  }
  else
  {
    NSUInteger const index = indexPath.row;
    // TODO: also display localized name if it exists.
    NSString * street = @(m_streets[index].m_defaultName.c_str());
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
    m_editedStreetName = text.UTF8String;
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
  if (m_streets.empty())
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorEditCell];
  if (indexPath.section == 0)
    return [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
  else
    return [tableView dequeueReusableCellWithIdentifier:kStreetEditorEditCell];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  auto const count = m_streets.size();
  if ((section == 0 && count == 0) || section != 0)
    return 1;
  return count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return m_streets.empty()? 1 : 2;
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
