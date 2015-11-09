#import "MWMRoutePointLayout.h"

static CGFloat const kHeight = 36.;

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

- (CGSize)itemSize
{
  if (IPAD)
    return {304., kHeight};
  return self.actualSize;
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
  self.itemSize = self.actualSize;
  return YES;
}

- (CGSize)actualSize
{
  BOOL const isPortrait = self.parentView.superview.height > self.parentView.superview.width;
  CGFloat const width = isPortrait ? self.collectionView.width : self.collectionView.width / 2. - self.minimumInteritemSpacing / 2.;
  return {width, kHeight};
}

@end
