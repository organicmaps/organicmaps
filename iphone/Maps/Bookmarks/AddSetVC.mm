
#import "AddSetVC.h"
#import "Framework.h"
#import "UIKitCategories.h"

#define TEXT_FIELD_TAG 666

@implementation AddSetVC

- (id)init
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:@selector(onSaveClicked)];
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(onCancelClicked)];
    self.title = L(@"add_new_set");
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
    [self.delegate addSetVC:self didAddSetWithIndex:static_cast<int>(GetFramework().AddCategory([text UTF8String]))];
    [self.navigationController popViewControllerAnimated:YES];
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
    f.placeholder = L(@"bookmark_set_name");
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
