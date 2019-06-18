#import "MWMPlacePageTaxiProvider.h"
#import "MWMRatingSummaryViewValueType.h"
#import "MWMUGCReviewSource.h"

typedef UIView * _Nullable (^MWMPlacePageButtonsDismissBlock)(NSInteger);

@protocol MWMReviewsViewModelProtocol;

@protocol MWMPlacePageButtonsProtocol<NSObject>

- (void)editPlace;
- (void)addPlace;
- (void)addBusiness;
- (void)book;
- (void)openDescriptionUrl;
- (void)openMoreUrl;
- (void)openReviewUrl;
- (void)editBookmark;
- (void)orderTaxi:(MWMPlacePageTaxiProvider)provider;
- (void)showAllFacilities;
- (void)showPhotoAtIndex:(NSInteger)index
                         referenceView:(UIView * _Nullable)referenceView
    referenceViewWhenDismissingHandler:
        (nonnull MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler;
- (void)showGallery;
- (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromSource:(MWMUGCReviewSource)source;
- (void)searchSimilar;

- (void)openLocalAdsURL;

- (void)openSponsoredURL:(NSURL * _Nullable)url;

- (void)openReviews:(id<MWMReviewsViewModelProtocol> _Nonnull)reviewsViewModel;

- (void)showPlaceDescription:(NSString * _Nonnull)htmlString;

- (void)openCatalogForURL:(NSURL * _Nullable)url;

@end
