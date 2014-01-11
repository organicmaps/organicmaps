#import "AddSetVC.h"
#import "Framework.h"

#define TEXT_FIELD_TAG 666

@implementation AddSetVC

- (id)initWithIndex:(size_t *)index;
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:@selector(onSaveClicked)];
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(onCancelClicked)];
    self.title = NSLocalizedString(@"add_new_set", @"Add New Bookmark Set dialog title");
    m_index = index;
  }
  return self;
}

- (void)onCancelClicked
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)onSaveClicked
{
  UITextField * textField = (UITextField *)[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]] viewWithTag:TEXT_FIELD_TAG];
  NSString * text = textField.text;
  if (text.length)
  {
    *m_index = GetFramework().AddCategory([text UTF8String]);
    NSArray * arr = self.navigationController.viewControllers;
    for (UIViewController * v in arr)
      if ([v isMemberOfClass:NSClassFromString(@"PlacePageVC")])
      {
        [self.navigationController popToViewController:v animated:YES];
        break;
      }
  }
}

- (void)viewDidAppear:(BOOL)animated
{
  // Set focus to editor
  UITextField * textField = (UITextField *)[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]] viewWithTag:TEXT_FIELD_TAG];
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
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"EditSetNameCell"];
  if (!cell)
  {
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"EditSetNameCell"];
    cell.textLabel.text = @"Temporary Name";
    [cell layoutSubviews];
    CGRect rect = cell.textLabel.frame;
    rect.size.width = cell.contentView.bounds.size.width - cell.textLabel.frame.origin.x;
    rect.origin.x = cell.textLabel.frame.origin.x;
    UITextField * f = [[UITextField alloc] initWithFrame:rect];
    f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    f.returnKeyType = UIReturnKeyDone;
    f.clearButtonMode = UITextFieldViewModeWhileEditing;
    f.autocorrectionType = UITextAutocorrectionTypeNo;
    f.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    f.delegate = self;
    f.textColor = cell.textLabel.textColor;
    f.placeholder = NSLocalizedString(@"bookmark_set_name", @"Add Bookmark Set dialog - hint when set name is empty");
    f.autocapitalizationType = UITextAutocapitalizationTypeWords;
    f.font = [cell.textLabel.font fontWithSize:[cell.textLabel.font pointSize]];
    f.tag = TEXT_FIELD_TAG;
    [cell.contentView addSubview:f];
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    cell.textLabel.text =  @"";
  }
  return cell;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if ([textField.text length] == 0)
    return YES;
  
  [textField resignFirstResponder];
  [self onSaveClicked];
  return NO;
}

@end
