@class MWMRoutePoint;

@interface MWMTaxiCollectionView : UICollectionView

- (void)setNumberOfPages:(NSUInteger)numberOfPages;
- (void)setCurrentPage:(NSUInteger)currentPage;

@end

@interface MWMTaxiPreviewDataSource : NSObject

- (instancetype)initWithCollectionView:(MWMTaxiCollectionView *)collectionView;

- (void)requestTaxiFrom:(MWMRoutePoint *)from
                     to:(MWMRoutePoint *)to
             completion:(TMWMVoidBlock)completion
                failure:(MWMStringBlock)failure;

- (NSURL *)taxiURL;
- (BOOL)isTaxiInstalled;

@end
