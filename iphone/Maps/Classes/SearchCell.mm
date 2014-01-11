#import "SearchCell.h"

@implementation SearchCell

- (id)initWithReuseIdentifier:(NSString *)identifier
{
  self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
  if (self)
  {
    // Fonts and sizes are hard-coded because they can't be easily retrieved
    UIFont * large = [UIFont fontWithName:@"Helvetica-Bold" size:[UIFont labelFontSize] + 1];
    UIFont * small = [UIFont fontWithName:@"Helvetica" size:[UIFont systemFontSize]];

    _featureName = [[UILabel alloc] init];
    _featureName.font = large;
    _featureType = [[UILabel alloc] init];
    _featureType.font = small;
    _featureType.textColor = [UIColor grayColor];
    _featureType.textAlignment = UITextAlignmentRight;
    _featureCountry = [[UILabel alloc] init];
    _featureCountry.font = small;
    _featureCountry.textColor = [UIColor grayColor];
    _featureDistance = [[UILabel alloc] init];
    _featureDistance.font = small;
    _featureDistance.textColor = [UIColor grayColor];
    _featureDistance.textAlignment = UITextAlignmentRight;

    [self.contentView addSubview:_featureName];
    [self.contentView addSubview:_featureType];
    [self.contentView addSubview:_featureCountry];
    [self.contentView addSubview:_featureDistance];
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  CGRect r = self.contentView.bounds;
  // Leave some experimentally choosen paddings
  CGFloat const KPaddingX = 9.0;
  CGFloat const KPaddingBottom = 1.0;

  r.origin.x += KPaddingX;
  r.size.width -= 2 * KPaddingX;
  r.size.height -= KPaddingBottom;

  // Labels on the right should always fit and be visible, but not more than half of the cell
  CGFloat const w = r.size.width;
  CGFloat const h = r.size.height;
  CGFloat const yDelim = (int)(r.origin.y + h / 5 * 3);

  CGFloat xTopDelim = (int)(r.origin.x + w / 2);
  CGFloat xBottomDelim = xTopDelim;
  if ([_featureDistance.text length])
  {
    CGSize const distanceTextSize = [_featureDistance.text sizeWithFont:_featureDistance.font];
    if (xTopDelim + distanceTextSize.width < r.origin.x + w)
      xTopDelim = r.origin.x + w - distanceTextSize.width - KPaddingX;
  }
  else // Sometimes distance is not available, so use full cell length for the name
  {
    xTopDelim = r.origin.x + w - KPaddingX;
  }

  _featureName.frame = CGRectMake(r.origin.x, r.origin.y, xTopDelim - r.origin.x, yDelim - r.origin.y);
  _featureDistance.frame = CGRectMake(xTopDelim, r.origin.y, r.origin.x + w - xTopDelim, yDelim - r.origin.y);

  if ([_featureType.text length])
  {
    CGSize const typeTextSize = [_featureType.text sizeWithFont:_featureType.font];
    if (xBottomDelim + typeTextSize.width < r.origin.x + w)
      xBottomDelim = r.origin.x + w - typeTextSize.width - KPaddingX;
  }
  _featureCountry.frame = CGRectMake(r.origin.x, yDelim, xBottomDelim - r.origin.x, r.origin.y + h - yDelim);
  _featureType.frame = CGRectMake(xBottomDelim, yDelim, r.origin.x + w - xBottomDelim, r.origin.y + h - yDelim);
}

@end
