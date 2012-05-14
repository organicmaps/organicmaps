#import "SelectColorVC.h"
#import "BalloonView.h"

static NSString * g_colors [] = {
  @"blue",
  @"brown",
  @"green",
  @"orange",
  @"pink",
  @"purple",
  @"red",
  @"yellow"
};

@implementation SelectColorVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    
    self.title = NSLocalizedString(@"Bookmark Color", @"Bookmark Color dialog title");
  }
  return self;
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
  // Number of supported bookmark colors
  return sizeof(g_colors)/sizeof(g_colors[0]);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  static NSString * kCellId = @"SelectColorCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
  if (cell == nil)
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kCellId] autorelease];

  // Customize cell
  cell.imageView.image = [UIImage imageNamed:g_colors[indexPath.row]];
  if (cell.imageView.image == m_balloon.pinImage.image)
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  [cell setSelected:NO animated:YES];
  m_balloon.pinImage = cell.imageView;
  [self.navigationController popViewControllerAnimated:YES];
}

@end
