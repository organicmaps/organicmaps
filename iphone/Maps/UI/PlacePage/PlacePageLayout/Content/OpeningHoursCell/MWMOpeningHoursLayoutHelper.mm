#import "MWMOpeningHoursLayoutHelper.h"
#import "MWMCommon.h"
#import "MWMOpeningHours.h"
#import "MWMPlacePageData.h"
#import "MWMTableViewCell.h"
#import "SwiftBridge.h"

#include "std/array.hpp"

@interface MWMPlacePageData()

- (vector<place_page::MetainfoRows> &)mutableMetainfoRows;

@end

#pragma mark - _MWMOHHeaderCell

@interface _MWMOHHeaderCell : MWMTableViewCell

@property(weak, nonatomic) IBOutlet UILabel * text;
@property(weak, nonatomic) IBOutlet UIImageView * arrowIcon;

@property(copy, nonatomic) MWMVoidBlock tapAction;

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

@end

@implementation _MWMOHSubCell

@end

namespace
{
array<Class, 2> const kCells = {{[_MWMOHHeaderCell class], [_MWMOHSubCell class]}};

NSAttributedString * richStringFromDay(osmoh::Day const & day, BOOL isClosedNow)
{
  auto const richString =
      ^NSMutableAttributedString *(NSString * str, UIFont * font, UIColor * color)
  {
    return [[NSMutableAttributedString alloc]
        initWithString:str
            attributes:@{NSFontAttributeName : font, NSForegroundColorAttributeName : color}];
  };

  auto str = richString(day.TodayTime(), [UIFont regular17],
                        day.m_isOpen ? [UIColor blackPrimaryText] : [UIColor red]);
  if (day.m_isOpen)
  {
    auto lineBreak = [[NSAttributedString alloc] initWithString:@"\n"];

    if (day.m_breaks.length)
    {
      [str appendAttributedString:lineBreak];
      [str appendAttributedString:richString(day.m_breaks, [UIFont regular13],
                                             [UIColor blackSecondaryText])];
    }

    if (isClosedNow)
    {
      [str appendAttributedString:lineBreak];
      [str appendAttributedString:richString(L(@"closed_now"), [UIFont regular13], [UIColor red])];
    }

    auto paragraphStyle = [[NSMutableParagraphStyle alloc] init];
    paragraphStyle.lineSpacing = 4;

    [str addAttributes:@{ NSParagraphStyleAttributeName : paragraphStyle } range:{0, str.length}];
  }
  return str;
}

}  // namespace

@interface MWMOpeningHoursLayoutHelper()
{
  vector<osmoh::Day> m_days;
}

@property(weak, nonatomic) UITableView * tableView;
@property(weak, nonatomic) MWMPlacePageData * data;

@property(copy, nonatomic) NSString * rawString;
@property(nonatomic) BOOL isClosed;
@property(nonatomic) BOOL isExtended;

@end

@implementation MWMOpeningHoursLayoutHelper

- (instancetype)initWithTableView:(UITableView *)tableView
{
  self = [super init];
  if (self)
  {
    _tableView = tableView;
    [self registerCells];
  }
  return self;
}

- (void)registerCells
{
  for (Class cls : kCells)
    [self.tableView registerWithCellClass:cls];
}

- (void)configWithData:(MWMPlacePageData *)data
{
  self.data = data;
  self.rawString = [data stringForRow:place_page::MetainfoRows::OpeningHours];
  self.isClosed = [data schedule] == place_page::OpeningHours::Closed;
  m_days = [MWMOpeningHours processRawString:self.rawString];
}

- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto tableView = self.tableView;
  auto const & day = m_days[indexPath.row];
  auto data = self.data;

  if ([data metainfoRows][indexPath.row] == place_page::MetainfoRows::OpeningHours)
  {
    Class cls = [_MWMOHHeaderCell class];
    auto cell = static_cast<_MWMOHHeaderCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);

    if (m_days.size() > 1)
    {
      __weak auto weakSelf = self;
      cell.tapAction = ^{
        __strong auto self = weakSelf;
        if (!self)
          return;
        self.isExtended = !self.isExtended;

        NSMutableArray<NSIndexPath *> * ip = [@[] mutableCopy];

        auto const  metainfoSection = [self indexOfMetainfoSection];
        if (metainfoSection == NSNotFound)
        {
          LOG(LERROR, ("Incorrect indexOfMetainfoSection!"));
          return;
        }

        for (auto i = 1; i < self->m_days.size(); i++)
          [ip addObject:[NSIndexPath indexPathForRow:i inSection:metainfoSection]];

        if (self.isExtended)
        {
          if ([self extendMetainfoRowsWithSize:ip.count])
          {
            [tableView insertRowsAtIndexPaths:ip
                             withRowAnimation:UITableViewRowAnimationLeft];
          }
          else
          {
            LOG(LWARNING, ("Incorrect raw opening hours string: ", self.rawString.UTF8String));
          }
        }
        else
        {
          [self reduceMetainfoRows];
          [tableView deleteRowsAtIndexPaths:ip
                           withRowAnimation:UITableViewRowAnimationLeft];
        }
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
      cell.text.attributedText = richStringFromDay(day, self.isClosed);

    return cell;
  }
  else
  {
    Class cls = [_MWMOHSubCell class];
    auto cell = static_cast<_MWMOHSubCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    cell.days.text = day.m_workingDays;
    cell.schedule.text = day.m_workingTimes ?: L(@"closed");
    cell.breaks.text = day.m_breaks;
    return cell;
  }
}

- (NSInteger)indexOfMetainfoSection
{
  auto data = self.data;
  if (!data)
    return 0;
  auto & sections = [data sections];
  auto it = find(sections.begin(), sections.end(), place_page::Sections::Metainfo);
  if (it == sections.end())
    return NSNotFound;
  return distance(sections.begin(), it);
}

- (BOOL)extendMetainfoRowsWithSize:(NSUInteger)size
{
  if (size == 0)
  {
    LOG(LWARNING, ("Incorrect number of days!"));
    return NO;
  }
  auto data = self.data;
  if (!data)
    return NO;

  auto & metainfoRows = [data mutableMetainfoRows];
  using place_page::MetainfoRows;

  auto it = find(metainfoRows.begin(), metainfoRows.end(), MetainfoRows::OpeningHours);
  if (it == metainfoRows.end())
  {
    LOG(LERROR, ("Incorrect state!"));
    return NO;
  }

  metainfoRows.insert(++it, size, MetainfoRows::ExtendedOpeningHours);
  return YES;
}

- (void)reduceMetainfoRows
{
  auto data = self.data;
  if (!data)
    return;

  auto & metainfoRows = data.mutableMetainfoRows;
  metainfoRows.erase(remove(metainfoRows.begin(), metainfoRows.end(), place_page::MetainfoRows::ExtendedOpeningHours), metainfoRows.end());
}

@end
