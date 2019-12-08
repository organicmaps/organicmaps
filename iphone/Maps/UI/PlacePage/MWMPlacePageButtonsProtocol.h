#import "MWMPlacePageTaxiProvider.h"
#import "MWMUGCReviewSource.h"
#import <CoreApi/UgcSummaryRatingType.h>
#import <CoreApi/PlacePageData.h>

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
- (void)showAllFacilities;
- (void)showPhotoAtIndex:(NSInteger)index
                         referenceView:(UIView * _Nullable)referenceView
    referenceViewWhenDismissingHandler:
        (nonnull MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler;
- (void)showGallery;
- (void)showUGCAddReview:(UgcSummaryRatingType)value fromSource:(MWMUGCReviewSource)source;
- (void)searchSimilar;

- (void)openLocalAdsURL;

- (void)openReviews:(id<MWMReviewsViewModelProtocol> _Nonnull)reviewsViewModel;

- (void)showPlaceDescription:(NSString * _Nonnull)htmlString;

- (void)openCatalogForURL:(NSURL * _Nullable)url;

@end
