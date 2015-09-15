#import "MWMSearchTabbedViewLayout.h"

@implementation MWMSearchTabbedViewLayout

- (CGSize)collectionViewContentSize
{
  CGSize size = self.collectionView.frame.size;
  return CGSizeMake(self.tablesCount * size.width, size.height);
}

- (UICollectionViewLayoutAttributes *)layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
  UICollectionViewLayoutAttributes * attr = [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:indexPath];
  CGSize const size = self.collectionView.frame.size;
  attr.size = size;
  attr.center = CGPointMake((indexPath.item + 0.5) * size.width, 0.5 * size.height);
  return attr;
}

- (NSArray *)layoutAttributesForElementsInRect:(CGRect)rect
{
  NSMutableArray * attrs = [NSMutableArray array];
  for (NSUInteger index = 0; index < self.tablesCount; index++)
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
