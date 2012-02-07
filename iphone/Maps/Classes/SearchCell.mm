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
  CGFloat const KPaddingX = 7.0;
  CGFloat const KPaddingBottom = 1.0;

  r.origin.x += KPaddingX;
  r.size.width -= 2 * KPaddingX;
  r.size.height -= KPaddingBottom;
  CGFloat const w = r.size.width;
  CGFloat const h = r.size.height;
  CGFloat const xDelim = r.origin.x + w / 3 * 2;
  CGFloat const yDelim = r.origin.y + h / 5 * 3;
  featureName.frame = CGRectMake(r.origin.x, r.origin.y, xDelim - r.origin.x, yDelim - r.origin.y);
  featureCountry.frame = CGRectMake(r.origin.x, yDelim, xDelim - r.origin.x, r.origin.y + h - yDelim);
  featureType.frame = CGRectMake(xDelim, r.origin.y, r.origin.x + w - xDelim, yDelim - r.origin.y);
  featureDistance.frame = CGRectMake(xDelim, yDelim, r.origin.x + w - xDelim, r.origin.y + h - yDelim);
}

@end
