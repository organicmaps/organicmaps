class MWMRoutePoint;

@interface MWMTaxiCollectionView : UICollectionView

- (void)setNumberOfPages:(NSUInteger)numberOfPages;
- (void)setCurrentPage:(NSUInteger)currentPage;

@end

@interface MWMTaxiPreviewDataSource : NSObject

- (instancetype)initWithCollectionView:(MWMTaxiCollectionView *)collectionView;

- (void)requestTaxiFrom:(MWMRoutePoint const &)from
                         to:(MWMRoutePoint const &)to
                          completion:(TMWMVoidBlock)completion
                             failure:(MWMStringBlock)failure;

- (NSURL *)taxiURL;

@end
