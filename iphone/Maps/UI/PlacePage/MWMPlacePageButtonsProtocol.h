typedef UIView * (^MWMPlacePageButtonsDismissBlock)(NSInteger);

@protocol MWMPlacePageButtonsProtocol<NSObject>

- (void)editPlace;
- (void)addPlace;
- (void)addBusiness;
- (void)book:(BOOL)isDescription;
- (void)editBookmark;
- (void)taxiTo;
- (void)showAllReviews;
- (void)showAllFacilities;
- (void)showPhotoAtIndex:(NSInteger)index
           referenceView:(UIView *)referenceView
           referenceViewWhenDismissingHandler:(MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler;
- (void)showGalery;

- (void)openLocalAdsURL;

@end
