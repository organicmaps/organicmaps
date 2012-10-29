#import "BookmarkCell.h"


@implementation BookmarkCell

@synthesize bmName;
@synthesize bmDistance;

- (id)initWithReuseIdentifier:(NSString *)identifier
{
  self = [super initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:identifier];
  if (self)
  {
    bmName = [[[UILabel alloc] init] autorelease];
    bmDistance = [[[UILabel alloc] init] autorelease];
    bmDistance.textAlignment = UITextAlignmentRight;

    // There is a hack below to get standart fonts and colors.
    self.textLabel.text = @"tmpText";
    self.detailTextLabel.text = @"tmpText";

    [super layoutSubviews];

    bmName.font = self.textLabel.font;
    bmName.textColor = self.textLabel.textColor;
    bmName.backgroundColor = [UIColor clearColor];

    bmDistance.font = self.detailTextLabel.font;
    bmDistance.textColor = self.detailTextLabel.textColor;
    bmDistance.backgroundColor = [UIColor clearColor];

    self.detailTextLabel.text = nil;
    self.textLabel.text = nil;

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
  CGFloat const KPaddingX = 9.0;
  CGFloat const KPaddingBottom = 1.0;

  if (self.imageView.image)
  {
    CGRect const imgRect = self.imageView.frame;
    CGFloat const imgPadding = imgRect.size.width + imgRect.origin.x;
    r.origin.x += imgPadding;
    r.size.width -= imgPadding;
  }

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
