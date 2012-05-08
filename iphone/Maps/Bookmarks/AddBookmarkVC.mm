#import "AddBookmarkVC.h"

@implementation AddBookmarkVC

- (id)initWithStyle:(UITableViewStyle)style
{
  self = [super initWithStyle:style];
  if (self)
  {
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(onAddClicked)];
    self.title = NSLocalizedString(@"Add Bookmark", @"Add bookmark dialog title");
  }
  return self;
}

- (void)onAddClicked:(id)sender
{
}

- (void)viewWillAppear:(BOOL)animated
{
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  [super viewWillAppear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 3;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"AddBMCell"];
  if (cell == nil)
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"AddBMCell"] autorelease];

  switch (indexPath.row)
  {
    case 0:
      cell.textLabel.text = NSLocalizedString(@"Name", @"Add bookmark dialog - bookmark name");
      cell.detailTextLabel.text = @"Washington";
    break;

    case 1:
      cell.textLabel.text = NSLocalizedString(@"Set", @"Add bookmark dialog - bookmark set");
      cell.detailTextLabel.text = @"Default";
    break;

    case 2:
      cell.textLabel.text = NSLocalizedString(@"Color", @"Add bookmark dialog - bookmark color");
      cell.detailTextLabel.text = @"Blue";
    break;
  }
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];
}

@end
