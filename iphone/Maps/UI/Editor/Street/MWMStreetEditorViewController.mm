#import "MWMStreetEditorViewController.h"
#import "MWMStreetEditorEditTableViewCell.h"
#import "SwiftBridge.h"

@interface MWMStreetEditorViewController () <MWMStreetEditorEditCellProtocol>
{
  std::vector<osm::LocalizedStreet> m_streets;
  std::string m_editedStreetName;
}

@property(nonatomic) NSUInteger selectedStreet;
@property(nonatomic) NSUInteger lastSelectedStreet;

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
  self.title = L(@"choose_street");
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
  id<MWMStreetEditorProtocol> delegate = self.delegate;
  m_streets = delegate.nearbyStreets;
  auto const & currentStreet = delegate.currentStreet;

  BOOL const haveCurrentStreet = !currentStreet.m_defaultName.empty();
  if (haveCurrentStreet)
  {
    auto const it = find(m_streets.begin(), m_streets.end(), currentStreet);
    if (it == m_streets.end())
    {
      m_streets.insert(m_streets.begin(), currentStreet);
      self.selectedStreet = 0;
    }
    else
    {
      self.selectedStreet = distance(m_streets.begin(), it);
    }
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
  UITableView * tv = self.tableView;
  [tv registerNibWithCellClass:[MWMStreetEditorEditTableViewCell class]];
}

#pragma mark - Actions

- (void)onCancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)onDone
{
  id<MWMStreetEditorProtocol> delegate = self.delegate;
  if (self.selectedStreet == NSNotFound)
    [delegate setNearbyStreet:{m_editedStreetName, ""}];
  else
    [delegate setNearbyStreet:m_streets[self.selectedStreet]];

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
    auto const & localizedStreet = m_streets[index];
    NSString * street = @(localizedStreet.m_defaultName.c_str());
    BOOL const selected = (self.selectedStreet == index);
    cell.textLabel.text = street;
    cell.detailTextLabel.text = @(localizedStreet.m_localizedName.c_str());
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

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView
                  cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  UITableViewCell * cell = nil;
  if (m_streets.empty())
  {
    cell = [tableView dequeueReusableCellWithCellClass:[MWMStreetEditorEditTableViewCell class] indexPath:indexPath];
  }
  else if (indexPath.section == 0)
  {
    Class cls =
        m_streets[indexPath.row].m_localizedName.empty() ? [UITableViewCell class] : [MWMTableViewSubtitleCell class];
    cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  }
  else
  {
    cell = [tableView dequeueReusableCellWithCellClass:[MWMStreetEditorEditTableViewCell class] indexPath:indexPath];
  }

  [self fillCell:cell indexPath:indexPath];
  return cell;
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
  return m_streets.empty() ? 1 : 2;
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

@end
