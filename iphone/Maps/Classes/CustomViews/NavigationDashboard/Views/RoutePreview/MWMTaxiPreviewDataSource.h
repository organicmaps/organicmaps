class MWMRoutePoint;

@interface MWMTaxiCollectionView : UICollectionView

@property(nonatomic) NSUInteger numberOfPages;

@end

@interface MWMTaxiPreviewDataSource : NSObject

- (instancetype)initWithCollectionView:(MWMTaxiCollectionView *)collectionView;

- (void)requestTaxiFrom:(MWMRoutePoint const &)from
                         to:(MWMRoutePoint const &)to
                          completion:(TMWMVoidBlock)completion
                             failure:(TMWMVoidBlock)failure;

- (NSURL *)taxiURL;
- (BOOL)isTaxiInstalled;

@end
