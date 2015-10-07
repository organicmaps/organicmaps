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
  CGSize const size = self.collectionView.frame.size;
  CGFloat const position = (CGFloat)indexPath.item / self.buttonsCount;
  attr.hidden = (size.width == 0.0 || size.height == 0.0);
  if (size.width > self.layoutThreshold)
  {
    CGFloat const xPos = nearbyint(position * size.width);
    CGFloat const width = nearbyint(size.width / self.buttonsCount);
    attr.frame = {{xPos, 0.0}, {width, size.height}};
  }
  else
  {
    CGFloat const yPos = nearbyint(position * size.height);
    CGFloat const height = nearbyint(size.height / self.buttonsCount);
    attr.frame = {{0.0, yPos}, {size.width, height}};
  }
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
