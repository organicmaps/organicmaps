#import "BookmarkCell.h"


@implementation BookmarkCell

@synthesize bmName;
@synthesize bmDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier
{
  self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
  if (self)
  {
    // Fonts and sizes are hard-coded because they can't be easily retrieved
    UIFont * large = [UIFont fontWithName:@"Helvetica-Bold" size:[UIFont labelFontSize] + 1];
    UIFont * small = [UIFont fontWithName:@"Helvetica" size:[UIFont systemFontSize]];
    
    bmName = [[[UILabel alloc] init] autorelease];
    bmName.font = large;
    
    bmDistance = [[[UILabel alloc] init] autorelease];
    bmDistance.font = small;
    bmDistance.textColor = [UIColor grayColor];
    bmDistance.textAlignment = UITextAlignmentRight;
    
    [self.contentView addSubview:bmName];
    [self.contentView addSubview:bmDistance];
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
  
  CGFloat xDelim = (int)(r.origin.x + w / 2);
  if (bmDistance.text.length)
  {
    CGSize const distanceTextSize = [bmDistance.text sizeWithFont:bmDistance.font];
    if (xDelim + distanceTextSize.width < r.origin.x + w)
      xDelim = r.origin.x + w - distanceTextSize.width - KPaddingX;
  }
  else // Sometimes distance is not available, so use full cell length for the name
  {
    xDelim = r.origin.x + w - KPaddingX;
  }
  
  bmName.frame = CGRectMake(r.origin.x, r.origin.y, xDelim - r.origin.x, h);
  bmDistance.frame = CGRectMake(xDelim, r.origin.y, r.origin.x + w - xDelim, h);
}

@end
