#import "SearchCell.h"
#import "CompassView.h"

@implementation SearchCell

@synthesize featureName;
@synthesize featureType;
@synthesize featureCountry;
@synthesize featureDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier
{
  self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
  if (self)
  {
    // Fonts and sizes are hard-coded because they can't be easily retrieved
    UIFont * large = [UIFont fontWithName:@"Helvetica-Bold" size:[UIFont labelFontSize] + 1];
    UIFont * small = [UIFont fontWithName:@"Helvetica" size:[UIFont systemFontSize]];

    featureName = [[[UILabel alloc] init] autorelease];
    featureName.font = large;
    featureType = [[[UILabel alloc] init] autorelease];
    featureType.font = small;
    featureType.textColor = [UIColor grayColor];
    featureType.textAlignment = UITextAlignmentRight;
    featureCountry = [[[UILabel alloc] init] autorelease];
    featureCountry.font = small;
    featureCountry.textColor = [UIColor grayColor];
    featureDistance = [[[UILabel alloc] init] autorelease];
    featureDistance.font = small;
    featureDistance.textColor = [UIColor grayColor];
    featureDistance.textAlignment = UITextAlignmentRight;

    [self.contentView addSubview:featureName];
    [self.contentView addSubview:featureType];
    [self.contentView addSubview:featureCountry];
    [self.contentView addSubview:featureDistance];
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  CGRect r = self.contentView.bounds;
  // Leave some experimentally choosen paddings
  CGFloat const KPaddingX = 10.0;
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
  if (featureType.text.length)
  {
    CGSize const typeTextSize = [featureType.text sizeWithFont:featureType.font];
    if (xTopDelim + typeTextSize.width < r.origin.x + w)
      xTopDelim = r.origin.x + w - typeTextSize.width - KPaddingX;
  }
  featureName.frame = CGRectMake(r.origin.x, r.origin.y, xTopDelim - r.origin.x, yDelim - r.origin.y);
  featureType.frame = CGRectMake(xTopDelim, r.origin.y, r.origin.x + w - xTopDelim, yDelim - r.origin.y);

  if (featureDistance.text.length)
  {
    CGSize const distanceTextSize = [featureDistance.text sizeWithFont:featureDistance.font];
    if (xBottomDelim + distanceTextSize.width < r.origin.x + w)
      xBottomDelim = r.origin.x + w - distanceTextSize.width - KPaddingX;
  }
  featureCountry.frame = CGRectMake(r.origin.x, yDelim, xBottomDelim - r.origin.x, r.origin.y + h - yDelim);
  featureDistance.frame = CGRectMake(xBottomDelim, yDelim, r.origin.x + w - xBottomDelim, r.origin.y + h - yDelim);
}

@end
