#import "SelectColorVC.h"
#import "BalloonView.h"

static NSString * g_colors [] = {
  @"placemark-red",
  @"placemark-blue",
  @"placemark-brown",
  @"placemark-green",
  @"placemark-orange",
  @"placemark-pink",
  @"placemark-purple"
};

@implementation SelectColorVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    
    self.title = NSLocalizedString(@"bookmark_color", @"Bookmark Color dialog title");
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
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  m_balloon.pinImage.image = cell.imageView.image;
  m_balloon.color = g_colors[indexPath.row];
  [self.navigationController popViewControllerAnimated:YES];
}

@end
