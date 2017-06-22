typedef UIView * _Nullable (^MWMPlacePageButtonsDismissBlock)(NSInteger);

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
                         referenceView:(nullable UIView *)referenceView
    referenceViewWhenDismissingHandler:
        (nonnull MWMPlacePageButtonsDismissBlock)referenceViewWhenDismissingHandler;
- (void)showGalery;

- (void)openLocalAdsURL;

- (void)openViatorURL:(nullable NSURL *)url;

@end
