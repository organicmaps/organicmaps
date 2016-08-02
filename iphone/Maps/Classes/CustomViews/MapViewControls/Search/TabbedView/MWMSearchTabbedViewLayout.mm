#import "MWMSearchTabbedViewLayout.h"
#import "Common.h"

@implementation MWMSearchTabbedViewLayout

- (CGSize)collectionViewContentSize
{
  CGSize size = self.collectionView.frame.size;
  return CGSizeMake(self.tablesCount * size.width, size.height);
}

- (UICollectionViewLayoutAttributes *)layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
  UICollectionViewLayoutAttributes * attr =
      [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:indexPath];
  CGSize const size = self.collectionView.frame.size;
  attr.size = size;
  CGFloat x = (indexPath.item + 0.5) * size.width;
  if (isInterfaceRightToLeft())
    x = self.collectionViewContentSize.width - x;
  attr.center = CGPointMake(x, 0.5 * size.height);
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

- (BOOL)shouldInvalidateLayoutForBoundsChange:(CGRect)newBounds { return YES; }
@end
