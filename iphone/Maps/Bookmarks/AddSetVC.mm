
#import "AddSetVC.h"
#import "UIViewController+Navigation.h"
#import "AddSetTableViewCell.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

#define TEXT_FIELD_TAG 666

static NSString * const kAddSetCellTableViewCell = @"AddSetTableViewCell";

@interface AddSetVC () <AddSetTableViewCellProtocol>

@property (nonatomic) AddSetTableViewCell * cell;

@end

@implementation AddSetVC

- (instancetype)init
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:@selector(onSaveClicked)];
  [(UIViewController *)self showBackButton];
  self.title = L(@"add_new_set");
  [self.tableView registerNib:[UINib nibWithNibName:kAddSetCellTableViewCell bundle:nil]
       forCellReuseIdentifier:kAddSetCellTableViewCell];
}

- (void)onSaveClicked
{
  [self onDone:self.cell.textField.text];
}

- (void)onDone:(NSString *)text
{
  if (text.length == 0)
    return;
  [self.delegate addSetVC:self didAddSetWithIndex:static_cast<int>(GetFramework().AddCategory([text UTF8String]))];
  [self.navigationController popViewControllerAnimated:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.cell = (AddSetTableViewCell *)[tableView dequeueReusableCellWithIdentifier:kAddSetCellTableViewCell];
  self.cell.delegate = self;
  self.cell.backgroundColor = [UIColor white];
  return self.cell;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(AddSetTableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath
{
  [cell.textField becomeFirstResponder];
}

@end
