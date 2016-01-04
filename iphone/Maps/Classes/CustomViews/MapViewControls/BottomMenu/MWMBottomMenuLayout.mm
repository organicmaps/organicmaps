#import "MWMBottomMenuLayout.h"

@implementation MWMBottomMenuLayout

- (CGSize)collectionViewContentSize
{
  return self.collectionView.frame.size;
}

- (UICollectionViewLayoutAttributes *)layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
  UICollectionViewLayoutAttributes * attr =
      [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:indexPath];
  CGPoint origin = {};
  CGSize size = self.collectionView.frame.size;
  NSUInteger const buttonsCount = self.buttonsCount;
  CGFloat const position = (CGFloat)indexPath.item / buttonsCount;
  attr.hidden = (size.width == 0.0 || size.height == 0.0);
  if (size.width > self.layoutThreshold)
  {
    origin.x = nearbyint(position * size.width);
    size.width = nearbyint(size.width / buttonsCount);
  }
  else
  {
    origin.y = nearbyint(position * size.height);
    size.height = nearbyint(size.height / buttonsCount);
  }
  NSAssert(!CGSizeEqualToSize(size, CGSizeZero), @"Invalid cell size");
  attr.frame = {origin, size};
  return attr;
}

- (NSArray *)layoutAttributesForElementsInRect:(CGRect)rect
{
  NSUInteger const buttonsCount = self.buttonsCount;
  NSMutableArray * attrs = [NSMutableArray arrayWithCapacity:buttonsCount];
  for (NSUInteger index = 0; index < buttonsCount; index++)
  {
    NSIndexPath * indexPath = [NSIndexPath indexPathForItem:index inSection:0];
    [attrs addObject:[self layoutAttributesForItemAtIndexPath:indexPath]];
  }
  return attrs;
}

- (BOOL)shouldInvalidateLayoutForBoundsChange:(CGRect)newBounds
{
  return YES;
}

@end
