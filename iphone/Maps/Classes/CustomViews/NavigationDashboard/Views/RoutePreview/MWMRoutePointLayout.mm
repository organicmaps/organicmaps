#import "MWMRoutePointLayout.h"

@implementation MWMRoutePointLayout

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
    self.isNeedToInitialLayout = YES;
  return self;
}

- (CGSize)collectionViewContentSize
{
  return self.collectionView.bounds.size;
}

- (CGFloat)minimumLineSpacing
{
  return 8.;
}

- (CGFloat)minimumInteritemSpacing
{
  return 8.;
}

- (UICollectionViewLayoutAttributes *)initialLayoutAttributesForAppearingItemAtIndexPath:(NSIndexPath *)itemIndexPath
{
  if (!self.isNeedToInitialLayout)
    return nil;
  UICollectionViewLayoutAttributes * attr = [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:itemIndexPath];
  attr.alpha = 0.;
  return attr;
}

- (UICollectionViewLayoutAttributes *)layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
  UICollectionViewLayoutAttributes * attr = [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:indexPath];
  attr.center = {self.collectionView.midX, self.collectionView.maxY};
  return attr;
}

- (BOOL)shouldInvalidateLayoutForBoundsChange:(CGRect)newBounds
{
  BOOL const isPortrait = self.collectionView.superview.superview.superview.height > self.collectionView.superview.superview.superview.width;
  CGFloat const width = isPortrait ? newBounds.size.width : newBounds.size.width / 2. - self.minimumInteritemSpacing / 2.;
  self.itemSize = {width, 36.};
  return YES;
}

@end
