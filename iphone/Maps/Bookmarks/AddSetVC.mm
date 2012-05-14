#import "AddSetVC.h"
#import "BalloonView.h"
#import "Framework.h"

@implementation AddSetVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;

    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:@selector(onSaveClicked)];
    self.title = NSLocalizedString(@"Add New Set", @"Add New Bookmark Set dialog title");
  }
  return self;
}

- (void)onSaveClicked
{
  UITextField * textField = (UITextField *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]].accessoryView;
  NSString * text = textField.text;
  if (text.length)
  {
    m_balloon.setName = text;
    // Create category if it doesn't exist
    (void)GetFramework().GetBmCategory([text UTF8String]);
    
    // Display "Add Bookmark" dialog
    NSArray * vcs = self.navigationController.viewControllers;
    UITableViewController * addBookmarkVC = (UITableViewController *)[vcs objectAtIndex:[vcs count] - 3];
    [self.navigationController popToViewController:addBookmarkVC animated:YES];
  }
}

- (void)viewDidAppear:(BOOL)animated
{
  // Set focus to editor
  UITextField * textField = (UITextField *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]].accessoryView;  
  [textField becomeFirstResponder];

  [super viewDidAppear:animated];
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
  return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"EditSetNameCell"] autorelease];
  UITextField * f = [[[UITextField alloc] initWithFrame:CGRectMake(20, 8, 260, 21)] autorelease];
  f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  f.returnKeyType = UIReturnKeyDone;
  f.clearButtonMode = UITextFieldViewModeWhileEditing;
  f.autocorrectionType = UITextAutocorrectionTypeNo;
  f.delegate = self;
  f.placeholder = NSLocalizedString(@"Bookmark Set Name", @"Add Bookmark Set dialog - hint when set name is empty");
  cell.accessoryView = f;
  cell.selectionStyle = UITableViewCellSelectionStyleNone;
  return cell;
}

//- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
//{
//  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];
//}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (textField.text.length == 0)
    return YES;
  
  [textField resignFirstResponder];
  [self onSaveClicked];
  return NO;
}

@end
