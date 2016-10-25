#import "MWMOpeningHoursCell.h"
#import "Common.h"
#import "MWMOpeningHours.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

#include "std/array.hpp"

namespace
{
array<NSString *, 2> kOHClasses = {{@"_MWMOHHeaderCell", @"_MWMOHSubCell"}};
void * kContext = &kContext;
NSString * const kTableViewContentSizeKeyPath = @"contentSize";

NSAttributedString * richStringFromDay(osmoh::Day const & day, BOOL isClosedNow)
{
  auto const richString = ^NSMutableAttributedString * (NSString * str, UIFont * font, UIColor * color)
  {
    return [[NSMutableAttributedString alloc] initWithString:str
                                           attributes:@{NSFontAttributeName : font,
                                                        NSForegroundColorAttributeName : color}];
  };

  auto str = richString(day.TodayTime(), [UIFont regular17], day.m_isOpen ? [UIColor blackPrimaryText] :
                                                                            [UIColor red]);
  if (day.m_isOpen)
  {
    auto lineBreak = [[NSAttributedString alloc] initWithString:@"\n"];

    if (day.m_breaks.length)
    {
      [str appendAttributedString:lineBreak];
      [str appendAttributedString:richString(day.m_breaks, [UIFont regular13], [UIColor blackSecondaryText])];
    }

    if (isClosedNow)
    {
      [str appendAttributedString:lineBreak];
      [str appendAttributedString:richString(L(@"closed_now"), [UIFont regular13], [UIColor red])];
    }

    auto paragraphStyle = [[NSMutableParagraphStyle alloc] init];
    paragraphStyle.lineSpacing = 4;

    [str addAttributes:@{NSParagraphStyleAttributeName : paragraphStyle} range:{0, str.length}];
  }
  return str;
}

}  // namespace

#pragma mark - _MWMOHHeaderCell

@interface _MWMOHHeaderCell : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * text;
@property(weak, nonatomic) IBOutlet UIImageView * arrowIcon;

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
@property(copy, nonatomic) NSString * rawString;

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

- (void)configureWithOpeningHours:(NSString *)openingHours
             updateLayoutDelegate:(id<MWMPlacePageCellUpdateProtocol>)delegate
                      isClosedNow:(BOOL)isClosedNow;
{
  self.tableView.delegate = nil;
  self.tableView.dataSource = nil;
  self.isExtended = NO;
  self.delegate = delegate;
  self.isClosedNow = isClosedNow;
  // If we can't parse opening hours string then leave it as is.
  self.rawString = openingHours;

  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
    self->m_days = [MWMOpeningHours processRawString:openingHours];
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
  return self.isExtended && !m_days.empty() ? m_days.size() : 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const & day = m_days[indexPath.row];

  if (indexPath.row == 0)
  {
    _MWMOHHeaderCell * cell =
        [tableView dequeueReusableCellWithIdentifier:[_MWMOHHeaderCell className]];

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

    // This means that we couldn't parse opening hours string.
    if (m_days.empty())
      cell.text.text = self.rawString;
    else
      cell.text.attributedText = richStringFromDay(day, self.isClosedNow);

    return cell;
  }
  else
  {
    _MWMOHSubCell * cell = [tableView dequeueReusableCellWithIdentifier:[_MWMOHSubCell className]];
    cell.days.text = day.m_workingDays;
    cell.schedule.text = day.m_workingTimes ? day.m_workingTimes : L(@"closed");
    cell.breaks.text = day.m_breaks;
    cell.separator.hidden = indexPath.row == m_days.size() - 1;
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
