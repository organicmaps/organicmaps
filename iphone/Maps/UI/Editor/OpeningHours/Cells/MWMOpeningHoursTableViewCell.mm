#import "MWMOpeningHoursTableViewCell.h"

@implementation MWMOpeningHoursTableViewCell

+ (CGFloat)heightForWidth:(CGFloat)width
{
  return 0.0;
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated
{}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{}

- (void)hide
{
  self.alpha = 0.0;
}

- (void)refresh
{
  self.alpha = 1.0;
}

#pragma mark - Properties

- (void)setSection:(MWMOpeningHoursSection *)section
{
  _section = section;
  [self refresh];
}

- (NSUInteger)row
{
  UITableView * tableView = self.section.delegate.tableView;
  NSIndexPath * indexPath = [tableView indexPathForCell:self];
  if (!indexPath)
    indexPath = self.indexPathAtInit;
  return indexPath.row;
}

- (BOOL)isVisible
{
  UITableView * tableView = self.section.delegate.tableView;
  return [tableView indexPathForCell:self] != nil;
}

@end
