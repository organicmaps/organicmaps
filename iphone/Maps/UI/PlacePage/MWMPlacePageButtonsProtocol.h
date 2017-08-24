#import "MWMPlacePageTaxiProvider.h"

typedef UIView * _Nullable (^MWMPlacePageButtonsDismissBlock)(NSInteger);

@protocol MWMPlacePageButtonsProtocol<NSObject>

- (void)editPlace;
- (void)addPlace;
- (void)addBusiness;
- (void)book:(BOOL)isDescription;
- (void)editBookmark;
- (void)orderTaxi:(MWMPlacePageTaxiProvider)provider;
- (void)showAllReviews;
- (void)showAllFacilities;
- (void)showPhotoAtIndex:(NSInteger)index
                         referenceView:(nullable UIView *)referenceView
    referenceViewWhenDismissingHandler:
        (nonnull MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler;
- (void)showGallery;

- (void)openLocalAdsURL;
- (void)reviewOn:(NSInteger)starNumber;

- (void)openSponsoredURL:(nullable NSURL *)url;

@end
