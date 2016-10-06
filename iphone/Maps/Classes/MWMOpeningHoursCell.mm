#import "MWMOpeningHoursCell.h"
#import "Common.h"
#import "MWMOpeningHours.h"
#import "MWMPlacePageCellUpdateProtocol.h"

#include "std/array.hpp"

namespace
{
array<NSString *, 2> kOHClasses = {{@"_MWMOHHeaderCell", @"_MWMOHSubCell"}};
void * kContext = &kContext;
NSString * const kTableViewContentSizeKeyPath = @"contentSize";

}  // namespace

#pragma mark - _MWMOHHeaderCell

@interface _MWMOHHeaderCell : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * today;
@property(weak, nonatomic) IBOutlet UILabel * breaks;
@property(weak, nonatomic) IBOutlet UILabel * closedNow;
@property(weak, nonatomic) IBOutlet UIImageView * arrowIcon;
@property(weak, nonatomic) IBOutlet UIImageView * separator;

@property(copy, nonatomic) TMWMVoidBlock tapAction;

@end

@implementation _MWMOHHeaderCell

- (IBAction)extendTap
{
  if (!self.tapAction)
    return;

  self.tapAction();
  [UIView animateWithDuration:kDefaultAnimationDuration
                   animations:^{
                     self.arrowIcon.transform =
                         CGAffineTransformIsIdentity(self.arrowIcon.transform)
                             ? CGAffineTransformMakeRotation(M_PI)
                             : CGAffineTransformIdentity;
                   }];
}

@end

#pragma mark - _MWMOHSubCell

@interface _MWMOHSubCell : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * days;
@property(weak, nonatomic) IBOutlet UILabel * schedule;
@property(weak, nonatomic) IBOutlet UILabel * breaks;
@property(weak, nonatomic) IBOutlet UIImageView * separator;

@end

@implementation _MWMOHSubCell

@end

@interface MWMOpeningHoursCell ()<UITableViewDelegate, UITableViewDataSource>

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;

@property(weak, nonatomic) id<MWMPlacePageCellUpdateProtocol> delegate;

@property(nonatomic) BOOL isExtended;
@property(nonatomic) BOOL isClosedNow;

@end

@implementation MWMOpeningHoursCell
{
  vector<osmoh::Day> m_days;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  for (auto s : kOHClasses)
    [self.tableView registerNib:[UINib nibWithNibName:s bundle:nil] forCellReuseIdentifier:s];

  self.tableView.estimatedRowHeight = 48;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  [self registerObserver];
}

- (void)configureWithOpeningHours:(NSString *)openningHours
             updateLayoutDelegate:(id<MWMPlacePageCellUpdateProtocol>)delegate
                      isClosedNow:(BOOL)isClosedNow;
{
  self.tableView.delegate = nil;
  self.tableView.dataSource = nil;
  self.isExtended = NO;
  self.delegate = delegate;
  self.isClosedNow = isClosedNow;

  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
    self->m_days = [MWMOpeningHours processRawString:openningHours];
    dispatch_async(dispatch_get_main_queue(), ^{
      self.tableView.delegate = self;
      self.tableView.dataSource = self;
      [self.tableView reloadData];
    });
  });
}

#pragma mark - UITableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return self.isExtended ? m_days.size() : 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & day = m_days[indexPath.row];
  BOOL const isSeparatorHidden =
      self.isExtended ? indexPath.row == 0 : indexPath.row == m_days.size() - 1;
  if (indexPath.row == 0)
  {
    _MWMOHHeaderCell * cell =
        [tableView dequeueReusableCellWithIdentifier:[_MWMOHHeaderCell className]];
    cell.today.text = day.TodayTime();
    cell.breaks.text = day.m_breaks;
    cell.closedNow.text = self.isClosedNow ? L(@"closed_now") : nil;
    if (m_days.size() > 1)
    {
      cell.tapAction = ^{
        self.isExtended = !self.isExtended;

        NSMutableArray<NSIndexPath *> * ip = [@[] mutableCopy];

        for (auto i = 1; i < self->m_days.size(); i++)
          [ip addObject:[NSIndexPath indexPathForRow:i inSection:0]];

        if (self.isExtended)
          [self.tableView insertRowsAtIndexPaths:ip
                                withRowAnimation:UITableViewRowAnimationAutomatic];
        else
          [self.tableView deleteRowsAtIndexPaths:ip
                                withRowAnimation:UITableViewRowAnimationAutomatic];
      };
      cell.arrowIcon.hidden = NO;
    }
    else
    {
      cell.tapAction = nil;
      cell.arrowIcon.hidden = YES;
    }
    cell.separator.hidden = isSeparatorHidden;
    return cell;
  }
  else
  {
    _MWMOHSubCell * cell = [tableView dequeueReusableCellWithIdentifier:[_MWMOHSubCell className]];
    cell.days.text = day.m_workingDays;
    cell.schedule.text = day.m_workingTimes ? day.m_workingTimes : L(@"closed");
    cell.breaks.text = day.m_breaks;
    cell.separator.hidden = isSeparatorHidden;
    return cell;
  }
}

#pragma mark - Observer's methods

- (void)dealloc { [self unregisterObserver]; }
- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == kContext)
  {
    NSValue * s = change[@"new"];
    CGFloat const height = s.CGSizeValue.height;
    self.tableViewHeight.constant = height;
    [self setNeedsLayout];
    [self.delegate updateCellWithForceReposition:NO];
    return;
  }

  [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (void)unregisterObserver
{
  [self.tableView removeObserver:self forKeyPath:kTableViewContentSizeKeyPath context:kContext];
}

- (void)registerObserver
{
  [self.tableView addObserver:self
                   forKeyPath:kTableViewContentSizeKeyPath
                      options:NSKeyValueObservingOptionNew
                      context:kContext];
}
@end
