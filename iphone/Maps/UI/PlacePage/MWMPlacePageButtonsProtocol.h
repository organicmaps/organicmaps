#import "MWMPlacePageTaxiProvider.h"
#import "MWMRatingSummaryViewValueType.h"

typedef UIView * _Nullable (^MWMPlacePageButtonsDismissBlock)(NSInteger);

@protocol MWMReviewsViewModelProtocol;

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
                         referenceView:(UIView * _Nullable)referenceView
    referenceViewWhenDismissingHandler:
        (nonnull MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler;
- (void)showGallery;
- (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromPreview:(BOOL)fromPreview;

- (void)openLocalAdsURL;

- (void)openSponsoredURL:(NSURL * _Nullable)url;

- (void)openReviews:(id<MWMReviewsViewModelProtocol> _Nonnull)reviewsViewModel;

@end
