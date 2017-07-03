#import "MWMUGCReviewController.h"
#import "MWMUGCReviewVM.h"
#import "SwiftBridge.h"

#include "ugc/types.hpp"

@interface MWMUGCReviewController () 

@property(weak, nonatomic) MWMUGCReviewVM<MWMUGCSpecificReviewDelegate, MWMUGCTextReviewDelegate> * viewModel;

@end

@implementation MWMUGCReviewController

+ (instancetype)instanceFromViewModel:(MWMUGCReviewVM *)viewModel
{
  auto inst = [[MWMUGCReviewController alloc] initWithNibName:self.className bundle:nil];
  inst.viewModel = static_cast<MWMUGCReviewVM<MWMUGCSpecificReviewDelegate, MWMUGCTextReviewDelegate> *>(viewModel);
  return inst;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = self.viewModel.name;
  auto tv = self.tableView;
  tv.estimatedRowHeight = 44;
  tv.rowHeight = UITableViewAutomaticDimension;
  [tv registerWithCellClass:[MWMUGCSpecificReviewCell class]];
  [tv registerWithCellClass:[MWMUGCTextReviewCell class]];
  self.navigationItem.rightBarButtonItem =
  [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                target:self
                                                action:@selector(submitTap)];
}

- (void)submitTap
{
  [self.viewModel submit];
  [self.navigationController popViewControllerAnimated:YES];
}

#pragma mark - UITableView's methods

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return self.viewModel.numberOfRows;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  using ugc_review::Row;
  auto vm = self.viewModel;
  switch ([vm rowForIndexPath:indexPath])
  {
  case Row::Detail:
  {
    auto const & record = [vm recordForIndexPath:indexPath];
    Class cls = [MWMUGCSpecificReviewCell class];
    auto c = static_cast<MWMUGCSpecificReviewCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [c configWithSpecification:@(record.m_key.m_key.c_str())
                          rate:static_cast<NSInteger>(record.m_value)
                   atIndexPath:indexPath
                      delegate:vm];
    return c;
  }
  case Row::SpecialQuestion:
  case Row::Message:
  {
    Class cls = [MWMUGCTextReviewCell class];
    auto c = static_cast<MWMUGCTextReviewCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [c configWithDelegate:vm];
    return c;
  }
  }
}

@end
