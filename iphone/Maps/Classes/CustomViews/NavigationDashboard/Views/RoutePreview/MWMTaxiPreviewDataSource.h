#import "MWMRoutePreviewTaxiCellType.h"

@class MWMRoutePoint;

@interface MWMTaxiCollectionView : UICollectionView

- (void)setNumberOfPages:(NSUInteger)numberOfPages;
- (void)setCurrentPage:(NSUInteger)currentPage;

@end

@interface MWMTaxiPreviewDataSource : NSObject

@property(nonatomic, readonly) MWMRoutePreviewTaxiCellType type;

- (instancetype)initWithCollectionView:(MWMTaxiCollectionView *)collectionView;

- (void)requestTaxiFrom:(MWMRoutePoint *)from
                     to:(MWMRoutePoint *)to
             completion:(MWMVoidBlock)completion
                failure:(MWMStringBlock)failure;

- (void)taxiURL:(MWMURLBlock)block;
- (BOOL)isTaxiInstalled;

@end
