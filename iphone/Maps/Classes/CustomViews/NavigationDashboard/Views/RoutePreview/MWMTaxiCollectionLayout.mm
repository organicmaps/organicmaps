#import "MWMTaxiCollectionLayout.h"

@implementation MWMTaxiCollectionLayout

- (CGSize)itemSize { return self.collectionView.size; }
- (CGFloat)minimumLineSpacing { return 0; }
- (CGFloat)minimumInteritemSpacing{ return 0; }
- (UICollectionViewScrollDirection)scrollDirection { return UICollectionViewScrollDirectionHorizontal; }

- (BOOL)shouldInvalidateLayoutForBoundsChange:(CGRect)newBounds
{
  auto const bounds = self.collectionView.bounds;
  if (CGRectGetWidth(newBounds) != CGRectGetWidth(bounds) || CGRectGetHeight(newBounds) != CGRectGetHeight(bounds))
  {
    [self invalidateLayout];
    dispatch_async(dispatch_get_main_queue(), ^{
      self.collectionView.contentOffset = {};
    });
  }
  return YES;
}

@end
