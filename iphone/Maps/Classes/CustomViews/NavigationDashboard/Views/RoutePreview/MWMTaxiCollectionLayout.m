#import "MWMTaxiCollectionLayout.h"
#import "MWMTaxiPreviewDataSource.h"

@implementation MWMTaxiCollectionLayout

- (CGSize)itemSize { return self.collectionView.size; }
- (CGFloat)minimumLineSpacing { return 0; }
- (CGFloat)minimumInteritemSpacing { return 0; }
- (UICollectionViewScrollDirection)scrollDirection { return UICollectionViewScrollDirectionHorizontal; }
- (BOOL)shouldInvalidateLayoutForBoundsChange:(CGRect)newBounds
{
  CGSize size = self.collectionView.bounds.size;
  if (newBounds.size.height == 0)
    return NO;
  
  if (!CGSizeEqualToSize(size, newBounds.size))
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self invalidateLayout];
      MWMTaxiCollectionView * cv = (MWMTaxiCollectionView *)self.collectionView;
      self.collectionView.contentOffset = CGPointZero;
      cv.currentPage = 0;
    });
  }
  return YES;
}

@end
