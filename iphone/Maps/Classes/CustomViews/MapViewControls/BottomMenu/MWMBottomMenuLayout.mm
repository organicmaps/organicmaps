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
  CGFloat const position = (CGFloat)indexPath.item / self.buttonsCount;
  attr.hidden = (size.width == 0.0 || size.height == 0.0);
  if (size.width > self.layoutThreshold)
  {
    origin = {nearbyint(position * size.width), 0.0};
    size.width = nearbyint(size.width / self.buttonsCount);
  }
  else
  {
    origin = {0.0, nearbyint(position * size.height)};
    size.height = nearbyint(size.height / self.buttonsCount);
  }
  NSAssert(!CGSizeEqualToSize(size, CGSizeZero), @"Invalid cell size");
  attr.frame = {origin, size};
  return attr;
}

- (NSArray *)layoutAttributesForElementsInRect:(CGRect)rect
{
  NSMutableArray * attrs = [NSMutableArray arrayWithCapacity:self.buttonsCount];
  for (NSUInteger index = 0; index < self.buttonsCount; index++)
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
